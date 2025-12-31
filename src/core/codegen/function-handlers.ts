import ts from "typescript";

import { DeclaredSymbols } from "../../ast/symbols";
import { CodeGenerator } from "./";
import type { VisitContext } from "./visitor";

export function generateLambda(
    this: CodeGenerator,
    node:
        | ts.ArrowFunction
        | ts.FunctionDeclaration
        | ts.FunctionExpression
        | ts.ConstructorDeclaration
        | ts.MethodDeclaration
        | ts.GetAccessorDeclaration
        | ts.SetAccessorDeclaration,
    context: VisitContext,
    options?: {
        isAssignment?: boolean;
        capture?: string;
        isClass?: boolean;
    },
): string {
    const isAssignment = options?.isAssignment || false;
    const capture = options?.capture || "[=]";

    const declaredSymbols = this.getDeclaredSymbols(node);
    const argsName = this.generateUniqueName("__args_", declaredSymbols);

    const isInsideGeneratorFunction = this.isGeneratorFunction(node);
    const isInsideAsyncFunction = this.isAsyncFunction(node);
    const returnCmd = this.getReturnCommand({
        isInsideGeneratorFunction: isInsideGeneratorFunction,
        isInsideAsyncFunction: isInsideAsyncFunction,
    });
    const funcReturnType = isInsideGeneratorFunction
        ? "jspp::JsIterator<jspp::AnyValue>"
        : (isInsideAsyncFunction ? "jspp::JsPromise" : "jspp::AnyValue");

    const isArrow = ts.isArrowFunction(node);

    // For generators and async functions, we MUST copy arguments because the coroutine suspends immediately
    // and references to temporary arguments would dangle.
    const paramThisType = (isInsideGeneratorFunction || isInsideAsyncFunction)
        ? "jspp::AnyValue"
        : "const jspp::AnyValue&";
    const paramArgsType = (isInsideGeneratorFunction || isInsideAsyncFunction)
        ? "std::vector<jspp::AnyValue>"
        : "std::span<const jspp::AnyValue>";

    const thisArgParam = isArrow
        ? "const jspp::AnyValue&" // Arrow functions are never generators in this parser
        : `${paramThisType} ${this.globalThisVar}`;

    let lambda =
        `${capture}(${thisArgParam}, ${paramArgsType} ${argsName}) mutable -> ${funcReturnType} `;

    const topLevelScopeSymbols = this.prepareScopeSymbolsForVisit(
        context.topLevelScopeSymbols,
        context.localScopeSymbols,
    );

    const visitContext: VisitContext = {
        isMainContext: false,
        isInsideFunction: true,
        isFunctionBody: false,
        lambdaName: undefined,
        topLevelScopeSymbols,
        localScopeSymbols: new DeclaredSymbols(),
        superClassVar: context.superClassVar,
    };

    const paramExtractor = (
        parameters: ts.NodeArray<ts.ParameterDeclaration>,
    ): string => {
        let code = "";
        parameters.forEach((p, i) => {
            const name = p.name.getText();
            const defaultValue = p.initializer
                ? this.visit(p.initializer, visitContext)
                : "jspp::Constants::UNDEFINED";

            const scope = this.getScopeForNode(p);
            const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                name,
                scope,
            )!;

            // Handle rest parameters
            if (!!p.dotDotDotToken) {
                if (parameters.length - 1 !== i) {
                    throw new SyntaxError(
                        "Rest parameter must be last formal parameter.",
                    );
                }

                if (typeInfo.needsHeapAllocation) {
                    code +=
                        `${this.indent()}auto ${name} = std::make_shared<jspp::AnyValue>(jspp::Constants::UNDEFINED);\n`;
                } else {
                    code +=
                        `${this.indent()}jspp::AnyValue ${name} = jspp::Constants::UNDEFINED;\n`;
                }

                // Extract rest parameters
                const tempName = `temp_${name}`;

                code += `${this.indent()}{\n`;
                this.indentationLevel++;
                code +=
                    `${this.indent()}std::vector<std::optional<jspp::AnyValue>> ${tempName};\n`;

                code += `${this.indent()}if (${argsName}.size() > ${i}) {\n`;
                this.indentationLevel++;
                code +=
                    `${this.indent()}${tempName}.reserve(${argsName}.size() - ${i});\n`;
                this.indentationLevel--;
                code += `${this.indent()}}\n`;

                code +=
                    `${this.indent()}for (size_t j = ${i}; j < ${argsName}.size(); j++) {\n`;
                this.indentationLevel++;
                code +=
                    `${this.indent()}${tempName}.push_back(${argsName}[j]);\n`;
                this.indentationLevel--;
                code += `${this.indent()}}\n`;
                code += `${this.indent()}${
                    typeInfo.needsHeapAllocation ? "*" : ""
                }${name} = jspp::AnyValue::make_array(std::move(${tempName}));\n`;
                this.indentationLevel--;
                code += `${this.indent()}}\n`;
                return;
            }

            // Normal parameter
            const initValue =
                `${argsName}.size() > ${i} ? ${argsName}[${i}] : ${defaultValue}`;
            if (typeInfo && typeInfo.needsHeapAllocation) {
                code +=
                    `${this.indent()}auto ${name} = std::make_shared<jspp::AnyValue>(${initValue});\n`;
            } else {
                code +=
                    `${this.indent()}jspp::AnyValue ${name} = ${initValue};\n`;
            }
        });
        return code;
    };

    if (node.body) {
        if (ts.isBlock(node.body)) {
            this.indentationLevel++;
            const paramExtraction = paramExtractor(node.parameters);
            this.indentationLevel--;

            const blockContent = this.visit(node.body, {
                ...visitContext,
                isMainContext: false,
                isInsideFunction: true,
                isFunctionBody: true,
                isInsideGeneratorFunction: isInsideGeneratorFunction,
                isInsideAsyncFunction: isInsideAsyncFunction,
            });
            // The block visitor already adds braces, so we need to inject the param extraction.
            lambda += "{\n" + paramExtraction + blockContent.substring(2);
        } else {
            lambda += "{\n";
            this.indentationLevel++;
            lambda += paramExtractor(node.parameters);
            lambda += `${this.indent()}${returnCmd} ${
                this.visit(node.body, {
                    ...visitContext,
                    isMainContext: false,
                    isInsideFunction: true,
                    isFunctionBody: false,
                    isInsideGeneratorFunction: isInsideGeneratorFunction,
                    isInsideAsyncFunction: isInsideAsyncFunction,
                })
            };\n`;
            this.indentationLevel--;
            lambda += `${this.indent()}}`;
        }
    } else {
        lambda += `{ ${returnCmd} jspp::Constants::UNDEFINED; }\n`;
    }

    let signature = "";
    let callable = "";
    let method = "";

    // Handle generator function
    if (isInsideGeneratorFunction) {
        signature =
            "jspp::JsIterator<jspp::AnyValue>(const jspp::AnyValue&, std::span<const jspp::AnyValue>)";
        callable = `std::function<${signature}>(${lambda})`;
        method = `jspp::AnyValue::make_generator`;
    } // Handle async function
    else if (isInsideAsyncFunction) {
        signature =
            "jspp::JsPromise(const jspp::AnyValue&, std::span<const jspp::AnyValue>)";
        callable = `std::function<${signature}>(${lambda})`;
        method = `jspp::AnyValue::make_async_function`;
    } // Handle normal function
    else {
        signature =
            `jspp::AnyValue(const jspp::AnyValue&, std::span<const jspp::AnyValue>)`;
        callable = `std::function<${signature}>(${lambda})`;
        if (options?.isClass) {
            method = `jspp::AnyValue::make_class`;
        } else {
            method = `jspp::AnyValue::make_function`;
        }
    }

    const funcName = context?.lambdaName || node.name?.getText();
    const hasName = !!funcName && funcName.length > 0;

    let args = callable;

    const isMethod = ts.isMethodDeclaration(node);
    const isAccessor = ts.isGetAccessor(node) || ts.isSetAccessor(node);
    const isConstructor = !isArrow && !isMethod && !isAccessor;

    // make_function(callable, name, is_constructor)
    if (method === `jspp::AnyValue::make_function`) {
        if (hasName) {
            args += `, "${funcName}"`;
        } else if (!isConstructor) {
            args += `, std::nullopt`;
        }

        if (!isConstructor) {
            args += `, false`;
        }
    } else {
        // make_class, make_generator, make_async_function
        if (hasName) {
            args += `, "${funcName}"`;
        }
    }

    const fullExpression = `${method}(${args})`;

    if (ts.isFunctionDeclaration(node) && !isAssignment && node.name) {
        const funcName = node.name?.getText();
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
        const lambda = this.generateLambda(funcExpr, {
            ...context,
            lambdaName: funcName,
        }, {
            isAssignment: true,
            capture: "[=]",
        });
        code += `${this.indent()}*${funcName} = ${lambda};\n`;
        code += `${this.indent()}return *${funcName};\n`;
        this.indentationLevel--;
        code += `${this.indent()}})()`;
        return code;
    }
    return this.generateLambda(node as ts.FunctionExpression, context);
}
