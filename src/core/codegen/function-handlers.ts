import ts from "typescript";

import { CodeGenerator } from "./";
import type { VisitContext } from "./visitor";

export function generateLambda(
    this: CodeGenerator,
    node: ts.ArrowFunction | ts.FunctionDeclaration | ts.FunctionExpression,
    context: VisitContext,
    isAssignment: boolean = false,
    capture: string = "[=]",
): string {
    const declaredSymbols = this.getDeclaredSymbols(node);
    const argsName = this.generateUniqueName("__args_", declaredSymbols);

    const isInsideGeneratorFunction = this.isGeneratorFunction(node);
    const returnCmd = this.getReturnCommand({
        isInsideGeneratorFunction: isInsideGeneratorFunction,
    });
    const funcReturnType = isInsideGeneratorFunction
        ? "jspp::JsIterator<jspp::AnyValue>"
        : "jspp::AnyValue";

    let lambda =
        `${capture}(const std::vector<jspp::AnyValue>& ${argsName}) mutable -> ${funcReturnType} `;

    const topLevelScopeSymbols = this.prepareScopeSymbolsForVisit(
        context.topLevelScopeSymbols,
        context.localScopeSymbols,
    );

    const visitContext: VisitContext = {
        isMainContext: false,
        isInsideFunction: true,
        isFunctionBody: false,
        topLevelScopeSymbols,
        localScopeSymbols: new Map(),
    };

    if (node.body) {
        if (ts.isBlock(node.body)) {
            let paramExtraction = "";
            this.indentationLevel++;
            node.parameters.forEach((p, i) => {
                const name = p.name.getText();
                const defaultValue = p.initializer
                    ? this.visit(p.initializer, visitContext)
                    : "jspp::AnyValue::make_undefined()";
                paramExtraction +=
                    `${this.indent()}auto ${name} = ${argsName}.size() > ${i} ? ${argsName}[${i}] : ${defaultValue};\n`;
            });
            this.indentationLevel--;

            const blockContent = this.visit(node.body, {
                ...visitContext,
                isMainContext: false,
                isInsideFunction: true,
                isFunctionBody: true,
                isInsideGeneratorFunction: isInsideGeneratorFunction,
            });
            // The block visitor already adds braces, so we need to inject the param extraction.
            lambda += "{\n" + paramExtraction + blockContent.substring(2);
        } else {
            lambda += "{\n";
            this.indentationLevel++;
            node.parameters.forEach((p, i) => {
                const name = p.name.getText();
                const defaultValue = p.initializer
                    ? this.visit(p.initializer, visitContext)
                    : "jspp::AnyValue::make_undefined()";
                lambda +=
                    `${this.indent()}auto ${name} = ${argsName}.size() > ${i} ? ${argsName}[${i}] : ${defaultValue};\n`;
            });
            lambda += `${this.indent()}${returnCmd} ${
                this.visit(node.body, {
                    ...visitContext,
                    isMainContext: false,
                    isInsideFunction: true,
                    isFunctionBody: false,
                })
            };
`;
            this.indentationLevel--;
            lambda += `${this.indent()}}`;
        }
    } else {
        lambda += `{ ${returnCmd} jspp::AnyValue::make_undefined(); }\n`;
    }

    let signature = "";
    let callable = "";
    let method = "";

    // Handle generator function
    if (isInsideGeneratorFunction) {
        signature =
            "jspp::JsIterator<jspp::AnyValue>(const std::vector<jspp::AnyValue>&)";
        callable = `std::function<${signature}>(${lambda})`;
        method = `jspp::AnyValue::make_generator`;
    } // Handle normal function
    else {
        signature = `jspp::AnyValue(const std::vector<jspp::AnyValue>&)`;
        callable = `std::function<${signature}>(${lambda})`;
        method = `jspp::AnyValue::make_function`;
    }

    const funcName = node.name?.getText();
    const fullExpression = `${method}(${callable}, "${funcName || ""}")`;

    if (ts.isFunctionDeclaration(node) && !isAssignment && funcName) {
        return `${this.indent()}auto ${funcName} = ${fullExpression};\n`;
    }
    return fullExpression;
}

export function visitFunctionDeclaration(
    this: CodeGenerator,
    node: ts.FunctionDeclaration,
    context: VisitContext,
): string {
    if (context.isInsideFunction) {
        // This will now be handled by the Block visitor for hoisting.
        // However, we still need to generate the lambda for assignment.
        // The block visitor will wrap this in an assignment.
        return this.generateLambda(node as ts.FunctionDeclaration, context);
    }
    return "";
}

export function visitArrowFunction(
    this: CodeGenerator,
    node: ts.ArrowFunction,
    context: VisitContext,
): string {
    return this.generateLambda(node as ts.ArrowFunction, context);
}

export function visitFunctionExpression(
    this: CodeGenerator,
    node: ts.FunctionExpression,
    context: VisitContext,
): string {
    const funcExpr = node as ts.FunctionExpression;
    if (funcExpr.name) {
        const funcName = funcExpr.name.getText();
        let code = "([=]() -> jspp::AnyValue {\n";
        this.indentationLevel++;
        code +=
            `${this.indent()}auto ${funcName} = std::make_shared<jspp::AnyValue>();\n`;
        const lambda = this.generateLambda(funcExpr, context, true, "[=]");
        code += `${this.indent()}*${funcName} = ${lambda};\n`;
        code += `${this.indent()}return *${funcName};\n`;
        this.indentationLevel--;
        code += `${this.indent()}})()`;
        return code;
    }
    return this.generateLambda(node as ts.FunctionExpression, context);
}
