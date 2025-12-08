import ts from "typescript";

import { CodeGenerator } from "./";
import type { VisitContext } from "./visitor";

function visitObjectPropertyName(
    this: CodeGenerator,
    node: ts.PropertyName,
    context: VisitContext,
): string {
    if (ts.isNumericLiteral(node)) {
        return context.isBracketNotationPropertyAccess
            ? node.getText()
            : `"${node.getText()}"`;
    }
    if (ts.isStringLiteral(node)) {
        return `"${
            node.getText().substring(
                1,
                node.getText().length - 1,
            ) // remove trailing "' from original name
        }"`;
    }
    if (ts.isComputedPropertyName(node)) {
        const compExpr = node.expression as ts.Expression;
        let propName = this.visit(compExpr, context);
        if (ts.isIdentifier(compExpr)) {
            const scope = this.getScopeForNode(compExpr);
            const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                compExpr.getText(),
                scope,
            )!;
            propName = this.getDerefCode(
                propName,
                this.getJsVarName(compExpr as ts.Identifier),
                typeInfo,
            );
        }
        propName += ".to_std_string()";
        return propName;
    }
    if (context.isBracketNotationPropertyAccess) {
        return this.visit(node, context);
    }
    if (ts.isIdentifier(node) && context.isObjectLiteralExpression) {
        const name = node.getText();
        if (
            context.localScopeSymbols.has(name) ||
            context.topLevelScopeSymbols.has(name)
        ) {
            const scope = this.getScopeForNode(node);
            const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                node.getText(),
                scope,
            )!;
            return `${
                this.getDerefCode(
                    node.getText(),
                    node.getText(),
                    typeInfo,
                )
            }.to_std_string()`;
        }
    }
    return `"${node.getText()}"`;
}

export function visitObjectLiteralExpression(
    this: CodeGenerator,
    node: ts.ObjectLiteralExpression,
    context: VisitContext,
): string {
    const obj = node as ts.ObjectLiteralExpression;
    let props = "";
    for (const prop of obj.properties) {
        if (ts.isPropertyAssignment(prop)) {
            const key = visitObjectPropertyName.call(this, prop.name, {
                ...context,
                isObjectLiteralExpression: true,
            });
            const initializer = prop.initializer;
            let value = this.visit(initializer, context);
            if (ts.isIdentifier(initializer)) {
                const scope = this.getScopeForNode(initializer);
                const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                    initializer.text,
                    scope,
                )!;
                if (typeInfo && !typeInfo.isBuiltin && !typeInfo.isParameter) {
                    value = this.getDerefCode(
                        value,
                        this.getJsVarName(initializer),
                        typeInfo,
                    );
                }
            }

            props += `{${key}, ${value}},`;
        } else if (ts.isShorthandPropertyAssignment(prop)) {
            const key = visitObjectPropertyName.call(this, prop.name, {
                ...context,
                isObjectLiteralExpression: true,
            });
            const scope = this.getScopeForNode(prop.name);
            const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                prop.name.text,
                scope,
            )!;
            let value = this.visit(prop.name, context);
            if (typeInfo && !typeInfo.isBuiltin && !typeInfo.isParameter) {
                value = this.getDerefCode(
                    value,
                    this.getJsVarName(prop.name),
                    typeInfo,
                );
            }

            props += `{${key}, ${value}},`;
        }
    }
    return `jspp::AnyValue::make_object({${props}})`;
}

export function visitArrayLiteralExpression(
    this: CodeGenerator,
    node: ts.ArrayLiteralExpression,
    context: VisitContext,
): string {
    const elements = (node as ts.ArrayLiteralExpression).elements
        .map((elem) => {
            let elemText = this.visit(elem, context);
            if (ts.isIdentifier(elem)) {
                const scope = this.getScopeForNode(elem);
                const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                    elem.text,
                    scope,
                )!;
                if (typeInfo && !typeInfo.isBuiltin && !typeInfo.isParameter) {
                    elemText = this.getDerefCode(
                        elemText,
                        this.getJsVarName(elem),
                        typeInfo,
                    );
                }
            }
            return elemText;
        })
        .join(", ");
    return `jspp::AnyValue::make_array({${elements}})`;
}

export function visitPrefixUnaryExpression(
    this: CodeGenerator,
    node: ts.PrefixUnaryExpression,
    context: VisitContext,
): string {
    const prefixUnaryExpr = node as ts.PrefixUnaryExpression;
    const operand = this.visit(prefixUnaryExpr.operand, context);
    const operator = ts.tokenToString(prefixUnaryExpr.operator);

    if (operator === "++" || operator === "--") {
        let target = operand;
        if (ts.isIdentifier(prefixUnaryExpr.operand)) {
            const scope = this.getScopeForNode(prefixUnaryExpr.operand);
            const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                prefixUnaryExpr.operand.getText(),
                scope,
            )!;
            if (context.derefBeforeAssignment) {
                target = this.getDerefCode(operand, operand, typeInfo);
            } else if (typeInfo.needsHeapAllocation) {
                target = `*${operand}`;
            }
        }
        return `${operator}(${target})`;
    }
    if (operator === "~") {
        let target = operand;
        if (ts.isIdentifier(prefixUnaryExpr.operand)) {
            const scope = this.getScopeForNode(prefixUnaryExpr.operand);
            const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                prefixUnaryExpr.operand.getText(),
                scope,
            )!;
            if (context.derefBeforeAssignment) {
                target = this.getDerefCode(operand, operand, typeInfo);
            } else if (typeInfo.needsHeapAllocation) {
                target = `*${operand}`;
            }
        }
        return `${operator}(${target})`;
    }
    return `${operator}${operand}`;
}

export function visitPostfixUnaryExpression(
    this: CodeGenerator,
    node: ts.PostfixUnaryExpression,
    context: VisitContext,
): string {
    const postfixUnaryExpr = node as ts.PostfixUnaryExpression;
    const operand = this.visit(postfixUnaryExpr.operand, context);
    const operator = ts.tokenToString(postfixUnaryExpr.operator);
    let target = operand;
    if (ts.isIdentifier(postfixUnaryExpr.operand)) {
        const scope = this.getScopeForNode(postfixUnaryExpr.operand);
        const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
            postfixUnaryExpr.operand.getText(),
            scope,
        )!;
        if (context.derefBeforeAssignment) {
            target = this.getDerefCode(operand, operand, typeInfo);
        } else if (typeInfo.needsHeapAllocation) {
            target = `*${operand}`;
        }
    }
    return `(${target})${operator}`;
}

export function visitParenthesizedExpression(
    this: CodeGenerator,
    node: ts.ParenthesizedExpression,
    context: VisitContext,
): string {
    const parenExpr = node as ts.ParenthesizedExpression;
    return `(${this.visit(parenExpr.expression, context)})`;
}

export function visitPropertyAccessExpression(
    this: CodeGenerator,
    node: ts.PropertyAccessExpression,
    context: VisitContext,
): string {
    const propAccess = node as ts.PropertyAccessExpression;
    const exprText = this.visit(propAccess.expression, context);
    const propName = propAccess.name.getText();

    const scope = this.getScopeForNode(propAccess.expression);
    const typeInfo = ts.isIdentifier(propAccess.expression)
        ? this.typeAnalyzer.scopeManager.lookupFromScope(
            propAccess.expression.getText(),
            scope,
        )
        : null;

    if (
        ts.isIdentifier(propAccess.expression) && !typeInfo &&
        !this.isBuiltinObject(propAccess.expression)
    ) {
        return `jspp::RuntimeError::throw_unresolved_reference_error(${
            this.getJsVarName(
                propAccess.expression,
            )
        })`;
    }

    let finalExpr = exprText;
    if (
        typeInfo &&
        !typeInfo.isParameter &&
        !typeInfo.isBuiltin &&
        ts.isIdentifier(propAccess.expression)
    ) {
        finalExpr = this.getDerefCode(
            exprText,
            this.getJsVarName(propAccess.expression),
            typeInfo,
        );
    }

    return `${finalExpr}.get_own_property("${propName}")`;
}

export function visitElementAccessExpression(
    this: CodeGenerator,
    node: ts.ElementAccessExpression,
    context: VisitContext,
): string {
    const elemAccess = node as ts.ElementAccessExpression;
    const exprText = this.visit(elemAccess.expression, context);
    let argText = visitObjectPropertyName.call(
        this,
        elemAccess.argumentExpression as ts.PropertyName,
        { ...context, isBracketNotationPropertyAccess: true },
    );

    // Dereference the expression being accessed
    const exprScope = this.getScopeForNode(elemAccess.expression);
    const exprTypeInfo = ts.isIdentifier(elemAccess.expression)
        ? this.typeAnalyzer.scopeManager.lookupFromScope(
            elemAccess.expression.getText(),
            exprScope,
        )
        : null;

    if (
        ts.isIdentifier(elemAccess.expression) && !exprTypeInfo &&
        !this.isBuiltinObject(elemAccess.expression as ts.Identifier)
    ) {
        return `jspp::RuntimeError::throw_unresolved_reference_error(${
            this.getJsVarName(
                elemAccess.expression as ts.Identifier,
            )
        })`;
    }

    let finalExpr = exprText;
    if (
        exprTypeInfo &&
        !exprTypeInfo.isParameter &&
        !exprTypeInfo.isBuiltin &&
        ts.isIdentifier(elemAccess.expression)
    ) {
        finalExpr = this.getDerefCode(
            exprText,
            this.getJsVarName(elemAccess.expression),
            exprTypeInfo,
        );
    }

    // Dereference the argument expression if it's an identifier
    if (ts.isIdentifier(elemAccess.argumentExpression)) {
        const argScope = this.getScopeForNode(elemAccess.argumentExpression);
        const argTypeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
            elemAccess.argumentExpression.getText(),
            argScope,
        );
        if (
            !argTypeInfo && !this.isBuiltinObject(elemAccess.argumentExpression)
        ) {
            return `jspp::RuntimeError::throw_unresolved_reference_error(${
                this.getJsVarName(
                    elemAccess.argumentExpression as ts.Identifier,
                )
            })`;
        }
        if (
            argTypeInfo &&
            !argTypeInfo.isParameter &&
            !argTypeInfo.isBuiltin
        ) {
            argText = this.getDerefCode(
                argText,
                this.getJsVarName(elemAccess.argumentExpression),
                argTypeInfo,
            );
        }
    }

    return `${finalExpr}.get_own_property(${argText})`;
}

export function visitBinaryExpression(
    this: CodeGenerator,
    node: ts.BinaryExpression,
    context: VisitContext,
): string {
    const binExpr = node as ts.BinaryExpression;
    const opToken = binExpr.operatorToken;
    let op = opToken.getText();

    const assignmentOperators = [
        ts.SyntaxKind.PlusEqualsToken,
        ts.SyntaxKind.MinusEqualsToken,
        ts.SyntaxKind.AsteriskEqualsToken,
        ts.SyntaxKind.SlashEqualsToken,
        ts.SyntaxKind.PercentEqualsToken,
    ];

    if (assignmentOperators.includes(opToken.kind)) {
        const leftText = this.visit(binExpr.left, context);
        let rightText = this.visit(binExpr.right, context);
        if (ts.isIdentifier(binExpr.right)) {
            const scope = this.getScopeForNode(binExpr.right);
            const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                binExpr.right.getText(),
                scope,
            )!;
            rightText = this.getDerefCode(
                rightText,
                this.getJsVarName(binExpr.right as ts.Identifier),
                typeInfo,
            );
        }

        let target = leftText;
        if (ts.isIdentifier(binExpr.left)) {
            const scope = this.getScopeForNode(binExpr.left);
            const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                binExpr.left.getText(),
                scope,
            )!;
            if (context.derefBeforeAssignment) {
                target = this.getDerefCode(leftText, leftText, typeInfo);
            } else if (typeInfo.needsHeapAllocation) {
                target = `*${leftText}`;
            }
        }
        return `${target} ${op} ${rightText}`;
    }

    if (opToken.kind === ts.SyntaxKind.EqualsToken) {
        const rightText = this.visit(binExpr.right, context);

        if (ts.isPropertyAccessExpression(binExpr.left)) {
            const propAccess = binExpr.left;
            const objExprText = this.visit(propAccess.expression, context);
            const propName = propAccess.name.getText();

            let finalObjExpr = objExprText;
            if (ts.isIdentifier(propAccess.expression)) {
                const scope = this.getScopeForNode(propAccess.expression);
                const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                    propAccess.expression.getText(),
                    scope,
                )!;

                if (typeInfo && !typeInfo.isParameter && !typeInfo.isBuiltin) {
                    finalObjExpr = this.getDerefCode(
                        objExprText,
                        this.getJsVarName(
                            propAccess.expression as ts.Identifier,
                        ),
                        typeInfo,
                    );
                }
            }

            let finalRightText = rightText;
            if (ts.isIdentifier(binExpr.right)) {
                const rightScope = this.getScopeForNode(binExpr.right);
                const rightTypeInfo = this.typeAnalyzer.scopeManager
                    .lookupFromScope(
                        binExpr.right.getText(),
                        rightScope,
                    )!;
                if (
                    rightTypeInfo &&
                    !rightTypeInfo.isParameter &&
                    !rightTypeInfo.isBuiltin
                ) {
                    finalRightText = this.getDerefCode(
                        rightText,
                        this.getJsVarName(binExpr.right as ts.Identifier),
                        rightTypeInfo,
                    );
                }
            }

            return `${finalObjExpr}.set_own_property("${propName}", ${finalRightText})`;
        } else if (ts.isElementAccessExpression(binExpr.left)) {
            const elemAccess = binExpr.left;
            const objExprText = this.visit(elemAccess.expression, context);
            let argText = visitObjectPropertyName.call(
                this,
                elemAccess.argumentExpression as ts.PropertyName,
                { ...context, isBracketNotationPropertyAccess: true },
            );

            let finalObjExpr = objExprText;
            if (ts.isIdentifier(elemAccess.expression)) {
                const scope = this.getScopeForNode(elemAccess.expression);
                const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                    elemAccess.expression.getText(),
                    scope,
                )!;
                if (typeInfo && !typeInfo.isParameter && !typeInfo.isBuiltin) {
                    finalObjExpr = this.getDerefCode(
                        objExprText,
                        this.getJsVarName(
                            elemAccess.expression as ts.Identifier,
                        ),
                        typeInfo,
                    );
                }
            }

            if (ts.isIdentifier(elemAccess.argumentExpression)) {
                const argScope = this.getScopeForNode(
                    elemAccess.argumentExpression,
                );
                const argTypeInfo = this.typeAnalyzer.scopeManager
                    .lookupFromScope(
                        elemAccess.argumentExpression.getText(),
                        argScope,
                    )!;
                if (
                    argTypeInfo &&
                    !argTypeInfo.isParameter &&
                    !argTypeInfo.isBuiltin
                ) {
                    argText = this.getDerefCode(
                        argText,
                        this.getJsVarName(
                            elemAccess.argumentExpression as ts.Identifier,
                        ),
                        argTypeInfo,
                    );
                }
            }

            let finalRightText = rightText;
            if (ts.isIdentifier(binExpr.right)) {
                const rightScope = this.getScopeForNode(binExpr.right);
                const rightTypeInfo = this.typeAnalyzer.scopeManager
                    .lookupFromScope(
                        binExpr.right.getText(),
                        rightScope,
                    )!;
                if (
                    rightTypeInfo &&
                    !rightTypeInfo.isParameter &&
                    !rightTypeInfo.isBuiltin
                ) {
                    finalRightText = this.getDerefCode(
                        rightText,
                        this.getJsVarName(binExpr.right as ts.Identifier),
                        rightTypeInfo,
                    );
                }
            }

            return `${finalObjExpr}.set_own_property(${argText}, ${finalRightText})`;
        }

        const leftText = this.visit(binExpr.left, context);
        const scope = this.getScopeForNode(binExpr.left);
        const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
            (binExpr.left as ts.Identifier).text,
            scope,
        )!;
        if (!typeInfo && !this.isBuiltinObject(binExpr.left as ts.Identifier)) {
            return `jspp::RuntimeError::throw_unresolved_reference_error(${
                this.getJsVarName(
                    binExpr.left as ts.Identifier,
                )
            })`;
        }
        if (typeInfo?.isConst) {
            return `jspp::RuntimeError::throw_immutable_assignment_error()`;
        }
        const target = context.derefBeforeAssignment
            ? this.getDerefCode(leftText, leftText, typeInfo)
            : (typeInfo.needsHeapAllocation ? `*${leftText}` : leftText);
        return `${target} ${op} ${rightText}`;
    }

    const leftText = this.visit(binExpr.left, context);
    const rightText = this.visit(binExpr.right, context);

    let finalLeft = leftText;
    if (ts.isIdentifier(binExpr.left)) {
        const scope = this.getScopeForNode(binExpr.left);
        const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
            binExpr.left.getText(),
            scope,
        );
        if (!typeInfo && !this.isBuiltinObject(binExpr.left as ts.Identifier)) {
            return `jspp::RuntimeError::throw_unresolved_reference_error(${
                this.getJsVarName(
                    binExpr.left as ts.Identifier,
                )
            })`;
        }
        if (typeInfo && !typeInfo.isParameter && !typeInfo.isBuiltin) {
            finalLeft = this.getDerefCode(
                leftText,
                this.getJsVarName(binExpr.left as ts.Identifier),
                typeInfo,
            );
        }
    }

    let finalRight = rightText;
    if (ts.isIdentifier(binExpr.right)) {
        const scope = this.getScopeForNode(binExpr.right);
        const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
            binExpr.right.getText(),
            scope,
        );
        if (
            !typeInfo &&
            !this.isBuiltinObject(binExpr.right as ts.Identifier)
        ) {
            return `jspp::RuntimeError::throw_unresolved_reference_error(${
                this.getJsVarName(
                    binExpr.right as ts.Identifier,
                )
            })`;
        }
        if (typeInfo && !typeInfo.isParameter && !typeInfo.isBuiltin) {
            finalRight = this.getDerefCode(
                rightText,
                this.getJsVarName(binExpr.right as ts.Identifier),
                typeInfo,
            );
        }
    }

    if (opToken.kind === ts.SyntaxKind.EqualsEqualsEqualsToken) {
        return `${finalLeft}.is_strictly_equal_to(${finalRight})`;
    }
    if (opToken.kind === ts.SyntaxKind.EqualsEqualsToken) {
        return `${finalLeft}.is_equal_to(${finalRight})`;
    }
    if (opToken.kind === ts.SyntaxKind.ExclamationEqualsEqualsToken) {
        return `${finalLeft}.not_strictly_equal_to(${finalRight})`;
    }
    if (opToken.kind === ts.SyntaxKind.ExclamationEqualsToken) {
        return `${finalLeft}.not_equal_to(${finalRight})`;
    }
    if (opToken.kind === ts.SyntaxKind.AsteriskAsteriskToken) {
        return `jspp::pow(${finalLeft}, ${finalRight})`;
    }

    if (
        op === "+" ||
        op === "-" ||
        op === "*" ||
        op === "/" ||
        op === "%" ||
        op === "^" ||
        op === "&" ||
        op === "|"
    ) {
        return `(${finalLeft} ${op} ${finalRight})`;
    }
    return `${finalLeft} ${op} ${finalRight}`;
}

export function visitCallExpression(
    this: CodeGenerator,
    node: ts.CallExpression,
    context: VisitContext,
): string {
    const callExpr = node as ts.CallExpression;
    const callee = callExpr.expression;
    const args = callExpr.arguments
        .map((arg) => {
            const argText = this.visit(arg, context);
            if (ts.isIdentifier(arg)) {
                const scope = this.getScopeForNode(arg);
                const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                    arg.text,
                    scope,
                );
                if (!typeInfo) {
                    return `jspp::RuntimeError::throw_unresolved_reference_error(${
                        this.getJsVarName(
                            arg,
                        )
                    })`;
                }
                if (typeInfo && !typeInfo.isParameter && !typeInfo.isBuiltin) {
                    return this.getDerefCode(
                        argText,
                        this.getJsVarName(arg),
                        typeInfo,
                    );
                }
            }
            return argText;
        })
        .join(", ");

    const calleeCode = this.visit(callee, context);
    let derefCallee = calleeCode;
    if (ts.isIdentifier(callee)) {
        const scope = this.getScopeForNode(callee);
        const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
            callee.text,
            scope,
        );
        if (!typeInfo && !this.isBuiltinObject(callee)) {
            return `jspp::RuntimeError::throw_unresolved_reference_error(${
                this.getJsVarName(
                    callee,
                )
            })`;
        }
        if (typeInfo?.isBuiltin) {
            derefCallee = calleeCode;
        } else if (typeInfo) {
            derefCallee = this.getDerefCode(
                calleeCode,
                this.getJsVarName(callee),
                typeInfo,
            );
        }
    }

    let calleeName = "";

    if (ts.isIdentifier(callee) || ts.isPropertyAccessExpression(callee)) {
        calleeName = this.escapeString(callee.getText());
    } else if (
        ts.isParenthesizedExpression(callee) &&
        ts.isFunctionExpression(callee.expression)
    ) {
        const funcExpr = callee.expression as ts.FunctionExpression;
        calleeName = this.escapeString(funcExpr.name?.getText() || "");
    }

    return `${derefCallee}.as_function("${calleeName}")->call({${args}})`;
}

export function visitVoidExpression(
    this: CodeGenerator,
    node: ts.VoidExpression,
    context: VisitContext,
): string {
    const voidExpr = node as ts.VoidExpression;
    const exprText = this.visit(voidExpr.expression, context);
    return `(${exprText}, jspp::AnyValue::make_undefined())`;
}

export function visitTemplateExpression(
    this: CodeGenerator,
    node: ts.TemplateExpression,
    context: VisitContext,
): string {
    const templateExpr = node as ts.TemplateExpression;

    let result = `jspp::AnyValue::make_string("${
        this.escapeString(
            templateExpr.head.text,
        )
    }")`;

    for (const span of templateExpr.templateSpans) {
        const expr = span.expression;
        const exprText = this.visit(expr, context);
        let finalExpr = exprText;

        if (ts.isIdentifier(expr)) {
            const scope = this.getScopeForNode(expr);
            const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                expr.text,
                scope,
            );
            if (!typeInfo && !this.isBuiltinObject(expr)) {
                finalExpr =
                    `jspp::RuntimeError::throw_unresolved_reference_error(${
                        this.getJsVarName(
                            expr as ts.Identifier,
                        )
                    })`;
            } else if (
                typeInfo &&
                !typeInfo.isParameter &&
                !typeInfo.isBuiltin
            ) {
                finalExpr = this.getDerefCode(
                    exprText,
                    this.getJsVarName(expr as ts.Identifier),
                    typeInfo,
                );
            }
        }

        result += ` + (${finalExpr})`;

        if (span.literal.text) {
            result += ` + jspp::AnyValue::make_string("${
                this.escapeString(
                    span.literal.text,
                )
            }")`;
        }
    }
    return result;
}
