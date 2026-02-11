import ts from "typescript";

import type { TypeInfo } from "../../analysis/typeAnalyzer.js";
import { constants } from "../constants.js";
import { CompilerError } from "../error.js";
import { CodeGenerator } from "./index.js";
import type { VisitContext } from "./visitor.js";

export function visitObjectPropertyName(
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
                context,
                typeInfo,
            );
        }
        return propName;
    }
    if (context.isBracketNotationPropertyAccess) {
        return this.visit(node, context);
    }
    return `"${node.getText()}"`;
}

export function visitObjectLiteralExpression(
    this: CodeGenerator,
    node: ts.ObjectLiteralExpression,
    context: VisitContext,
): string {
    const obj = node as ts.ObjectLiteralExpression;
    const objVar = this.generateUniqueName(
        "__obj_",
        this.getDeclaredSymbols(node),
    );

    if (
        !obj.properties.some((prop) =>
            ts.isPropertyAssignment(prop) ||
            ts.isShorthandPropertyAssignment(prop) ||
            ts.isMethodDeclaration(prop) || ts.isGetAccessor(prop) ||
            ts.isSetAccessor(prop) || ts.isSpreadAssignment(prop)
        )
    ) {
        // Empty object
        return `jspp::AnyValue::make_object_with_proto({}, ::Object.get_own_property("prototype"))`;
    }

    let code = `([&]() {\n`;
    code +=
        `${this.indent()}  auto ${objVar} = jspp::AnyValue::make_object_with_proto({}, ::Object.get_own_property("prototype"));\n`;

    this.indentationLevel++;

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
                        context,
                        typeInfo,
                    );
                }
            }

            code +=
                `${this.indent()}${objVar}.define_data_property(${key}, ${value});\n`;
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
                    context,
                    typeInfo,
                );
            }

            code +=
                `${this.indent()}${objVar}.define_data_property(${key}, ${value});\n`;
        } else if (ts.isMethodDeclaration(prop)) {
            const key = visitObjectPropertyName.call(this, prop.name, {
                ...context,
                isObjectLiteralExpression: true,
            });
            const lambda = this.generateWrappedLambda(
                this.generateLambdaComponents(prop, {
                    ...context,
                    isInsideFunction: true,
                }),
            );
            code +=
                `${this.indent()}${objVar}.define_data_property(${key}, ${lambda});\n`;
        } else if (ts.isGetAccessor(prop)) {
            const key = visitObjectPropertyName.call(this, prop.name, {
                ...context,
                isObjectLiteralExpression: true,
            });
            const lambda = this.generateWrappedLambda(
                this.generateLambdaComponents(prop, {
                    ...context,
                    isInsideFunction: true,
                }),
            );
            code +=
                `${this.indent()}${objVar}.define_getter(${key}, ${lambda});\n`;
        } else if (ts.isSetAccessor(prop)) {
            const key = visitObjectPropertyName.call(this, prop.name, {
                ...context,
                isObjectLiteralExpression: true,
            });
            const lambda = this.generateWrappedLambda(
                this.generateLambdaComponents(prop, {
                    ...context,
                    isInsideFunction: true,
                }),
            );
            code +=
                `${this.indent()}${objVar}.define_setter(${key}, ${lambda});\n`;
        } else if (ts.isSpreadAssignment(prop)) {
            let spreadExpr = this.visit(prop.expression, context);
            if (ts.isIdentifier(prop.expression)) {
                const scope = this.getScopeForNode(prop.expression);
                const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                    prop.expression.text,
                    scope,
                )!;
                if (typeInfo && !typeInfo.isBuiltin && !typeInfo.isParameter) {
                    spreadExpr = this.getDerefCode(
                        spreadExpr,
                        this.getJsVarName(prop.expression),
                        context,
                        typeInfo,
                    );
                }
            }
            code +=
                `${this.indent()}jspp::Access::spread_object(${objVar}, ${spreadExpr});\n`;
        }
    }

    this.indentationLevel--;
    code += `${this.indent()}  return ${objVar};\n${this.indent()}})()`;

    return code;
}

export function visitArrayLiteralExpression(
    this: CodeGenerator,
    node: ts.ArrayLiteralExpression,
    context: VisitContext,
): string {
    const hasSpread = node.elements.some(ts.isSpreadElement);

    if (!hasSpread) {
        const elements = (node as ts.ArrayLiteralExpression).elements
            .map((elem) => {
                let elemText = this.visit(elem, context);
                if (ts.isIdentifier(elem)) {
                    const scope = this.getScopeForNode(elem);
                    const typeInfo = this.typeAnalyzer.scopeManager
                        .lookupFromScope(
                            elem.text,
                            scope,
                        )!;
                    if (
                        typeInfo && !typeInfo.isBuiltin && !typeInfo.isParameter
                    ) {
                        elemText = this.getDerefCode(
                            elemText,
                            this.getJsVarName(elem),
                            context,
                            typeInfo,
                        );
                    }
                }
                if (ts.isOmittedExpression(elem)) {
                    elemText = "jspp::Constants::UNINITIALIZED";
                }
                return elemText;
            });
        const elementsJoined = elements.join(", ");
        const elementsSpan = elements.length > 0
            ? `std::span<const jspp::AnyValue>((const jspp::AnyValue[]){${elementsJoined}}, ${elements.length})`
            : "std::span<const jspp::AnyValue>{}";
        return `jspp::AnyValue::make_array_with_proto(${elementsSpan}, ::Array.get_own_property("prototype"))`;
    }

    const arrVar = this.generateUniqueName(
        "__arr_",
        this.getDeclaredSymbols(node),
    );
    let code = `([&]() {\n`;
    code += `${this.indent()}  std::vector<jspp::AnyValue> ${arrVar};\n`;
    this.indentationLevel++;

    for (const elem of node.elements) {
        if (ts.isSpreadElement(elem)) {
            let spreadExpr = this.visit(elem.expression, context);
            if (ts.isIdentifier(elem.expression)) {
                const scope = this.getScopeForNode(elem.expression);
                const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                    elem.expression.text,
                    scope,
                )!;
                if (typeInfo && !typeInfo.isBuiltin && !typeInfo.isParameter) {
                    spreadExpr = this.getDerefCode(
                        spreadExpr,
                        this.getJsVarName(elem.expression),
                        context,
                        typeInfo,
                    );
                }
            }
            code +=
                `${this.indent()}jspp::Access::spread_array(${arrVar}, ${spreadExpr});\n`;
        } else {
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
                        context,
                        typeInfo,
                    );
                }
            }
            if (ts.isOmittedExpression(elem)) {
                elemText = "jspp::Constants::UNINITIALIZED";
            }
            code += `${this.indent()}${arrVar}.push_back(${elemText});\n`;
        }
    }

    this.indentationLevel--;
    code +=
        `${this.indent()}  return jspp::AnyValue::make_array_with_proto(std::move(${arrVar}), ::Array.get_own_property("prototype"));\n`;
    code += `${this.indent()}})()`;

    return code;
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
                target = this.getDerefCode(operand, operand, context, typeInfo);
            } else if (typeInfo.needsHeapAllocation) {
                target = `*${operand}`;
            }
        }
        return `${operator}(${target})`;
    }

    if (operator === "+") {
        return `jspp::plus(${operand})`;
    }
    if (operator === "-") {
        return `jspp::negate(${operand})`;
    }
    if (operator === "!") {
        return `jspp::logical_not(${operand})`;
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
                target = this.getDerefCode(operand, operand, context, typeInfo);
            } else if (typeInfo.needsHeapAllocation) {
                target = `*${operand}`;
            }
        }
        return `jspp::bitwise_not(${target})`;
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
            target = this.getDerefCode(operand, operand, context, typeInfo);
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

    if (propAccess.expression.kind === ts.SyntaxKind.SuperKeyword) {
        if (!context.superClassVar) {
            throw new CompilerError(
                "super.prop accessed but no super class variable found in context",
                propAccess.expression,
                "SyntaxError",
            );
        }
        const propName = propAccess.name.getText();
        return `jspp::AnyValue::resolve_property_for_read((${context.superClassVar}).get_own_property("prototype").get_own_property("${propName}"), ${this.globalThisVar}, "${
            this.escapeString(propName)
        }")`;
    }

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
        return `jspp::Exception::throw_unresolved_reference(${
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
            context,
            typeInfo,
        );
    }

    if (propAccess.questionDotToken) {
        return `jspp::Access::optional_get_property(${finalExpr}, "${propName}")`;
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
        return `jspp::Exception::throw_unresolved_reference(${
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
            context,
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
            return `jspp::Exception::throw_unresolved_reference(${
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
                context,
                argTypeInfo,
            );
        }
    }

    if (elemAccess.questionDotToken) {
        return `jspp::Access::optional_get_element(${finalExpr}, ${argText})`;
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
    const op = opToken.getText();
    const visitContext: VisitContext = {
        ...context,
        supportedNativeLiterals: undefined,
    };

    const assignmentOperators = [
        ts.SyntaxKind.PlusEqualsToken,
        ts.SyntaxKind.MinusEqualsToken,
        ts.SyntaxKind.AsteriskEqualsToken,
        ts.SyntaxKind.SlashEqualsToken,
        ts.SyntaxKind.PercentEqualsToken,
        ts.SyntaxKind.AsteriskAsteriskEqualsToken,
        ts.SyntaxKind.LessThanLessThanEqualsToken,
        ts.SyntaxKind.GreaterThanGreaterThanEqualsToken,
        ts.SyntaxKind.GreaterThanGreaterThanGreaterThanEqualsToken,
        ts.SyntaxKind.AmpersandEqualsToken,
        ts.SyntaxKind.BarEqualsToken,
        ts.SyntaxKind.CaretEqualsToken,
        ts.SyntaxKind.AmpersandAmpersandEqualsToken,
        ts.SyntaxKind.BarBarEqualsToken,
        ts.SyntaxKind.QuestionQuestionEqualsToken,
    ];
    if (assignmentOperators.includes(opToken.kind)) {
        if (
            opToken.kind ===
                ts.SyntaxKind.GreaterThanGreaterThanGreaterThanEqualsToken
        ) {
            const leftText = this.visit(binExpr.left, visitContext);
            const rightText = this.visit(binExpr.right, visitContext);
            let target = leftText;
            if (ts.isIdentifier(binExpr.left)) {
                const scope = this.getScopeForNode(binExpr.left);
                const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                    binExpr.left.text,
                    scope,
                )!;
                target = typeInfo.needsHeapAllocation
                    ? `*${leftText}`
                    : leftText;
                return `${target} = jspp::unsigned_right_shift(${target}, ${rightText})`;
            }
        }
        if (opToken.kind === ts.SyntaxKind.AsteriskAsteriskEqualsToken) {
            const leftText = this.visit(binExpr.left, visitContext);
            const rightText = this.visit(binExpr.right, visitContext);
            let target = leftText;
            if (ts.isIdentifier(binExpr.left)) {
                const scope = this.getScopeForNode(binExpr.left);
                const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                    binExpr.left.text,
                    scope,
                )!;
                target = typeInfo.needsHeapAllocation
                    ? `*${leftText}`
                    : leftText;
                return `${target} = jspp::pow(${target}, ${rightText})`;
            }
            // For complex LHS, we need a different approach, but this is a start.
        }
        if (
            opToken.kind === ts.SyntaxKind.AmpersandAmpersandEqualsToken ||
            opToken.kind === ts.SyntaxKind.BarBarEqualsToken ||
            opToken.kind === ts.SyntaxKind.QuestionQuestionEqualsToken
        ) {
            const leftText = this.visit(binExpr.left, visitContext);
            const rightText = this.visit(binExpr.right, visitContext);
            let target = leftText;
            if (ts.isIdentifier(binExpr.left)) {
                const scope = this.getScopeForNode(binExpr.left);
                const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                    binExpr.left.text,
                    scope,
                )!;
                target = typeInfo.needsHeapAllocation
                    ? `*${leftText}`
                    : leftText;

                if (
                    opToken.kind === ts.SyntaxKind.AmpersandAmpersandEqualsToken
                ) {
                    return `jspp::logical_and_assign(${target}, ${rightText})`;
                } else if (opToken.kind === ts.SyntaxKind.BarBarEqualsToken) {
                    return `jspp::logical_or_assign(${target}, ${rightText})`;
                } else {
                    return `jspp::nullish_coalesce_assign(${target}, ${rightText})`;
                }
            }
        }

        const leftText = this.visit(binExpr.left, visitContext);
        let rightText = ts.isNumericLiteral(binExpr.right)
            ? binExpr.right.getText()
            : this.visit(binExpr.right, visitContext);

        if (ts.isIdentifier(binExpr.right)) {
            const scope = this.getScopeForNode(binExpr.right);
            const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                binExpr.right.getText(),
                scope,
            )!;
            rightText = this.getDerefCode(
                rightText,
                this.getJsVarName(binExpr.right as ts.Identifier),
                visitContext,
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
                target = this.getDerefCode(
                    leftText,
                    leftText,
                    visitContext,
                    typeInfo,
                );
            } else if (typeInfo.needsHeapAllocation) {
                target = `*${leftText}`;
            }
        }
        return `${target} ${op} ${rightText}`;
    }

    // Assignment expression `a = 1`
    if (opToken.kind === ts.SyntaxKind.EqualsToken) {
        let rightText = this.visit(binExpr.right, visitContext);

        if (ts.isPropertyAccessExpression(binExpr.left)) {
            const propAccess = binExpr.left;

            if (propAccess.expression.kind === ts.SyntaxKind.SuperKeyword) {
                const propName = propAccess.name.getText();
                let finalRightText = rightText;
                // Deref right text logic...
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
                            visitContext,
                            rightTypeInfo,
                        );
                    }
                }
                // Approximate super assignment as setting property on 'this'
                return `(${this.globalThisVar}).set_own_property("${propName}", ${finalRightText})`;
            }

            const objExprText = this.visit(propAccess.expression, visitContext);
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
                        context,
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
                        context,
                        rightTypeInfo,
                    );
                }
            }

            return `${finalObjExpr}.set_own_property("${propName}", ${finalRightText})`;
        } else if (ts.isElementAccessExpression(binExpr.left)) {
            const elemAccess = binExpr.left;
            const objExprText = this.visit(elemAccess.expression, visitContext);
            let argText = visitObjectPropertyName.call(
                this,
                elemAccess.argumentExpression as ts.PropertyName,
                { ...visitContext, isBracketNotationPropertyAccess: true },
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
                        visitContext,
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
                        visitContext,
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
                        visitContext,
                        rightTypeInfo,
                    );
                }
            }

            return `${finalObjExpr}.set_own_property(${argText}, ${finalRightText})`;
        }

        const leftText = this.visit(binExpr.left, visitContext);
        const scope = this.getScopeForNode(binExpr.left);
        const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
            (binExpr.left as ts.Identifier).text,
            scope,
        )!;
        if (!typeInfo && !this.isBuiltinObject(binExpr.left as ts.Identifier)) {
            return `jspp::Exception::throw_unresolved_reference(${
                this.getJsVarName(
                    binExpr.left as ts.Identifier,
                )
            })`;
        }
        if (typeInfo?.isConst) {
            return `jspp::Exception::throw_immutable_assignment()`;
        }
        if (ts.isNumericLiteral(binExpr.right)) {
            rightText = binExpr.right.getText();
        }
        const target = context.derefBeforeAssignment
            ? this.getDerefCode(leftText, leftText, visitContext, typeInfo)
            : (typeInfo.needsHeapAllocation ? `*${leftText}` : leftText);

        // Update scope symbols on variable re-assignment
        // Reset features
        if (ts.isIdentifier(binExpr.left)) {
            if (!ts.isFunctionDeclaration(binExpr.right)) {
                if (context.localScopeSymbols.has(binExpr.left.text)) {
                    context.localScopeSymbols.set(binExpr.left.text, {
                        features: {},
                    });
                } else if (context.globalScopeSymbols.has(binExpr.left.text)) {
                    context.globalScopeSymbols.set(binExpr.left.text, {
                        features: {},
                    });
                }
            }
        }

        return `${target} ${op} ${rightText}`;
    }

    const leftText = this.visit(binExpr.left, visitContext);
    const rightText = this.visit(binExpr.right, visitContext);

    // Generate lhs and rhs code
    let finalLeft = leftText;
    if (ts.isIdentifier(binExpr.left)) {
        const scope = this.getScopeForNode(binExpr.left);
        const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
            binExpr.left.getText(),
            scope,
        );
        if (!typeInfo && !this.isBuiltinObject(binExpr.left as ts.Identifier)) {
            return `jspp::Exception::throw_unresolved_reference(${
                this.getJsVarName(
                    binExpr.left as ts.Identifier,
                )
            })`;
        }
        if (typeInfo && !typeInfo.isParameter && !typeInfo.isBuiltin) {
            finalLeft = this.getDerefCode(
                leftText,
                this.getJsVarName(binExpr.left as ts.Identifier),
                visitContext,
                typeInfo,
            );
        }
        // Number optimizations
        if (typeInfo && typeInfo.type === "number") {
            finalLeft = `${finalLeft}.as_double()`;
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
            return `jspp::Exception::throw_unresolved_reference(${
                this.getJsVarName(
                    binExpr.right as ts.Identifier,
                )
            })`;
        }
        if (typeInfo && !typeInfo.isParameter && !typeInfo.isBuiltin) {
            finalRight = this.getDerefCode(
                rightText,
                this.getJsVarName(binExpr.right as ts.Identifier),
                visitContext,
                typeInfo,
            );
        }
        // Number optimizations
        if (typeInfo && typeInfo.type === "number") {
            finalRight = `${finalRight}.as_double()`;
        }
    }

    if (opToken.kind === ts.SyntaxKind.InKeyword) {
        return `jspp::Access::in(${finalLeft}, ${finalRight})`;
    }
    if (opToken.kind === ts.SyntaxKind.InstanceOfKeyword) {
        return `jspp::Access::instance_of(${finalLeft}, ${finalRight})`;
    }
    if (opToken.kind === ts.SyntaxKind.AmpersandAmpersandToken) {
        return `jspp::logical_and(${finalLeft}, ${finalRight})`;
    }
    if (opToken.kind === ts.SyntaxKind.BarBarToken) {
        return `jspp::logical_or(${finalLeft}, ${finalRight})`;
    }
    if (opToken.kind === ts.SyntaxKind.QuestionQuestionToken) {
        return `jspp::nullish_coalesce(${finalLeft}, ${finalRight})`;
    }

    const isLiteral = (n: ts.Node) => ts.isNumericLiteral(n);
    const supportsNativeBoolean =
        context.supportedNativeLiterals?.includes("boolean") || false;

    // Native values for lhs and rhs
    const literalLeft = isLiteral(binExpr.left)
        ? binExpr.left.getText()
        : finalLeft;
    const literalRight = isLiteral(binExpr.right)
        ? binExpr.right.getText()
        : finalRight;

    // Operations that returns boolean should return the native boolean if supported
    if (
        constants.booleanOperators.includes(opToken.kind) &&
        supportsNativeBoolean
    ) {
        if (opToken.kind === ts.SyntaxKind.EqualsEqualsEqualsToken) {
            return `jspp::is_strictly_equal_to_primitive(${literalLeft}, ${literalRight})`;
        }
        if (opToken.kind === ts.SyntaxKind.EqualsEqualsToken) {
            return `jspp::is_equal_to_primitive(${literalLeft}, ${literalRight})`;
        }
        if (opToken.kind === ts.SyntaxKind.ExclamationEqualsEqualsToken) {
            return `!jspp::is_strictly_equal_to_primitive(${literalLeft}, ${literalRight})`;
        }
        if (opToken.kind === ts.SyntaxKind.ExclamationEqualsToken) {
            return `!jspp::is_equal_to_primitive(${literalLeft}, ${literalRight})`;
        }

        let funcName = "";
        if (opToken.kind === ts.SyntaxKind.LessThanToken) {
            funcName = "jspp::less_than_primitive";
        }
        if (opToken.kind === ts.SyntaxKind.LessThanEqualsToken) {
            funcName = "jspp::less_than_or_equal_primitive";
        }
        if (opToken.kind === ts.SyntaxKind.GreaterThanToken) {
            funcName = "jspp::greater_than_primitive";
        }
        if (opToken.kind === ts.SyntaxKind.GreaterThanEqualsToken) {
            funcName = "jspp::greater_than_or_equal_primitive";
        }

        // For C++ primitive literals, standard operators are fine if they map directly,
        // but we are safe using our functions (which handle doubles correctly).
        // Actually, for pure numeric literals like "1 < 2", we can leave it as is if we want optimization,
        // but consistency is safer.
        // Let's stick to valid C++ syntax for literals if possible to avoid overhead?
        // jspp::less_than(1, 2) works.
        return `${funcName}(${literalLeft}, ${literalRight})`;
    }

    // Return boxed value
    if (opToken.kind === ts.SyntaxKind.EqualsEqualsEqualsToken) {
        return `jspp::is_strictly_equal_to(${literalLeft}, ${literalRight})`;
    }
    if (opToken.kind === ts.SyntaxKind.EqualsEqualsToken) {
        return `jspp::is_equal_to(${literalLeft}, ${literalRight})`;
    }
    if (opToken.kind === ts.SyntaxKind.ExclamationEqualsEqualsToken) {
        return `jspp::not_strictly_equal_to(${literalLeft}, ${literalRight})`;
    }
    if (opToken.kind === ts.SyntaxKind.ExclamationEqualsToken) {
        return `jspp::not_equal_to(${literalLeft}, ${literalRight})`;
    }
    if (opToken.kind === ts.SyntaxKind.AsteriskAsteriskToken) {
        return `jspp::pow(${literalLeft}, ${literalRight})`;
    }
    if (opToken.kind === ts.SyntaxKind.GreaterThanGreaterThanGreaterThanToken) {
        return `jspp::unsigned_right_shift(${literalLeft}, ${literalRight})`;
    }

    // For other arithmetic and bitwise operations, use native operations if possible
    switch (op) {
        case "+":
            return `jspp::add(${literalLeft}, ${literalRight})`;
        case "-":
            return `jspp::sub(${literalLeft}, ${literalRight})`;
        case "*":
            return `jspp::mul(${literalLeft}, ${literalRight})`;
        case "/":
            return `jspp::div(${literalLeft}, ${literalRight})`;
        case "%":
            return `jspp::mod(${literalLeft}, ${literalRight})`;
        case "^":
            return `jspp::bitwise_xor(${literalLeft}, ${literalRight})`;
        case "&":
            return `jspp::bitwise_and(${literalLeft}, ${literalRight})`;
        case "|":
            return `jspp::bitwise_or(${literalLeft}, ${literalRight})`;
        case "<<":
            return `jspp::left_shift(${literalLeft}, ${literalRight})`;
        case ">>":
            return `jspp::right_shift(${literalLeft}, ${literalRight})`;
        case "<":
            return `jspp::less_than(${literalLeft}, ${literalRight})`;
        case ">":
            return `jspp::greater_than(${literalLeft}, ${literalRight})`;
        case "<=":
            return `jspp::less_than_or_equal(${literalLeft}, ${literalRight})`;
        case ">=":
            return `jspp::greater_than_or_equal(${literalLeft}, ${literalRight})`;
    }

    return `/* Unhandled Operator: ${finalLeft} ${op} ${finalRight} */`; // Default fallback
}

export function visitConditionalExpression(
    this: CodeGenerator,
    node: ts.ConditionalExpression,
    context: VisitContext,
): string {
    const condExpr = node as ts.ConditionalExpression;
    const isBinaryExpression = ts.isBinaryExpression(condExpr.condition) &&
        constants.booleanOperators.includes(
            condExpr.condition.operatorToken.kind,
        );

    const condition = this.visit(condExpr.condition, {
        ...context,
        supportedNativeLiterals: isBinaryExpression ? ["boolean"] : undefined,
    });
    const whenTrueStmt = this.visit(condExpr.whenTrue, {
        ...context,
        isFunctionBody: false,
    });
    const whenFalseStmt = this.visit(condExpr.whenFalse, {
        ...context,
        isFunctionBody: false,
    });

    if (isBinaryExpression) {
        return `${condition} ? ${whenTrueStmt} : ${whenFalseStmt}`;
    }
    return `jspp::is_truthy(${condition}) ? ${whenTrueStmt} : ${whenFalseStmt}`;
}

export function visitCallExpression(
    this: CodeGenerator,
    node: ts.CallExpression,
    context: VisitContext,
): string {
    const callExpr = node as ts.CallExpression;
    const callee = callExpr.expression;
    const hasSpread = callExpr.arguments.some(ts.isSpreadElement);

    if (callee.kind === ts.SyntaxKind.SuperKeyword) {
        if (!context.superClassVar) {
            throw new CompilerError(
                "super() called but no super class variable found in context",
                callee,
                "SyntaxError",
            );
        }
        if (!hasSpread) {
            const args = callExpr.arguments.map((arg) =>
                this.visit(arg, context)
            )
                .join(", ");
            return `(${context.superClassVar}).call(${this.globalThisVar}, (const jspp::AnyValue[]){${args}}, "super")`;
        } else {
            const argsVar = this.generateUniqueName(
                "__args_",
                this.getDeclaredSymbols(node),
            );
            let code = `([&]() {\n`;
            code +=
                `${this.indent()}  std::vector<jspp::AnyValue> ${argsVar};\n`;
            this.indentationLevel++;
            for (const arg of callExpr.arguments) {
                if (ts.isSpreadElement(arg)) {
                    let spreadExpr = this.visit(arg.expression, context);
                    if (ts.isIdentifier(arg.expression)) {
                        const scope = this.getScopeForNode(arg.expression);
                        const typeInfo = this.typeAnalyzer.scopeManager
                            .lookupFromScope(arg.expression.text, scope)!;
                        spreadExpr = this.getDerefCode(
                            spreadExpr,
                            this.getJsVarName(arg.expression),
                            context,
                            typeInfo,
                        );
                    }
                    code +=
                        `${this.indent()}jspp::Access::spread_array(${argsVar}, ${spreadExpr});\n`;
                } else {
                    let argText = this.visit(arg, context);
                    if (ts.isIdentifier(arg)) {
                        const scope = this.getScopeForNode(arg);
                        const typeInfo = this.typeAnalyzer.scopeManager
                            .lookupFromScope(arg.text, scope)!;
                        argText = this.getDerefCode(
                            argText,
                            this.getJsVarName(arg),
                            context,
                            typeInfo,
                        );
                    }
                    code +=
                        `${this.indent()}${argsVar}.push_back(${argText});\n`;
                }
            }
            code +=
                `${this.indent()}  return (${context.superClassVar}).call(${this.globalThisVar}, ${argsVar}, "super");\n`;
            this.indentationLevel--;
            code += `${this.indent()}})()`;
            return code;
        }
    }

    const generateArgsSpan = (args: ts.NodeArray<ts.Expression>) => {
        const argsArray = args
            .map((arg) => {
                const argText = this.visit(arg, context);
                if (ts.isIdentifier(arg)) {
                    const scope = this.getScopeForNode(arg);
                    const typeInfo = this.typeAnalyzer.scopeManager
                        .lookupFromScope(
                            arg.text,
                            scope,
                        );
                    if (!typeInfo) {
                        return `jspp::Exception::throw_unresolved_reference(${
                            this.getJsVarName(
                                arg,
                            )
                        })`;
                    }
                    if (typeInfo && !typeInfo.isBuiltin) {
                        return this.getDerefCode(
                            argText,
                            this.getJsVarName(arg),
                            context,
                            typeInfo,
                        );
                    }
                }
                return argText;
            });
        const argsJoined = argsArray.join(", ");
        const argsCount = argsArray.length;
        return argsCount > 0
            ? `std::span<const jspp::AnyValue>((const jspp::AnyValue[]){${argsJoined}}, ${argsCount})`
            : "std::span<const jspp::AnyValue>{}";
    };

    const generateArgsVectorBuilder = (
        args: ts.NodeArray<ts.Expression>,
        argsVar: string,
    ) => {
        let code = `${this.indent()}std::vector<jspp::AnyValue> ${argsVar};\n`;
        for (const arg of args) {
            if (ts.isSpreadElement(arg)) {
                let spreadExpr = this.visit(arg.expression, context);
                if (ts.isIdentifier(arg.expression)) {
                    const scope = this.getScopeForNode(arg.expression);
                    const typeInfo = this.typeAnalyzer.scopeManager
                        .lookupFromScope(arg.expression.text, scope)!;
                    if (
                        typeInfo && !typeInfo.isBuiltin && !typeInfo.isParameter
                    ) {
                        spreadExpr = this.getDerefCode(
                            spreadExpr,
                            this.getJsVarName(arg.expression),
                            context,
                            typeInfo,
                        );
                    }
                }
                code +=
                    `${this.indent()}jspp::Access::spread_array(${argsVar}, ${spreadExpr});\n`;
            } else {
                let argText = this.visit(arg, context);
                if (ts.isIdentifier(arg)) {
                    const scope = this.getScopeForNode(arg);
                    const typeInfo = this.typeAnalyzer.scopeManager
                        .lookupFromScope(arg.text, scope)!;
                    if (
                        typeInfo && !typeInfo.isBuiltin && !typeInfo.isParameter
                    ) {
                        argText = this.getDerefCode(
                            argText,
                            this.getJsVarName(arg),
                            context,
                            typeInfo,
                        );
                    }
                }
                code += `${this.indent()}${argsVar}.push_back(${argText});\n`;
            }
        }
        return code;
    };

    if (ts.isPropertyAccessExpression(callee)) {
        const propAccess = callee as ts.PropertyAccessExpression;

        if (propAccess.expression.kind === ts.SyntaxKind.SuperKeyword) {
            if (!context.superClassVar) {
                throw new CompilerError(
                    "super.method() called but no super class variable found in context",
                    propAccess.expression,
                    "SyntaxError",
                );
            }
            const propName = propAccess.name.getText();
            if (!hasSpread) {
                const argsSpan = generateArgsSpan(callExpr.arguments);
                return `(${context.superClassVar}).get_own_property("prototype").get_own_property("${propName}").call(${this.globalThisVar}, ${argsSpan}, "${
                    this.escapeString(propName)
                }")`;
            } else {
                const argsVar = this.generateUniqueName(
                    "__args_",
                    this.getDeclaredSymbols(node),
                );
                let code = `([&]() {\n`;
                this.indentationLevel++;
                code += generateArgsVectorBuilder(callExpr.arguments, argsVar);
                code +=
                    `${this.indent()}return (${context.superClassVar}).get_own_property("prototype").get_own_property("${propName}").call(${this.globalThisVar}, ${argsVar}, "${
                        this.escapeString(propName)
                    }");\n`;
                this.indentationLevel--;
                code += `${this.indent()}})()`;
                return code;
            }
        }

        const objExpr = propAccess.expression;
        const propName = propAccess.name.getText();
        const objCode = this.visit(objExpr, context);

        let derefObj = objCode;
        if (ts.isIdentifier(objExpr)) {
            const scope = this.getScopeForNode(objExpr);
            const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                objExpr.getText(),
                scope,
            );
            if (!typeInfo && !this.isBuiltinObject(objExpr)) {
                return `jspp::Exception::throw_unresolved_reference(${
                    this.getJsVarName(objExpr)
                })`;
            }
            if (typeInfo && !typeInfo.isBuiltin && !typeInfo.isParameter) {
                derefObj = this.getDerefCode(
                    objCode,
                    this.getJsVarName(objExpr),
                    context,
                    typeInfo,
                );
            }
        }

        if (!hasSpread) {
            const argsSpan = generateArgsSpan(callExpr.arguments);
            if (callExpr.questionDotToken) {
                return `jspp::Access::optional_call(${derefObj}.get_own_property("${propName}"), ${derefObj}, ${argsSpan}, "${
                    this.escapeString(propName)
                }")`;
            }
            return `${derefObj}.call_own_property("${propName}", ${argsSpan})`;
        } else {
            const argsVar = this.generateUniqueName(
                "__args_",
                this.getDeclaredSymbols(node),
            );
            let code = `([&]() {\n`;
            this.indentationLevel++;
            code += generateArgsVectorBuilder(callExpr.arguments, argsVar);
            if (callExpr.questionDotToken) {
                code +=
                    `${this.indent()}return jspp::Access::optional_call(${derefObj}.get_own_property("${propName}"), ${derefObj}, ${argsVar}, "${
                        this.escapeString(propName)
                    }");\n`;
            } else {
                code +=
                    `${this.indent()}return ${derefObj}.call_own_property("${propName}", ${argsVar});\n`;
            }
            this.indentationLevel--;
            code += `${this.indent()}})()`;
            return code;
        }
    }

    if (ts.isElementAccessExpression(callee)) {
        const elemAccess = callee as ts.ElementAccessExpression;
        const objExpr = elemAccess.expression;
        const objCode = this.visit(objExpr, context);

        let derefObj = objCode;
        if (ts.isIdentifier(objExpr)) {
            const scope = this.getScopeForNode(objExpr);
            const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                objExpr.getText(),
                scope,
            );
            if (!typeInfo && !this.isBuiltinObject(objExpr)) {
                return `jspp::Exception::throw_unresolved_reference(${
                    this.getJsVarName(objExpr)
                })`;
            }
            if (typeInfo && !typeInfo.isBuiltin && !typeInfo.isParameter) {
                derefObj = this.getDerefCode(
                    objCode,
                    this.getJsVarName(objExpr),
                    context,
                    typeInfo,
                );
            }
        }

        let argText = visitObjectPropertyName.call(
            this,
            elemAccess.argumentExpression as ts.PropertyName,
            { ...context, isBracketNotationPropertyAccess: true },
        );

        if (ts.isIdentifier(elemAccess.argumentExpression)) {
            const argScope = this.getScopeForNode(
                elemAccess.argumentExpression,
            );
            const argTypeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                elemAccess.argumentExpression.getText(),
                argScope,
            );
            if (
                !argTypeInfo &&
                !this.isBuiltinObject(elemAccess.argumentExpression)
            ) {
                return `jspp::Exception::throw_unresolved_reference(${
                    this.getJsVarName(
                        elemAccess.argumentExpression as ts.Identifier,
                    )
                })`;
            }
            if (
                argTypeInfo && !argTypeInfo.isParameter &&
                !argTypeInfo.isBuiltin
            ) {
                argText = this.getDerefCode(
                    argText,
                    this.getJsVarName(elemAccess.argumentExpression),
                    context,
                    argTypeInfo,
                );
            }
        }

        if (!hasSpread) {
            const argsSpan = generateArgsSpan(callExpr.arguments);
            if (callExpr.questionDotToken) {
                return `jspp::Access::optional_call(${derefObj}.get_own_property(${argText}), ${derefObj}, ${argsSpan})`;
            }
            return `${derefObj}.call_own_property(${argText}, ${argsSpan})`;
        } else {
            const argsVar = this.generateUniqueName(
                "__args_",
                this.getDeclaredSymbols(node),
            );
            let code = `([&]() {\n`;
            this.indentationLevel++;
            code += generateArgsVectorBuilder(callExpr.arguments, argsVar);
            if (callExpr.questionDotToken) {
                code +=
                    `${this.indent()}return jspp::Access::optional_call(${derefObj}.get_own_property(${argText}), ${derefObj}, ${argsVar});\n`;
            } else {
                code +=
                    `${this.indent()}return ${derefObj}.call_own_property(${argText}, ${argsVar});\n`;
            }
            this.indentationLevel--;
            code += `${this.indent()}})()`;
            return code;
        }
    }

    const calleeCode = this.visit(callee, context);
    let derefCallee = calleeCode;
    let calleeTypeInfo: TypeInfo | null = null;
    if (ts.isIdentifier(callee)) {
        const scope = this.getScopeForNode(callee);
        calleeTypeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
            callee.text,
            scope,
        );
        if (!calleeTypeInfo && !this.isBuiltinObject(callee)) {
            return `jspp::Exception::throw_unresolved_reference(${
                this.getJsVarName(
                    callee,
                )
            })`;
        }
        if (calleeTypeInfo?.isBuiltin) {
            derefCallee = calleeCode;
        } else if (calleeTypeInfo) {
            derefCallee = this.getDerefCode(
                calleeCode,
                this.getJsVarName(callee),
                context,
                calleeTypeInfo,
            );
        }
    }

    // Direct native lamda if available
    if (
        ts.isIdentifier(callee) && calleeTypeInfo
    ) {
        const name = callee.getText();
        const symbol = context.localScopeSymbols.get(name) ??
            context.globalScopeSymbols.get(name);

        const nativeFeature = symbol?.features?.native;

        if (nativeFeature && nativeFeature.type === "lambda") {
            const nativeName = nativeFeature.name;
            const parameters = nativeFeature.parameters || [];

            if (!hasSpread) {
                let argsPart = "";

                if (parameters) {
                    const argsArray = callExpr.arguments.map((arg) => {
                        let argText = this.visit(arg, context);
                        if (ts.isIdentifier(arg)) {
                            const scope = this.getScopeForNode(arg);
                            const typeInfo = this.typeAnalyzer.scopeManager
                                .lookupFromScope(arg.text, scope)!;
                            argText = this.getDerefCode(
                                argText,
                                this.getJsVarName(arg),
                                context,
                                typeInfo,
                            );
                        }
                        return argText;
                    });

                    const argsText = argsArray.slice(0, parameters.length)
                        .filter((
                            _,
                            i,
                        ) => !parameters![i]?.dotDotDotToken).join(", ");
                    if (argsText) argsPart += `, ${argsText}`;

                    if (
                        argsArray.length > parameters.length &&
                        !!parameters[parameters.length - 1]?.dotDotDotToken
                    ) {
                        const restArgsText =
                            `jspp::AnyValue::make_array(std::vector<jspp::AnyValue>{${
                                argsArray.slice(
                                    parameters.length - 1,
                                ).join(", ")
                            }})`;
                        argsPart += `, ${restArgsText}`;
                    }
                }

                const callImplementation =
                    `${nativeName}(jspp::Constants::UNDEFINED${argsPart})`;
                if (symbol.features.isGenerator) {
                    if (symbol.features.isAsync) {
                        return `jspp::AnyValue::from_async_iterator(${callImplementation})`;
                    }
                    return `jspp::AnyValue::from_iterator(${callImplementation})`;
                }
                if (symbol.features.isAsync) {
                    return `jspp::AnyValue::from_promise(${callImplementation})`;
                }
                return callImplementation;
            } else {
                const argsVar = this.generateUniqueName(
                    "__args_",
                    this.getDeclaredSymbols(node),
                );
                let code = `([&]() {\n`;
                this.indentationLevel++;
                code += generateArgsVectorBuilder(callExpr.arguments, argsVar);

                const callArgs: string[] = [];
                for (let i = 0; i < parameters.length; i++) {
                    const p = parameters[i];
                    if (!p) continue;
                    if (p.dotDotDotToken) {
                        callArgs.push(
                            `jspp::AnyValue::make_array(std::vector<jspp::AnyValue>(${argsVar}.begin() + std::min((size_t)${i}, ${argsVar}.size()), ${argsVar}.end()))`,
                        );
                    } else {
                        callArgs.push(
                            `(${argsVar}.size() > ${i} ? ${argsVar}[${i}] : jspp::Constants::UNDEFINED)`,
                        );
                    }
                }

                let callExprStr = `${nativeName}(jspp::Constants::UNDEFINED${
                    callArgs.length > 0 ? ", " + callArgs.join(", ") : ""
                })`;

                if (symbol.features.isGenerator) {
                    if (symbol.features.isAsync) {
                        callExprStr =
                            `jspp::AnyValue::from_async_iterator(${callExprStr})`;
                    } else {
                        callExprStr =
                            `jspp::AnyValue::from_iterator(${callExprStr})`;
                    }
                } else if (symbol.features.isAsync) {
                    callExprStr =
                        `jspp::AnyValue::from_promise(${callExprStr})`;
                }

                code += `${this.indent()}return ${callExprStr};\n`;
                this.indentationLevel--;
                code += `${this.indent()}})()`;
                return code;
            }
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

    const calleeNamePart = calleeName && calleeName.length > 0
        ? `, "${calleeName}"`
        : "";

    if (!hasSpread) {
        const argsSpan = generateArgsSpan(callExpr.arguments);
        if (callExpr.questionDotToken) {
            return `jspp::Access::optional_call(${derefCallee}, jspp::Constants::UNDEFINED, ${argsSpan}${calleeNamePart})`;
        }
        return `${derefCallee}.call(jspp::Constants::UNDEFINED, ${argsSpan}${calleeNamePart})`;
    } else {
        const argsVar = this.generateUniqueName(
            "__args_",
            this.getDeclaredSymbols(node),
        );
        let code = `([&]() {\n`;
        this.indentationLevel++;
        code += generateArgsVectorBuilder(callExpr.arguments, argsVar);
        if (callExpr.questionDotToken) {
            code +=
                `${this.indent()}return jspp::Access::optional_call(${derefCallee}, jspp::Constants::UNDEFINED, ${argsVar}${calleeNamePart});\n`;
        } else {
            code +=
                `${this.indent()}return ${derefCallee}.call(jspp::Constants::UNDEFINED, ${argsVar}${calleeNamePart});\n`;
        }
        this.indentationLevel--;
        code += `${this.indent()}})()`;
        return code;
    }
}

export function visitVoidExpression(
    this: CodeGenerator,
    node: ts.VoidExpression,
    context: VisitContext,
): string {
    const voidExpr = node as ts.VoidExpression;
    const exprText = this.visit(voidExpr.expression, context);
    return `(${exprText}, jspp::Constants::UNDEFINED)`;
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
                finalExpr = `jspp::Exception::throw_unresolved_reference(${
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
                    context,
                    typeInfo,
                );
            }
        }

        result = `jspp::add(${result}, ${finalExpr})`;

        if (span.literal.text) {
            result = `jspp::add(${result}, jspp::AnyValue::make_string("${
                this.escapeString(
                    span.literal.text,
                )
            }"))`;
        }
    }
    return result;
}

export function visitNewExpression(
    this: CodeGenerator,
    node: ts.NewExpression,
    context: VisitContext,
): string {
    const newExpr = node as ts.NewExpression;
    const exprText = this.visit(newExpr.expression, context);
    const hasSpread = newExpr.arguments?.some(ts.isSpreadElement) ?? false;

    let derefExpr = exprText;
    let name = `"${exprText}"`;

    if (ts.isIdentifier(newExpr.expression)) {
        name = this.getJsVarName(newExpr.expression);

        const scope = this.getScopeForNode(newExpr.expression);
        const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
            newExpr.expression.getText(),
            scope,
        );
        if (
            !typeInfo &&
            !this.isBuiltinObject(newExpr.expression as ts.Identifier)
        ) {
            return `jspp::Exception::throw_unresolved_reference(${name})`;
        }
        if (typeInfo && !typeInfo.isParameter && !typeInfo.isBuiltin) {
            derefExpr = this.getDerefCode(
                exprText,
                name,
                context,
                typeInfo,
            );
        }
    }

    if (!hasSpread) {
        const argsArray = newExpr.arguments
            ? newExpr.arguments
                .map((arg) => {
                    const argText = this.visit(arg, context);
                    if (ts.isIdentifier(arg)) {
                        const scope = this.getScopeForNode(arg);
                        const typeInfo = this.typeAnalyzer.scopeManager
                            .lookupFromScope(
                                arg.text,
                                scope,
                            );
                        if (!typeInfo) {
                            return `jspp::Exception::throw_unresolved_reference(${
                                this.getJsVarName(
                                    arg as ts.Identifier,
                                )
                            })`;
                        }
                        if (
                            typeInfo && !typeInfo.isParameter &&
                            !typeInfo.isBuiltin
                        ) {
                            return this.getDerefCode(
                                argText,
                                this.getJsVarName(arg as ts.Identifier),
                                context,
                                typeInfo,
                            );
                        }
                    }
                    return argText;
                })
            : [];
        const args = argsArray.join(", ");
        const argsCount = argsArray.length;
        const argsSpan = argsCount > 0
            ? `std::span<const jspp::AnyValue>((const jspp::AnyValue[]){${args}}, ${argsCount})`
            : "std::span<const jspp::AnyValue>{}";

        return `${derefExpr}.construct(${argsSpan}, ${name})`;
    } else {
        const argsVar = this.generateUniqueName(
            "__args_",
            this.getDeclaredSymbols(node),
        );
        let code = `([&]() {\n`;
        this.indentationLevel++;
        code += `${this.indent()}std::vector<jspp::AnyValue> ${argsVar};\n`;
        if (newExpr.arguments) {
            for (const arg of newExpr.arguments) {
                if (ts.isSpreadElement(arg)) {
                    let spreadExpr = this.visit(arg.expression, context);
                    if (ts.isIdentifier(arg.expression)) {
                        const scope = this.getScopeForNode(arg.expression);
                        const typeInfo = this.typeAnalyzer.scopeManager
                            .lookupFromScope(arg.expression.text, scope)!;
                        if (
                            typeInfo && !typeInfo.isBuiltin &&
                            !typeInfo.isParameter
                        ) {
                            spreadExpr = this.getDerefCode(
                                spreadExpr,
                                this.getJsVarName(arg.expression),
                                context,
                                typeInfo,
                            );
                        }
                    }
                    code +=
                        `${this.indent()}jspp::Access::spread_array(${argsVar}, ${spreadExpr});\n`;
                } else {
                    let argText = this.visit(arg, context);
                    if (ts.isIdentifier(arg)) {
                        const scope = this.getScopeForNode(arg);
                        const typeInfo = this.typeAnalyzer.scopeManager
                            .lookupFromScope(arg.text, scope)!;
                        if (
                            typeInfo && !typeInfo.isBuiltin &&
                            !typeInfo.isParameter
                        ) {
                            argText = this.getDerefCode(
                                argText,
                                this.getJsVarName(arg),
                                context,
                                typeInfo,
                            );
                        }
                    }
                    code +=
                        `${this.indent()}${argsVar}.push_back(${argText});\n`;
                }
            }
        }
        code +=
            `${this.indent()}  return ${derefExpr}.construct(${argsVar}, ${name});\n`;
        this.indentationLevel--;
        code += `${this.indent()}})()`;
        return code;
    }
}

export function visitTypeOfExpression(
    this: CodeGenerator,
    node: ts.TypeOfExpression,
    context: VisitContext,
): string {
    const typeOfExpr = node as ts.TypeOfExpression;
    const exprText = this.visit(typeOfExpr.expression, context);

    let derefExpr = exprText;
    if (ts.isIdentifier(typeOfExpr.expression)) {
        const scope = this.getScopeForNode(typeOfExpr.expression);
        const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
            typeOfExpr.expression.getText(),
            scope,
        );
        if (
            !typeInfo &&
            !this.isBuiltinObject(typeOfExpr.expression as ts.Identifier)
        ) {
            derefExpr = `/* undeclared variable: ${
                this.getJsVarName(
                    typeOfExpr.expression as ts.Identifier,
                )
            } */`; // typeof undeclared variable is 'undefined'
        }
        if (typeInfo && !typeInfo.isParameter && !typeInfo.isBuiltin) {
            derefExpr = this.getDerefCode(
                exprText,
                this.getJsVarName(typeOfExpr.expression as ts.Identifier),
                context,
                typeInfo,
            );
        }
    }

    return `jspp::Access::type_of(${derefExpr})`;
}

export function visitAwaitExpression(
    this: CodeGenerator,
    node: ts.AwaitExpression,
    context: VisitContext,
): string {
    const awaitExpr = node as ts.AwaitExpression;
    const exprText = this.visit(awaitExpr.expression, context);

    let derefExpr = exprText;
    if (ts.isIdentifier(awaitExpr.expression)) {
        const scope = this.getScopeForNode(awaitExpr.expression);
        const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
            awaitExpr.expression.getText(),
            scope,
        );
        if (
            !typeInfo &&
            !this.isBuiltinObject(awaitExpr.expression as ts.Identifier)
        ) {
            // This assumes co_awaiting the result of throw_unresolved_reference (which throws)
            // But throw_unresolved_reference returns AnyValue (void-ish).
            // We can just throw before co_await.
            // But we need to return a string expression.
            // Using comma operator: (throw..., AnyValue())
            return `(jspp::Exception::throw_unresolved_reference(${
                this.getJsVarName(
                    awaitExpr.expression as ts.Identifier,
                )
            }), co_await jspp::Constants::UNDEFINED)`;
        }
        if (typeInfo && !typeInfo.isParameter && !typeInfo.isBuiltin) {
            derefExpr = this.getDerefCode(
                exprText,
                this.getJsVarName(awaitExpr.expression as ts.Identifier),
                context,
                typeInfo,
            );
        }
    }

    return `co_await ${derefExpr}`;
}

export function visitDeleteExpression(
    this: CodeGenerator,
    node: ts.DeleteExpression,
    context: VisitContext,
): string {
    const expr = node.expression;
    if (ts.isPropertyAccessExpression(expr)) {
        const obj = this.visit(expr.expression, context);
        const prop = `jspp::AnyValue::make_string("${expr.name.getText()}")`;
        return `jspp::Access::delete_property(${obj}, ${prop})`;
    } else if (ts.isElementAccessExpression(expr)) {
        const obj = this.visit(expr.expression, context);
        const prop = this.visit(expr.argumentExpression, context);
        return `jspp::Access::delete_property(${obj}, ${prop})`;
    }
    return "jspp::Constants::TRUE"; // delete on non-property is true in JS
}

export function visitAsExpression(
    this: CodeGenerator,
    node: ts.AsExpression,
    context: VisitContext,
): string {
    return this.visit(node.expression, context);
}

export function visitTypeAssertionExpression(
    this: CodeGenerator,
    node: ts.TypeAssertion,
    context: VisitContext,
): string {
    return this.visit(node.expression, context);
}

export function visitNonNullExpression(
    this: CodeGenerator,
    node: ts.NonNullExpression,
    context: VisitContext,
): string {
    return this.visit(node.expression, context);
}

export function visitSatisfiesExpression(
    this: CodeGenerator,
    node: ts.SatisfiesExpression,
    context: VisitContext,
): string {
    return this.visit(node.expression, context);
}
