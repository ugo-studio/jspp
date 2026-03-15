import ts from "typescript";

import type { TypeInfo } from "../../analysis/typeAnalyzer.js";
import { constants } from "../constants.js";
import { CompilerError } from "../error.js";
import { CodeGenerator } from "./index.js";
import type { VisitContext } from "./visitor.js";

/**
 * Helper to recursively flatten static spread elements in arrays or call arguments.
 */
function flattenArrayElements(
    elements: ts.NodeArray<ts.Expression> | ts.Expression[],
): (ts.Expression | { dynamicSpread: ts.Expression })[] {
    const flattened: (ts.Expression | { dynamicSpread: ts.Expression })[] = [];
    for (const elem of elements) {
        if (elem && ts.isSpreadElement(elem)) {
            const expr = elem.expression;
            if (ts.isArrayLiteralExpression(expr)) {
                flattened.push(...flattenArrayElements(expr.elements));
            } else if (
                ts.isStringLiteral(expr) ||
                ts.isNoSubstitutionTemplateLiteral(expr)
            ) {
                for (const char of expr.text) {
                    flattened.push(ts.factory.createStringLiteral(char));
                }
            } else {
                flattened.push({ dynamicSpread: expr });
            }
        } else {
            flattened.push(elem);
        }
    }
    return flattened;
}

/**
 * Helper to recursively flatten static spread assignments in object literals.
 */
function flattenObjectProperties(
    properties:
        | ts.NodeArray<ts.ObjectLiteralElementLike>
        | ts.ObjectLiteralElementLike[],
): ts.ObjectLiteralElementLike[] {
    const flattened: ts.ObjectLiteralElementLike[] = [];
    for (const prop of properties) {
        if (ts.isSpreadAssignment(prop)) {
            const expr = prop.expression;
            if (ts.isObjectLiteralExpression(expr)) {
                flattened.push(...flattenObjectProperties(expr.properties));
            } else {
                flattened.push(prop);
            }
        } else {
            flattened.push(prop);
        }
    }
    return flattened;
}

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
            );
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
    const properties = flattenObjectProperties(obj.properties);

    const objVar = this.generateUniqueName(
        "__obj_",
        this.getDeclaredSymbols(node),
    );

    if (
        !properties.some((prop) =>
            ts.isPropertyAssignment(prop) ||
            ts.isShorthandPropertyAssignment(prop) ||
            ts.isMethodDeclaration(prop) || ts.isGetAccessor(prop) ||
            ts.isSetAccessor(prop) || ts.isSpreadAssignment(prop)
        )
    ) {
        // Empty object
        return `jspp::AnyValue::make_object({}).set_prototype(::Object.get_own_property("prototype"))`;
    }

    let code = `([&]() {\n`;
    code +=
        `${this.indent()}  auto ${objVar} = jspp::AnyValue::make_object({}).set_prototype(::Object.get_own_property("prototype"));\n`;

    this.indentationLevel++;

    for (const prop of properties) {
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
                );
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
            );
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
                );
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
    const flattened = flattenArrayElements(node.elements);
    const hasSpread = flattened.some((e) =>
        typeof e === "object" && "dynamicSpread" in e
    );

    if (!hasSpread) {
        const elements = flattened
            .map((elem) => {
                const expr = elem as ts.Expression;
                let elemText = this.visit(expr, context);
                if (ts.isIdentifier(expr)) {
                    const scope = this.getScopeForNode(expr);
                    const typeInfo = this.typeAnalyzer.scopeManager
                        .lookupFromScope(
                            expr.text,
                            scope,
                        );
                    if (
                        typeInfo && !typeInfo.isBuiltin && !typeInfo.isParameter
                    ) {
                        elemText = this.getDerefCode(
                            elemText,
                            this.getJsVarName(expr),
                            context,
                            typeInfo,
                        );
                    }
                }
                if (ts.isOmittedExpression(expr)) {
                    elemText = "jspp::Constants::UNINITIALIZED";
                }
                return elemText;
            });
        const elementsJoined = elements.join(", ");
        const elementsSpan = elements.length > 0
            ? `std::span<const jspp::AnyValue>((const jspp::AnyValue[]){${elementsJoined}}, ${elements.length})`
            : "std::span<const jspp::AnyValue>{}";
        return `jspp::AnyValue::make_array(${elementsSpan}).set_prototype(::Array.get_own_property("prototype"))`;
    }

    const arrVar = this.generateUniqueName(
        "__arr_",
        this.getDeclaredSymbols(node),
    );
    let code = `([&]() {\n`;
    code += `${this.indent()}  std::vector<jspp::AnyValue> ${arrVar};\n`;
    this.indentationLevel++;

    for (const elem of flattened) {
        if (typeof elem === "object" && "dynamicSpread" in elem) {
            const spreadExprSource = elem.dynamicSpread;
            let spreadExpr = this.visit(spreadExprSource, context);
            if (ts.isIdentifier(spreadExprSource)) {
                const scope = this.getScopeForNode(spreadExprSource);
                const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                    spreadExprSource.text,
                    scope,
                );
                if (typeInfo && !typeInfo.isBuiltin && !typeInfo.isParameter) {
                    spreadExpr = this.getDerefCode(
                        spreadExpr,
                        this.getJsVarName(spreadExprSource),
                        context,
                        typeInfo,
                    );
                }
            }
            code +=
                `${this.indent()}jspp::Access::spread_array(${arrVar}, ${spreadExpr});\n`;
        } else {
            const expr = elem as ts.Expression;
            let elemText = this.visit(expr, context);
            if (ts.isIdentifier(expr)) {
                const scope = this.getScopeForNode(expr);
                const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                    expr.text,
                    scope,
                );
                if (typeInfo && !typeInfo.isBuiltin && !typeInfo.isParameter) {
                    elemText = this.getDerefCode(
                        elemText,
                        this.getJsVarName(expr),
                        context,
                        typeInfo,
                    );
                }
            }
            if (ts.isOmittedExpression(expr)) {
                elemText = "jspp::Constants::UNINITIALIZED";
            }
            code += `${this.indent()}${arrVar}.push_back(${elemText});\n`;
        }
    }

    this.indentationLevel--;
    code +=
        `${this.indent()}  return jspp::AnyValue::make_array(std::move(${arrVar})).set_prototype(::Array.get_own_property("prototype"));\n`;
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
        const opFunc = operator === "++" ? "add" : "sub";
        if (ts.isPropertyAccessExpression(prefixUnaryExpr.operand)) {
            const obj = this.visit(prefixUnaryExpr.operand.expression, context);
            const prop = visitObjectPropertyName.call(
                this,
                prefixUnaryExpr.operand.name,
                context,
            );
            return `${obj}.set_own_property(${prop}, jspp::${opFunc}(${obj}.get_own_property(${prop}), 1.0))`;
        } else if (ts.isElementAccessExpression(prefixUnaryExpr.operand)) {
            const obj = this.visit(prefixUnaryExpr.operand.expression, context);
            const prop = visitObjectPropertyName.call(
                this,
                prefixUnaryExpr.operand.argumentExpression as ts.PropertyName,
                { ...context, isBracketNotationPropertyAccess: true },
            );
            return `${obj}.set_own_property(${prop}, jspp::${opFunc}(${obj}.get_own_property(${prop}), 1.0))`;
        }

        let target = operand;
        if (ts.isIdentifier(prefixUnaryExpr.operand)) {
            const scope = this.getScopeForNode(prefixUnaryExpr.operand);
            const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                prefixUnaryExpr.operand.getText(),
                scope,
            );
            if (context.derefBeforeAssignment) {
                target = this.getDerefCode(operand, operand, context, typeInfo);
            } else if (typeInfo?.needsHeapAllocation) {
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
            );
            if (context.derefBeforeAssignment) {
                target = this.getDerefCode(operand, operand, context, typeInfo);
            } else if (typeInfo?.needsHeapAllocation) {
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

    if (ts.isPropertyAccessExpression(postfixUnaryExpr.operand)) {
        const obj = this.visit(postfixUnaryExpr.operand.expression, context);
        const prop = visitObjectPropertyName.call(
            this,
            postfixUnaryExpr.operand.name,
            context,
        );
        const opFunc = operator === "++" ? "add" : "sub";
        // Postfix needs IILE to return old value
        return `([&]() { auto oldVal = ${obj}.get_own_property(${prop}); ${obj}.set_own_property(${prop}, jspp::${opFunc}(oldVal, 1.0)); return oldVal; })()`;
    } else if (ts.isElementAccessExpression(postfixUnaryExpr.operand)) {
        const obj = this.visit(postfixUnaryExpr.operand.expression, context);
        const prop = visitObjectPropertyName.call(
            this,
            postfixUnaryExpr.operand.argumentExpression as ts.PropertyName,
            { ...context, isBracketNotationPropertyAccess: true },
        );
        const opFunc = operator === "++" ? "add" : "sub";
        // Postfix needs IILE to return old value
        return `([&]() { auto oldVal = ${obj}.get_own_property(${prop}); ${obj}.set_own_property(${prop}, jspp::${opFunc}(oldVal, 1.0)); return oldVal; })()`;
    }

    let target = operand;
    if (ts.isIdentifier(postfixUnaryExpr.operand)) {
        const scope = this.getScopeForNode(postfixUnaryExpr.operand);
        const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
            postfixUnaryExpr.operand.getText(),
            scope,
        );
        if (context.derefBeforeAssignment) {
            target = this.getDerefCode(operand, operand, context, typeInfo);
        } else if (typeInfo?.needsHeapAllocation) {
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
        return `jspp::Access::get_optional_property(${finalExpr}, "${propName}")`;
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
        return `jspp::Access::get_optional_element(${finalExpr}, ${argText})`;
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
            const rightText = this.visit(binExpr.right, visitContext);
            if (ts.isIdentifier(binExpr.left)) {
                const leftText = this.visit(binExpr.left, visitContext);
                const scope = this.getScopeForNode(binExpr.left);
                const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                    binExpr.left.text,
                    scope,
                );
                const target = typeInfo?.needsHeapAllocation
                    ? `*${leftText}`
                    : leftText;
                return `jspp::unsigned_right_shift_assign(${target}, ${rightText})`;
            } else if (ts.isPropertyAccessExpression(binExpr.left)) {
                const obj = this.visit(binExpr.left.expression, context);
                const prop = visitObjectPropertyName.call(
                    this,
                    binExpr.left.name,
                    context,
                );
                return `${obj}.set_own_property(${prop}, jspp::unsigned_right_shift(${obj}.get_own_property(${prop}), ${rightText}))`;
            } else if (ts.isElementAccessExpression(binExpr.left)) {
                const obj = this.visit(binExpr.left.expression, context);
                const prop = visitObjectPropertyName.call(
                    this,
                    binExpr.left.argumentExpression as ts.PropertyName,
                    { ...context, isBracketNotationPropertyAccess: true },
                );
                return `${obj}.set_own_property(${prop}, jspp::unsigned_right_shift(${obj}.get_own_property(${prop}), ${rightText}))`;
            }
        }

        if (
            opToken.kind === ts.SyntaxKind.AmpersandAmpersandEqualsToken ||
            opToken.kind === ts.SyntaxKind.BarBarEqualsToken ||
            opToken.kind === ts.SyntaxKind.QuestionQuestionEqualsToken
        ) {
            const rightText = this.visit(binExpr.right, visitContext);
            if (ts.isIdentifier(binExpr.left)) {
                const leftText = this.visit(binExpr.left, visitContext);
                const scope = this.getScopeForNode(binExpr.left);
                const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                    binExpr.left.text,
                    scope,
                );
                const target = typeInfo?.needsHeapAllocation
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
            } else if (ts.isPropertyAccessExpression(binExpr.left)) {
                const obj = this.visit(binExpr.left.expression, context);
                const prop = visitObjectPropertyName.call(
                    this,
                    binExpr.left.name,
                    context,
                );
                const func = opToken.kind ===
                        ts.SyntaxKind.AmpersandAmpersandEqualsToken
                    ? "logical_and"
                    : (opToken.kind === ts.SyntaxKind.BarBarEqualsToken
                        ? "logical_or"
                        : "nullish_coalesce");
                // Logical assignments return newVal, set_own_property returns newVal.
                return `${obj}.set_own_property(${prop}, jspp::${func}(${obj}.get_own_property(${prop}), ${rightText}))`;
            } else if (ts.isElementAccessExpression(binExpr.left)) {
                const obj = this.visit(binExpr.left.expression, context);
                const prop = visitObjectPropertyName.call(
                    this,
                    binExpr.left.argumentExpression as ts.PropertyName,
                    { ...context, isBracketNotationPropertyAccess: true },
                );
                const func = opToken.kind ===
                        ts.SyntaxKind.AmpersandAmpersandEqualsToken
                    ? "logical_and"
                    : (opToken.kind === ts.SyntaxKind.BarBarEqualsToken
                        ? "logical_or"
                        : "nullish_coalesce");
                return `${obj}.set_own_property(${prop}, jspp::${func}(${obj}.get_own_property(${prop}), ${rightText}))`;
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
            );
            rightText = this.getDerefCode(
                rightText,
                this.getJsVarName(binExpr.right as ts.Identifier),
                visitContext,
                typeInfo,
            );
        }

        if (ts.isPropertyAccessExpression(binExpr.left)) {
            const obj = this.visit(binExpr.left.expression, context);
            const prop = visitObjectPropertyName.call(
                this,
                binExpr.left.name,
                context,
            );
            const opBase = op.slice(0, -1); // "+=", "-=" -> "+", "-"
            const opFunc = opBase === "+" ? "add" : (
                opBase === "-" ? "sub" : (
                    opBase === "*" ? "mul" : (
                        opBase === "/" ? "div" : (
                            opBase === "%" ? "mod" : (
                                opBase === "&" ? "bitwise_and" : (
                                    opBase === "|" ? "bitwise_or" : (
                                        opBase === "^" ? "bitwise_xor" : (
                                            opBase === "<<" ? "left_shift" : (
                                                opBase === ">>"
                                                    ? "right_shift"
                                                    : ""
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            );

            if (opFunc) {
                return `${obj}.set_own_property(${prop}, jspp::${opFunc}(${obj}.get_own_property(${prop}), ${rightText}))`;
            } else {
                // Fallback to IILE if we don't have an opFunc mapping
                return `([&]() { auto val = ${obj}.get_own_property(${prop}); val ${op} ${rightText}; ${obj}.set_own_property(${prop}, val); return val; })()`;
            }
        } else if (ts.isElementAccessExpression(binExpr.left)) {
            const obj = this.visit(binExpr.left.expression, context);
            const prop = visitObjectPropertyName.call(
                this,
                binExpr.left.argumentExpression as ts.PropertyName,
                { ...context, isBracketNotationPropertyAccess: true },
            );
            const opBase = op.slice(0, -1);
            const opFunc = opBase === "+" ? "add" : (
                opBase === "-" ? "sub" : (
                    opBase === "*" ? "mul" : (
                        opBase === "/" ? "div" : (
                            opBase === "%" ? "mod" : (
                                opBase === "&" ? "bitwise_and" : (
                                    opBase === "|" ? "bitwise_or" : (
                                        opBase === "^" ? "bitwise_xor" : (
                                            opBase === "<<" ? "left_shift" : (
                                                opBase === ">>"
                                                    ? "right_shift"
                                                    : ""
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            );

            if (opFunc) {
                return `${obj}.set_own_property(${prop}, jspp::${opFunc}(${obj}.get_own_property(${prop}), ${rightText}))`;
            } else {
                return `([&]() { auto val = ${obj}.get_own_property(${prop}); val ${op} ${rightText}; ${obj}.set_own_property(${prop}, val); return val; })()`;
            }
        }

        let target = leftText;
        if (ts.isIdentifier(binExpr.left)) {
            const scope = this.getScopeForNode(binExpr.left);
            const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                binExpr.left.getText(),
                scope,
            );
            if (context.derefBeforeAssignment) {
                target = this.getDerefCode(
                    leftText,
                    leftText,
                    visitContext,
                    typeInfo,
                );
            } else if (typeInfo?.needsHeapAllocation) {
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
                );

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
                );
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

        // Array/Object destructuring assignment
        if (
            ts.isArrayLiteralExpression(binExpr.left) ||
            ts.isObjectLiteralExpression(binExpr.left)
        ) {
            const rhsCode = this.visit(binExpr.right, visitContext);
            return this.generateDestructuring(
                binExpr.left,
                rhsCode,
                visitContext,
            );
        }

        // Simple variable or property assignment
        const leftText = this.visit(binExpr.left, visitContext);
        const scope = this.getScopeForNode(binExpr.left);
        const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
            (binExpr.left as ts.Identifier).text,
            scope,
        );
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
            : (typeInfo?.needsHeapAllocation ? `*${leftText}` : leftText);

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
        const nodeType = this.typeAnalyzer.inferNodeReturnType(binExpr.left);
        if (nodeType === "number") {
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
        const nodeType = this.typeAnalyzer.inferNodeReturnType(binExpr.right);
        if (nodeType === "number") {
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

    // Native values for lhs and rhs
    const literalLeft = ts.isNumericLiteral(binExpr.left)
        ? binExpr.left.getText()
        : finalLeft;
    const literalRight = ts.isNumericLiteral(binExpr.right)
        ? binExpr.right.getText()
        : finalRight;

    let supportsNativeValue = false;
    const exprReturnType = this.typeAnalyzer.inferNodeReturnType(node);
    if (
        exprReturnType === "boolean" &&
        (ts.isIfStatement(node.parent) ||
            ts.isConditionalExpression(node.parent))
    ) {
        supportsNativeValue = true;
    } else if (
        exprReturnType === "number" &&
        context.isInsideNativeLambda &&
        context.isInsideFunction
    ) {
        const funcDecl = this
            .findEnclosingFunctionDeclarationFromReturnStatement(node);
        if (funcDecl) {
            const funcReturnType = this.typeAnalyzer.inferFunctionReturnType(
                funcDecl,
            );
            if (funcReturnType === "number") {
                supportsNativeValue = true;
            }
        }
    }

    const method = supportsNativeValue ? "_native" : "";

    // Return boxed value
    if (opToken.kind === ts.SyntaxKind.EqualsEqualsEqualsToken) {
        return `jspp::is_strictly_equal_to${method}(${literalLeft}, ${literalRight})`;
    }
    if (opToken.kind === ts.SyntaxKind.EqualsEqualsToken) {
        return `jspp::is_equal_to${method}(${literalLeft}, ${literalRight})`;
    }
    if (opToken.kind === ts.SyntaxKind.ExclamationEqualsEqualsToken) {
        return `jspp::not_strictly_equal_to${method}(${literalLeft}, ${literalRight})`;
    }
    if (opToken.kind === ts.SyntaxKind.ExclamationEqualsToken) {
        return `jspp::not_equal_to${method}(${literalLeft}, ${literalRight})`;
    }
    if (opToken.kind === ts.SyntaxKind.AsteriskAsteriskToken) {
        return `jspp::pow${method}(${literalLeft}, ${literalRight})`;
    }
    if (opToken.kind === ts.SyntaxKind.GreaterThanGreaterThanGreaterThanToken) {
        return `jspp::unsigned_right_shift${method}(${literalLeft}, ${literalRight})`;
    }

    // For other arithmetic and bitwise operations, use native operations if possible
    switch (op) {
        case "+":
            return `jspp::add${method}(${literalLeft}, ${literalRight})`;
        case "-":
            return `jspp::sub${method}(${literalLeft}, ${literalRight})`;
        case "*":
            return `jspp::mul${method}(${literalLeft}, ${literalRight})`;
        case "/":
            return `jspp::div${method}(${literalLeft}, ${literalRight})`;
        case "%":
            return `jspp::mod${method}(${literalLeft}, ${literalRight})`;
        case "^":
            return `jspp::bitwise_xor${method}(${literalLeft}, ${literalRight})`;
        case "&":
            return `jspp::bitwise_and${method}(${literalLeft}, ${literalRight})`;
        case "|":
            return `jspp::bitwise_or${method}(${literalLeft}, ${literalRight})`;
        case "<<":
            return `jspp::left_shift${method}(${literalLeft}, ${literalRight})`;
        case ">>":
            return `jspp::right_shift${method}(${literalLeft}, ${literalRight})`;
        case "<":
            return `jspp::less_than${method}(${literalLeft}, ${literalRight})`;
        case ">":
            return `jspp::greater_than${method}(${literalLeft}, ${literalRight})`;
        case "<=":
            return `jspp::less_than_or_equal${method}(${literalLeft}, ${literalRight})`;
        case ">=":
            return `jspp::greater_than_or_equal${method}(${literalLeft}, ${literalRight})`;
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

    const condition = this.visit(condExpr.condition, context);
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
    const flattened = flattenArrayElements(callExpr.arguments);
    const hasSpread = flattened.some((e) =>
        typeof e === "object" && "dynamicSpread" in e
    );

    if (callee.kind === ts.SyntaxKind.SuperKeyword) {
        if (!context.superClassVar) {
            throw new CompilerError(
                "super() called but no super class variable found in context",
                callee,
                "SyntaxError",
            );
        }
        if (!hasSpread) {
            const args = flattened.map((arg) =>
                this.visit(arg as ts.Expression, context)
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
            for (const arg of flattened) {
                if (typeof arg === "object" && "dynamicSpread" in arg) {
                    const spreadExprSource = arg.dynamicSpread;
                    let spreadExpr = this.visit(spreadExprSource, context);
                    if (ts.isIdentifier(spreadExprSource)) {
                        const scope = this.getScopeForNode(spreadExprSource);
                        const typeInfo = this.typeAnalyzer.scopeManager
                            .lookupFromScope(spreadExprSource.text, scope);
                        spreadExpr = this.getDerefCode(
                            spreadExpr,
                            this.getJsVarName(spreadExprSource),
                            context,
                            typeInfo,
                        );
                    }
                    code +=
                        `${this.indent()}jspp::Access::spread_array(${argsVar}, ${spreadExpr});\n`;
                } else {
                    const expr = arg as ts.Expression;
                    let argText = this.visit(expr, context);
                    if (ts.isIdentifier(expr)) {
                        const scope = this.getScopeForNode(expr);
                        const typeInfo = this.typeAnalyzer.scopeManager
                            .lookupFromScope(expr.text, scope);
                        argText = this.getDerefCode(
                            argText,
                            this.getJsVarName(expr),
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

    const generateArgsSpan = (
        args: (ts.Expression | { dynamicSpread: ts.Expression })[],
    ) => {
        const argsArray = args
            .map((arg) => {
                const expr = arg as ts.Expression;
                const argText = this.visit(expr, context);
                if (ts.isIdentifier(expr)) {
                    const scope = this.getScopeForNode(expr);
                    const typeInfo = this.typeAnalyzer.scopeManager
                        .lookupFromScope(
                            expr.text,
                            scope,
                        );
                    if (!typeInfo) {
                        return `jspp::Exception::throw_unresolved_reference(${
                            this.getJsVarName(
                                expr,
                            )
                        })`;
                    }
                    if (typeInfo && !typeInfo.isBuiltin) {
                        return this.getDerefCode(
                            argText,
                            this.getJsVarName(expr),
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
        args: (ts.Expression | { dynamicSpread: ts.Expression })[],
        argsVar: string,
    ) => {
        let code = `${this.indent()}std::vector<jspp::AnyValue> ${argsVar};\n`;
        for (const arg of args) {
            if (typeof arg === "object" && "dynamicSpread" in arg) {
                const spreadExprSource = arg.dynamicSpread;
                let spreadExpr = this.visit(spreadExprSource, context);
                if (ts.isIdentifier(spreadExprSource)) {
                    const scope = this.getScopeForNode(spreadExprSource);
                    const typeInfo = this.typeAnalyzer.scopeManager
                        .lookupFromScope(spreadExprSource.text, scope);
                    if (
                        typeInfo && !typeInfo.isBuiltin && !typeInfo.isParameter
                    ) {
                        spreadExpr = this.getDerefCode(
                            spreadExpr,
                            this.getJsVarName(spreadExprSource),
                            context,
                            typeInfo,
                        );
                    }
                }
                code +=
                    `${this.indent()}jspp::Access::spread_array(${argsVar}, ${spreadExpr});\n`;
            } else {
                const expr = arg as ts.Expression;
                let argText = this.visit(expr, context);
                if (ts.isIdentifier(expr)) {
                    const scope = this.getScopeForNode(expr);
                    const typeInfo = this.typeAnalyzer.scopeManager
                        .lookupFromScope(expr.text, scope);
                    if (
                        typeInfo && !typeInfo.isBuiltin && !typeInfo.isParameter
                    ) {
                        argText = this.getDerefCode(
                            argText,
                            this.getJsVarName(expr),
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
                const argsSpan = generateArgsSpan(flattened);
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
                code += generateArgsVectorBuilder(flattened, argsVar);
                code +=
                    `${this.indent}return (${context.superClassVar}).get_own_property("prototype").get_own_property("${propName}").call(${this.globalThisVar}, ${argsVar}, "${
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
            const argsSpan = generateArgsSpan(flattened);
            if (propAccess.questionDotToken) {
                const method = callExpr.questionDotToken
                    ? "jspp::Access::call_optional_property_with_optional_call"
                    : "jspp::Access::call_optional_property";
                return `${method}(${derefObj}, "${propName}", ${argsSpan}, "${
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
            code += generateArgsVectorBuilder(flattened, argsVar);
            if (propAccess.questionDotToken) {
                const method = callExpr.questionDotToken
                    ? "jspp::Access::call_optional_property_with_optional_call"
                    : "jspp::Access::call_optional_property";
                code +=
                    `${this.indent()}return ${method}(${derefObj}, "${propName}", ${argsVar}, "${
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
            const argsSpan = generateArgsSpan(flattened);
            if (elemAccess.questionDotToken) {
                const method = callExpr.questionDotToken
                    ? "jspp::Access::call_optional_property_with_optional_call"
                    : "jspp::Access::call_optional_property";
                return `${method}(${derefObj}, ${argText}, ${argsSpan})`;
            }
            return `${derefObj}.call_own_property(${argText}, ${argsSpan})`;
        } else {
            const argsVar = this.generateUniqueName(
                "__args_",
                this.getDeclaredSymbols(node),
            );
            let code = `([&]() {\n`;
            this.indentationLevel++;
            code += generateArgsVectorBuilder(flattened, argsVar);
            if (elemAccess.questionDotToken) {
                const method = callExpr.questionDotToken
                    ? "jspp::Access::call_optional_property_with_optional_call"
                    : "jspp::Access::call_optional_property";
                code +=
                    `${this.indent()}return ${method}(${derefObj}, ${argText}, ${argsVar});\n`;
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
            const argumentKeywordIsUsed = nativeFeature.argumentKeywordIsUsed;

            if (!hasSpread) {
                let argsPart = "";

                if (parameters) {
                    const argsArray = flattened.map((arg) => {
                        const expr = arg as ts.Expression;
                        let argText = this.visit(expr, context);
                        if (ts.isIdentifier(expr)) {
                            const scope = this.getScopeForNode(expr);
                            const typeInfo = this.typeAnalyzer.scopeManager
                                .lookupFromScope(expr.text, scope);
                            argText = this.getDerefCode(
                                argText,
                                this.getJsVarName(expr),
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
                        argsArray.length >= parameters.length &&
                        !!parameters[parameters.length - 1]?.dotDotDotToken
                    ) {
                        const restArgsText =
                            `jspp::AnyValue::make_array(std::vector<jspp::AnyValue>{${
                                argsArray.slice(
                                    parameters.length - 1,
                                ).join(", ")
                            }})`;
                        argsPart += `, ${restArgsText}${
                            argumentKeywordIsUsed ? `, ${argsArray.length}` : ""
                        }`;
                    } else if (
                        argsArray.length > parameters.length &&
                        argumentKeywordIsUsed
                    ) {
                        const restArgsArray = argsArray.slice(
                            parameters.length,
                        );
                        const nativeRestArgsText =
                            `std::span<const jspp::AnyValue>((const jspp::AnyValue[]){${
                                restArgsArray.join(", ")
                            }}, ${restArgsArray.length})`;
                        argsPart +=
                            `, ${argsArray.length}, ${nativeRestArgsText}`;
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
                code += generateArgsVectorBuilder(flattened, argsVar);

                const callArgs: string[] = [];
                for (let i = 0; i < parameters.length; i++) {
                    const p = parameters[i];
                    if (!p) continue;
                    if (p.dotDotDotToken) {
                        callArgs.push(
                            `jspp::AnyValue::make_array(std::vector<jspp::AnyValue>(${argsVar}.begin() + std::min((std::size_t)${i}, ${argsVar}.size()), ${argsVar}.end()))`,
                        );
                    } else {
                        callArgs.push(
                            `(${argsVar}.size() > ${i} ? ${argsVar}[${i}] : jspp::Constants::UNDEFINED)`,
                        );
                    }
                }

                const callArgsPart = callArgs.length > 0
                    ? `, ${callArgs.join(", ")}`
                    : "";
                const totalArgsSizePart = argumentKeywordIsUsed
                    ? `, ${argsVar}.size()`
                    : "";

                let callImplementation =
                    `${nativeName}(jspp::Constants::UNDEFINED${callArgsPart}${totalArgsSizePart})`;

                if (symbol.features.isGenerator) {
                    if (symbol.features.isAsync) {
                        callImplementation =
                            `jspp::AnyValue::from_async_iterator(${callImplementation})`;
                    } else {
                        callImplementation =
                            `jspp::AnyValue::from_iterator(${callImplementation})`;
                    }
                } else if (symbol.features.isAsync) {
                    callImplementation =
                        `jspp::AnyValue::from_promise(${callImplementation})`;
                }

                code += `${this.indent()}return ${callImplementation};\n`;
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
        const argsSpan = generateArgsSpan(flattened);
        if (callExpr.questionDotToken) {
            return `${derefCallee}.optional_call(jspp::Constants::UNDEFINED, ${argsSpan}${calleeNamePart})`;
        }
        return `${derefCallee}.call(jspp::Constants::UNDEFINED, ${argsSpan}${calleeNamePart})`;
    } else {
        const argsVar = this.generateUniqueName(
            "__args_",
            this.getDeclaredSymbols(node),
        );
        let code = `([&]() {\n`;
        this.indentationLevel++;
        code += generateArgsVectorBuilder(flattened, argsVar);
        if (callExpr.questionDotToken) {
            code +=
                `${this.indent()}return ${derefCallee}.optional_call(jspp::Constants::UNDEFINED, ${argsVar}${calleeNamePart});\n`;
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
    const flattened = flattenArrayElements(newExpr.arguments || [] as any);
    const hasSpread = flattened.some((e) =>
        typeof e === "object" && "dynamicSpread" in e
    );

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
        const argsArray = flattened
            .map((arg) => {
                const expr = arg as ts.Expression;
                const argText = this.visit(expr, context);
                if (ts.isIdentifier(expr)) {
                    const scope = this.getScopeForNode(expr);
                    const typeInfo = this.typeAnalyzer.scopeManager
                        .lookupFromScope(
                            expr.text,
                            scope,
                        );
                    if (!typeInfo) {
                        return `jspp::Exception::throw_unresolved_reference(${
                            this.getJsVarName(
                                expr as ts.Identifier,
                            )
                        })`;
                    }
                    if (
                        typeInfo && !typeInfo.isParameter &&
                        !typeInfo.isBuiltin
                    ) {
                        return this.getDerefCode(
                            argText,
                            this.getJsVarName(expr as ts.Identifier),
                            context,
                            typeInfo,
                        );
                    }
                }
                return argText;
            });
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
            for (const arg of flattened) {
                if (typeof arg === "object" && "dynamicSpread" in arg) {
                    const spreadExprSource = arg.dynamicSpread;
                    let spreadExpr = this.visit(spreadExprSource, context);
                    if (ts.isIdentifier(spreadExprSource)) {
                        const scope = this.getScopeForNode(spreadExprSource);
                        const typeInfo = this.typeAnalyzer.scopeManager
                            .lookupFromScope(spreadExprSource.text, scope);
                        if (
                            typeInfo && !typeInfo.isBuiltin &&
                            !typeInfo.isParameter
                        ) {
                            spreadExpr = this.getDerefCode(
                                spreadExpr,
                                this.getJsVarName(spreadExprSource),
                                context,
                                typeInfo,
                            );
                        }
                    }
                    code +=
                        `${this.indent()}jspp::Access::spread_array(${argsVar}, ${spreadExpr});\n`;
                } else {
                    const expr = arg as ts.Expression;
                    let argText = this.visit(expr, context);
                    if (ts.isIdentifier(expr)) {
                        const scope = this.getScopeForNode(expr);
                        const typeInfo = this.typeAnalyzer.scopeManager
                            .lookupFromScope(expr.text, scope);
                        if (
                            typeInfo && !typeInfo.isBuiltin &&
                            !typeInfo.isParameter
                        ) {
                            argText = this.getDerefCode(
                                argText,
                                this.getJsVarName(expr),
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
