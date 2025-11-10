import ts from "typescript";

import { CodeGenerator } from "./";
import type { VisitContext } from "./visitor";

function visitObjectPropertyName(
    this: CodeGenerator,
    node: ts.PropertyName,
    context: VisitContext,
): string {
    if (ts.isNumericLiteral(node)) {
        return context.isPropertyNameAccess
            ? node.getText()
            : `"${node.getText()}"`;
    } else if (ts.isStringLiteral(node)) {
        return `"${
            node.getText().substring(
                1,
                node.getText().length - 1,
            ) // remove trailing "' from original name
        }"`;
    } else if (ts.isComputedPropertyName(node)) {
        let name = ts.isIdentifier(node.expression)
            ? `jspp::Access::deref(${node.expression.getText()},${
                this.getJsVarName(
                    node.expression as ts.Identifier,
                )
            })`
            : this.visit(node.expression, context);
        name += ".to_std_string()";
        return name;
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
            const key = visitObjectPropertyName.call(this, prop.name, context);
            const value = this.visit(prop.initializer, context);
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
        .map((elem) => this.visit(elem, context))
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
        return `${operator}(*${operand})`;
    }
    if (operator === "~") {
        return `${operator}(*${operand})`;
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
    return `(*${operand})${operator}`;
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
        return `jspp::RuntimeError::throw_unresolved_reference(${
            this.getJsVarName(
                propAccess.expression,
            )
        })`;
    }

    let finalExpr = "";
    if (typeInfo && !typeInfo.isParameter && !typeInfo.isBuiltin) {
        finalExpr = `jspp::Access::deref(${exprText}, ${
            this.getJsVarName(
                propAccess.expression as ts.Identifier,
            )
        })`;
    } else {
        finalExpr = exprText;
    }

    return `${finalExpr}["${propName}"]`;
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
        { ...context, isPropertyNameAccess: true },
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
        return `jspp::RuntimeError::throw_unresolved_reference(${
            this.getJsVarName(
                elemAccess.expression as ts.Identifier,
            )
        })`;
    }

    const finalExpr =
        exprTypeInfo && !exprTypeInfo.isParameter && !exprTypeInfo.isBuiltin
            ? `jspp::Access::deref(${exprText}, ${
                this.getJsVarName(
                    elemAccess.expression as ts.Identifier,
                )
            })`
            : exprText;

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
            return `jspp::RuntimeError::throw_unresolved_reference(${
                this.getJsVarName(
                    elemAccess.argumentExpression as ts.Identifier,
                )
            })`;
        }
        if (argTypeInfo && !argTypeInfo.isParameter && !argTypeInfo.isBuiltin) {
            argText = `jspp::Access::deref(${argText}, ${
                this.getJsVarName(
                    elemAccess.argumentExpression as ts.Identifier,
                )
            })`;
        }
    }

    return `${finalExpr}[${argText}]`;
}

export function visitBinaryExpression(
    this: CodeGenerator,
    node: ts.BinaryExpression,
    context: VisitContext,
): string {
    const binExpr = node as ts.BinaryExpression;
    const opToken = binExpr.operatorToken;
    let op = opToken.getText();

    if (
        opToken.kind === ts.SyntaxKind.PlusEqualsToken ||
        opToken.kind === ts.SyntaxKind.MinusEqualsToken ||
        opToken.kind === ts.SyntaxKind.AsteriskEqualsToken ||
        opToken.kind === ts.SyntaxKind.SlashEqualsToken ||
        opToken.kind === ts.SyntaxKind.PercentEqualsToken
    ) {
        const leftText = this.visit(binExpr.left, context);
        const rightText = this.visit(binExpr.right, context);
        return `*${leftText} ${op} ${rightText}`;
    }

    if (opToken.kind === ts.SyntaxKind.EqualsToken) {
        const rightText = this.visit(binExpr.right, context);

        if (ts.isPropertyAccessExpression(binExpr.left)) {
            const propAccess = binExpr.left;
            const objExprText = this.visit(propAccess.expression, context);
            const propName = propAccess.name.getText();

            const scope = this.getScopeForNode(propAccess.expression);
            const typeInfo = ts.isIdentifier(propAccess.expression)
                ? this.typeAnalyzer.scopeManager.lookupFromScope(
                    propAccess.expression.getText(),
                    scope,
                )
                : null;

            const finalObjExpr =
                typeInfo && !typeInfo.isParameter && !typeInfo.isBuiltin
                    ? `jspp::Access::deref(${objExprText}, ${
                        this.getJsVarName(
                            propAccess.expression as ts.Identifier,
                        )
                    })`
                    : objExprText;

            let finalRightText = rightText;
            if (ts.isIdentifier(binExpr.right)) {
                const rightScope = this.getScopeForNode(binExpr.right);
                const rightTypeInfo = this.typeAnalyzer.scopeManager
                    .lookupFromScope(
                        binExpr.right.getText(),
                        rightScope,
                    );
                if (
                    rightTypeInfo &&
                    !rightTypeInfo.isParameter &&
                    !rightTypeInfo.isBuiltin
                ) {
                    finalRightText = `jspp::Access::deref(${rightText}, ${
                        this.getJsVarName(
                            binExpr.right as ts.Identifier,
                        )
                    })`;
                }
            }

            return `${finalObjExpr}["${propName}"] = ${finalRightText}`;
        } else if (ts.isElementAccessExpression(binExpr.left)) {
            const elemAccess = binExpr.left;
            const objExprText = this.visit(elemAccess.expression, context);
            let argText = visitObjectPropertyName.call(
                this,
                elemAccess.argumentExpression as ts.PropertyName,
                { ...context, isPropertyNameAccess: true },
            );

            const scope = this.getScopeForNode(elemAccess.expression);
            const typeInfo = ts.isIdentifier(elemAccess.expression)
                ? this.typeAnalyzer.scopeManager.lookupFromScope(
                    elemAccess.expression.getText(),
                    scope,
                )
                : null;

            const finalObjExpr =
                typeInfo && !typeInfo.isParameter && !typeInfo.isBuiltin
                    ? `jspp::Access::deref(${objExprText}, ${
                        this.getJsVarName(
                            elemAccess.expression as ts.Identifier,
                        )
                    })`
                    : objExprText;

            if (ts.isIdentifier(elemAccess.argumentExpression)) {
                const argScope = this.getScopeForNode(
                    elemAccess.argumentExpression,
                );
                const argTypeInfo = this.typeAnalyzer.scopeManager
                    .lookupFromScope(
                        elemAccess.argumentExpression.getText(),
                        argScope,
                    );
                if (
                    argTypeInfo &&
                    !argTypeInfo.isParameter &&
                    !argTypeInfo.isBuiltin
                ) {
                    argText = `jspp::Access::deref(${argText}, ${
                        this.getJsVarName(
                            elemAccess.argumentExpression as ts.Identifier,
                        )
                    })`;
                }
            }

            let finalRightText = rightText;
            if (ts.isIdentifier(binExpr.right)) {
                const rightScope = this.getScopeForNode(binExpr.right);
                const rightTypeInfo = this.typeAnalyzer.scopeManager
                    .lookupFromScope(
                        binExpr.right.getText(),
                        rightScope,
                    );
                if (
                    rightTypeInfo &&
                    !rightTypeInfo.isParameter &&
                    !rightTypeInfo.isBuiltin
                ) {
                    finalRightText = `jspp::Access::deref(${rightText}, ${
                        this.getJsVarName(
                            binExpr.right as ts.Identifier,
                        )
                    })`;
                }
            }

            return `${finalObjExpr}[${argText}] = ${finalRightText}`;
        }

        const leftText = this.visit(binExpr.left, context);
        const scope = this.getScopeForNode(binExpr.left);
        const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
            (binExpr.left as ts.Identifier).text,
            scope,
        );
        if (!typeInfo && !this.isBuiltinObject(binExpr.left as ts.Identifier)) {
            return `jspp::RuntimeError::throw_unresolved_reference(${
                this.getJsVarName(
                    binExpr.left as ts.Identifier,
                )
            })`;
        }
        if (typeInfo?.isConst) {
            return `jspp::RuntimeError::throw_immutable_assignment()`;
        }
        return `*${leftText} ${op} ${rightText}`;
    }

    const leftText = this.visit(binExpr.left, context);
    const rightText = this.visit(binExpr.right, context);

    const leftIsIdentifier = ts.isIdentifier(binExpr.left);
    const rightIsIdentifier = ts.isIdentifier(binExpr.right);

    const scope = this.getScopeForNode(node);
    const leftTypeInfo = leftIsIdentifier
        ? this.typeAnalyzer.scopeManager.lookupFromScope(
            binExpr.left.getText(),
            scope,
        )
        : null;
    const rightTypeInfo = rightIsIdentifier
        ? this.typeAnalyzer.scopeManager.lookupFromScope(
            binExpr.right.getText(),
            scope,
        )
        : null;

    const finalLeft =
        leftIsIdentifier && leftTypeInfo && !leftTypeInfo.isParameter &&
            !leftTypeInfo.isBuiltin
            ? `jspp::Access::deref(${leftText}, ${
                this.getJsVarName(
                    binExpr.left as ts.Identifier,
                )
            })`
            : leftText;
    const finalRight =
        rightIsIdentifier && rightTypeInfo && !rightTypeInfo.isParameter &&
            !rightTypeInfo.isBuiltin
            ? `jspp::Access::deref(${rightText}, ${
                this.getJsVarName(
                    binExpr.right as ts.Identifier,
                )
            })`
            : rightText;

    if (
        leftIsIdentifier && !leftTypeInfo &&
        !this.isBuiltinObject(binExpr.left as ts.Identifier)
    ) {
        return `jspp::RuntimeError::throw_unresolved_reference(${
            this.getJsVarName(
                binExpr.left as ts.Identifier,
            )
        })`;
    }
    if (
        rightIsIdentifier && !rightTypeInfo &&
        !this.isBuiltinObject(binExpr.right as ts.Identifier)
    ) {
        return `jspp::RuntimeError::throw_unresolved_reference(${
            this.getJsVarName(
                binExpr.right as ts.Identifier,
            )
        })`;
    }

    if (opToken.kind === ts.SyntaxKind.EqualsEqualsEqualsToken) {
        return `${finalLeft}.is_strictly_equal_to_primitive(${finalRight})`;
    }
    if (opToken.kind === ts.SyntaxKind.EqualsEqualsToken) {
        return `${finalLeft}.is_equal_to_primitive(${finalRight})`;
    }
    if (opToken.kind === ts.SyntaxKind.ExclamationEqualsEqualsToken) {
        return `${finalLeft}.not_strictly_equal_to_primitive(${finalRight})`;
    }
    if (opToken.kind === ts.SyntaxKind.ExclamationEqualsToken) {
        return `${finalLeft}.not_equal_to_primitive(${finalRight})`;
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
    const calleeName = this.escapeString(callee.getText());
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
                    return `jspp::RuntimeError::throw_unresolved_reference(${
                        this.getJsVarName(
                            arg,
                        )
                    })`;
                }
                if (typeInfo && !typeInfo.isParameter && !typeInfo.isBuiltin) {
                    return `jspp::Access::deref(${argText}, ${
                        this.getJsVarName(
                            arg,
                        )
                    })`;
                }
            }
            return argText;
        })
        .join(", ");

    const calleeCode = this.visit(callee, context);
    let derefCallee;
    if (ts.isIdentifier(callee)) {
        const scope = this.getScopeForNode(callee);
        const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
            callee.text,
            scope,
        );
        if (!typeInfo && !this.isBuiltinObject(callee)) {
            return `jspp::RuntimeError::throw_unresolved_reference(${
                this.getJsVarName(
                    callee,
                )
            })`;
        }
        if (typeInfo?.isBuiltin) {
            derefCallee = calleeCode;
        } else {
            derefCallee = `jspp::Access::deref(${calleeCode}, ${
                this.getJsVarName(
                    callee,
                )
            })`;
        }
    } else {
        derefCallee = calleeCode;
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
                finalExpr = `jspp::RuntimeError::throw_unresolved_reference(${
                    this.getJsVarName(
                        expr as ts.Identifier,
                    )
                })`;
            } else if (
                typeInfo &&
                !typeInfo.isParameter &&
                !typeInfo.isBuiltin
            ) {
                finalExpr = `jspp::Access::deref(${exprText}, ${
                    this.getJsVarName(
                        expr as ts.Identifier,
                    )
                })`;
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
