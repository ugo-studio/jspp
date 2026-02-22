import ts from "typescript";

import { visitObjectPropertyName } from "./expression-handlers.js";
import { CodeGenerator } from "./index.js";
import { type VisitContext } from "./visitor.js";

export function generateDestructuring(
    this: CodeGenerator,
    lhs:
        | ts.BindingPattern
        | ts.ArrayLiteralExpression
        | ts.ObjectLiteralExpression,
    rhsCode: string,
    context: VisitContext,
): string {
    const declaredSymbols = this.getDeclaredSymbols(lhs);
    const sourceVar = this.generateUniqueName(
        "__destruct_src",
        declaredSymbols,
        context.localScopeSymbols,
        context.globalScopeSymbols,
    );

    let code = `([&]() {\n`;
    this.indentationLevel++;
    code += `${this.indent()}auto ${sourceVar} = ${rhsCode};\n`;

    const genAssignment = (pattern: ts.Node, valueCode: string): string => {
        if (ts.isIdentifier(pattern)) {
            const name = pattern.text;
            const scope = this.getScopeForNode(pattern);
            const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                name,
                scope,
            );

            // Mark initialized if it's a declaration
            this.markSymbolAsInitialized(
                name,
                context.globalScopeSymbols,
                context.localScopeSymbols,
            );

            let target = this.visit(pattern, context);
            if (typeInfo?.needsHeapAllocation) {
                target = `(*${target})`;
            }
            return `${this.indent()}${target} = ${valueCode};\n`;
        } else if (ts.isPropertyAccessExpression(pattern)) {
            const objCode = this.visit(pattern.expression, context);
            const propName = visitObjectPropertyName.call(
                this,
                pattern.name,
                context,
            );
            return `${this.indent()}${objCode}.set_own_property(${propName}, ${valueCode});\n`;
        } else if (ts.isElementAccessExpression(pattern)) {
            const objCode = this.visit(pattern.expression, context);
            const argCode = visitObjectPropertyName.call(
                this,
                pattern.argumentExpression as ts.PropertyName,
                { ...context, isBracketNotationPropertyAccess: true },
            );
            return `${this.indent()}${objCode}.set_own_property(${argCode}, ${valueCode});\n`;
        } else if (
            ts.isArrayBindingPattern(pattern) ||
            ts.isArrayLiteralExpression(pattern)
        ) {
            let innerCode = "";
            const elements = ts.isArrayBindingPattern(pattern)
                ? pattern.elements
                : pattern.elements;

            const iterVar = this.generateUniqueName(
                "__destruct_iter",
                declaredSymbols,
                context.localScopeSymbols,
                context.globalScopeSymbols,
            );
            const nextVar = this.generateUniqueName(
                "__destruct_next",
                declaredSymbols,
                context.localScopeSymbols,
                context.globalScopeSymbols,
            );

            innerCode +=
                `${this.indent()}auto ${iterVar} = jspp::Access::get_object_value_iterator(${valueCode}, "destructuring");\n`;
            innerCode +=
                `${this.indent()}auto ${nextVar} = ${iterVar}.get_own_property("next");\n`;

            elements.forEach((element, index) => {
                if (ts.isOmittedExpression(element)) {
                    innerCode +=
                        `${this.indent()}${nextVar}.call(${iterVar}, {}, "next");\n`;
                    return;
                }

                let target: ts.Node;
                let initializer: ts.Expression | undefined;
                let isRest = false;

                if (ts.isBindingElement(element)) {
                    target = element.name;
                    initializer = element.initializer;
                    isRest = !!element.dotDotDotToken;
                } else if (ts.isSpreadElement(element)) {
                    target = element.expression;
                    isRest = true;
                } else {
                    target = element as ts.Expression;
                }

                if (isRest) {
                    const restVecVar = this.generateUniqueName(
                        "__rest_vec",
                        declaredSymbols,
                        context.localScopeSymbols,
                        context.globalScopeSymbols,
                    );
                    innerCode +=
                        `${this.indent()}std::vector<jspp::AnyValue> ${restVecVar};\n`;
                    innerCode += `${this.indent()}while(true) {\n`;
                    this.indentationLevel++;
                    const resVar = this.generateUniqueName(
                        "__res",
                        declaredSymbols,
                        context.localScopeSymbols,
                        context.globalScopeSymbols,
                    );
                    innerCode +=
                        `${this.indent()}auto ${resVar} = ${nextVar}.call(${iterVar}, {}, "next");\n`;
                    innerCode +=
                        `${this.indent()}if (jspp::is_truthy(${resVar}.get_own_property("done"))) break;\n`;
                    innerCode +=
                        `${this.indent()}${restVecVar}.push_back(${resVar}.get_own_property("value"));\n`;
                    this.indentationLevel--;
                    innerCode += `${this.indent()}}\n`;
                    innerCode += genAssignment(
                        target,
                        `jspp::AnyValue::make_array(std::move(${restVecVar}))`,
                    );
                } else {
                    const resVar = this.generateUniqueName(
                        "__res",
                        declaredSymbols,
                        context.localScopeSymbols,
                        context.globalScopeSymbols,
                    );
                    innerCode +=
                        `${this.indent()}auto ${resVar} = ${nextVar}.call(${iterVar}, {}, "next");\n`;

                    let elementValueCode: string;
                    if (initializer) {
                        const valVar = this.generateUniqueName(
                            "__val",
                            declaredSymbols,
                            context.localScopeSymbols,
                            context.globalScopeSymbols,
                        );
                        innerCode +=
                            `${this.indent()}auto ${valVar} = (jspp::is_truthy(${resVar}.get_own_property("done")) ? jspp::Constants::UNDEFINED : ${resVar}.get_own_property("value"));\n`;
                        const initCode = this.visit(initializer, context);
                        elementValueCode =
                            `(${valVar}.is_undefined() ? ${initCode} : ${valVar})`;
                    } else {
                        elementValueCode =
                            `(jspp::is_truthy(${resVar}.get_own_property("done")) ? jspp::Constants::UNDEFINED : ${resVar}.get_own_property("value"))`;
                    }
                    innerCode += genAssignment(target, elementValueCode);
                }
            });
            return innerCode;
        } else if (
            ts.isObjectBindingPattern(pattern) ||
            ts.isObjectLiteralExpression(pattern)
        ) {
            let innerCode = "";
            const properties = ts.isObjectBindingPattern(pattern)
                ? pattern.elements
                : pattern.properties;

            const seenKeys: string[] = [];

            properties.forEach((prop) => {
                let target: ts.Node;
                let propertyName: string | undefined;
                let initializer: ts.Expression | undefined;
                let isRest = false;

                if (ts.isBindingElement(prop)) {
                    if (prop.dotDotDotToken) {
                        isRest = true;
                        target = prop.name;
                    } else {
                        target = prop.name;
                        propertyName = prop.propertyName
                            ? visitObjectPropertyName.call(
                                this,
                                prop.propertyName,
                                context,
                            )
                            : (ts.isIdentifier(prop.name)
                                ? `"${prop.name.text}"`
                                : undefined);
                        initializer = prop.initializer;
                    }
                } else if (ts.isPropertyAssignment(prop)) {
                    target = prop.initializer;
                    propertyName = visitObjectPropertyName.call(
                        this,
                        prop.name,
                        context,
                    );
                } else if (ts.isShorthandPropertyAssignment(prop)) {
                    target = prop.name;
                    propertyName = `"${prop.name.text}"`;
                } else if (ts.isSpreadAssignment(prop)) {
                    isRest = true;
                    target = prop.expression;
                } else {
                    return;
                }

                if (isRest) {
                    const keysArray = `{${
                        seenKeys.map((k) => `std::string(${k})`).join(", ")
                    }}`;
                    const restValueCode =
                        `jspp::Access::get_rest_object(${valueCode}, std::vector<std::string>${keysArray})`;
                    innerCode += genAssignment(target, restValueCode);
                } else {
                    if (propertyName) {
                        seenKeys.push(propertyName);
                        let elementValueCode: string;
                        if (initializer) {
                            const valVar = this.generateUniqueName(
                                "__val",
                                declaredSymbols,
                                context.localScopeSymbols,
                                context.globalScopeSymbols,
                            );
                            innerCode +=
                                `${this.indent()}auto ${valVar} = ${valueCode}.get_own_property(${propertyName});\n`;
                            const initCode = this.visit(initializer, context);
                            elementValueCode =
                                `(${valVar}.is_undefined() ? ${initCode} : ${valVar})`;
                        } else {
                            elementValueCode =
                                `${valueCode}.get_own_property(${propertyName})`;
                        }
                        innerCode += genAssignment(target, elementValueCode);
                    }
                }
            });
            return innerCode;
        }
        return "";
    };

    code += genAssignment(lhs, sourceVar);

    code += `${this.indent()}return ${sourceVar};\n`;
    this.indentationLevel--;
    code += `${this.indent()}})()`;

    return code;
}
