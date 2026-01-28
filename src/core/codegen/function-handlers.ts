import ts from "typescript";

import { DeclarationType, DeclaredSymbols } from "../../ast/symbols.js";
import { CompilerError } from "../error.js";
import { collectFunctionScopedDeclarations } from "./helpers.js";
import { CodeGenerator } from "./index.js";
import type { VisitContext } from "./visitor.js";

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
        nativeName?: string;
        generateOnlyLambda?: boolean;
    },
): string {
    const capture = options?.capture || "[=]";
    const nativeName = options?.nativeName;

    const declaredSymbols = this.getDeclaredSymbols(node);
    const argsName = this.generateUniqueName(
        "__args_",
        declaredSymbols,
        context.globalScopeSymbols,
        context.localScopeSymbols,
    );

    const isInsideGeneratorFunction = this.isGeneratorFunction(node);
    const isInsideAsyncFunction = this.isAsyncFunction(node);
    const returnCmd = this.getReturnCommand({
        isInsideGeneratorFunction: isInsideGeneratorFunction,
        isInsideAsyncFunction: isInsideAsyncFunction,
    });
    const funcReturnType = (isInsideGeneratorFunction && isInsideAsyncFunction)
        ? "jspp::JsAsyncIterator<jspp::AnyValue>"
        : (isInsideGeneratorFunction
            ? "jspp::JsIterator<jspp::AnyValue>"
            : (isInsideAsyncFunction ? "jspp::JsPromise" : "jspp::AnyValue"));

    const isArrow = ts.isArrowFunction(node);

    // Lambda arguments are ALWAYS const references/spans to avoid copy overhead for normal functions.
    // For generators/async, we manually copy them inside the body.
    const paramThisType = "const jspp::AnyValue&";
    const paramArgsType = "std::span<const jspp::AnyValue>";

    const selfParamPart = nativeName ? `this auto&& ${nativeName}, ` : "";
    const mutablePart = nativeName ? "" : "mutable ";

    const thisArgParam = isArrow
        ? "const jspp::AnyValue&" // Arrow functions use captured 'this' or are never generators in this parser context
        : `${paramThisType} ${this.globalThisVar}`;

    let lambda =
        `${capture}(${selfParamPart}${thisArgParam}, ${paramArgsType} ${argsName}) ${mutablePart}-> ${funcReturnType} `;

    const globalScopeSymbols = this.prepareScopeSymbolsForVisit(
        context.globalScopeSymbols,
        context.localScopeSymbols,
    );

    const visitContext: VisitContext = {
        currentScopeNode: context.currentScopeNode,
        isMainContext: false,
        isInsideFunction: true,
        isFunctionBody: false,
        lambdaName: undefined,
        globalScopeSymbols,
        localScopeSymbols: new DeclaredSymbols(),
        superClassVar: context.superClassVar,
        isInsideGeneratorFunction: isInsideGeneratorFunction,
        isInsideAsyncFunction: isInsideAsyncFunction,
    };

    // Preamble to copy arguments for coroutines
    let preamble = "";
    if (isInsideGeneratorFunction || isInsideAsyncFunction) {
        const thisCopy = this.generateUniqueName(
            "__this_copy",
            declaredSymbols,
        );
        const argsCopy = this.generateUniqueName(
            "__args_copy",
            declaredSymbols,
        );

        if (!isArrow) {
            preamble +=
                `${this.indent()}jspp::AnyValue ${thisCopy} = ${this.globalThisVar};\n`;
        }
        preamble +=
            `${this.indent()}std::vector<jspp::AnyValue> ${argsCopy}(${argsName}.begin(), ${argsName}.end());\n`;
    }

    // Adjust parameter names for generator/async to allow shadowing/copying
    let finalThisParamName = isArrow ? "" : this.globalThisVar;
    let finalArgsParamName = argsName;

    if (isInsideGeneratorFunction || isInsideAsyncFunction) {
        if (!isArrow) {
            finalThisParamName = this.generateUniqueName(
                "__this_ref",
                declaredSymbols,
            );
        }
        finalArgsParamName = this.generateUniqueName(
            "__args_ref",
            declaredSymbols,
        );
    }

    const thisArgParamFinal = isArrow
        ? "const jspp::AnyValue&"
        : `${paramThisType} ${finalThisParamName}`;

    // Re-construct lambda header with potentially new param names
    lambda =
        `${capture}(${selfParamPart}${thisArgParamFinal}, ${paramArgsType} ${finalArgsParamName}) ${mutablePart}-> ${funcReturnType} `;

    // Regenerate preamble
    preamble = "";
    if (isInsideGeneratorFunction || isInsideAsyncFunction) {
        if (!isArrow) {
            preamble +=
                `${this.indent()}jspp::AnyValue ${this.globalThisVar} = ${finalThisParamName};\n`;
        }
        preamble +=
            `${this.indent()}std::vector<jspp::AnyValue> ${argsName}(${finalArgsParamName}.begin(), ${finalArgsParamName}.end());\n`;
    }

    // Now 'argsName' refers to the vector (if copied) or the span (if not).
    // And 'this.globalThisVar' refers to the copy (if copied) or the param (if not).
    // So subsequent code using `argsName` and `visit` (using `globalThisVar`) works correctly.

    const paramExtractor = (
        parameters: ts.NodeArray<ts.ParameterDeclaration>,
    ): string => {
        let code = "";
        parameters.filter((p) =>
            this.isTypescript && p.name.getText() === "this" ? false : true // Ignore "this" parameters
        ).forEach((p, i) => {
            const name = p.name.getText();
            const defaultValue = p.initializer
                ? this.visit(p.initializer, visitContext)
                : "jspp::Constants::UNDEFINED";

            // Catch invalid parameters
            if (name === "this") {
                throw new CompilerError(
                    `Cannot use '${name}' as a parameter name.`,
                    p,
                    "SyntaxError",
                );
            }

            // Add paramerter to local context
            visitContext.localScopeSymbols.add(name, {
                type: DeclarationType.let,
                checks: { initialized: true },
            });

            const scope = this.getScopeForNode(p);
            const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                name,
                scope,
            )!;

            // Handle rest parameters
            if (!!p.dotDotDotToken) {
                if (parameters.length - 1 !== i) {
                    throw new CompilerError(
                        "Rest parameter must be last formal parameter.",
                        p,
                        "SyntaxError",
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
            // Hoist var declarations in the function body
            const varDecls = collectFunctionScopedDeclarations(node.body);
            varDecls.forEach((decl) => {
                preamble += this.hoistDeclaration(
                    decl,
                    visitContext.localScopeSymbols,
                    node.body as ts.Node,
                );
            });

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
            // The block visitor already adds braces, so we need to inject the preamble and param extraction.
            lambda += "{\n" + preamble + paramExtraction +
                blockContent.trimStart().substring(2);
        } else {
            lambda += "{\n";
            this.indentationLevel++;
            lambda += preamble;
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
        lambda += `{ ${preamble} ${returnCmd} jspp::Constants::UNDEFINED; }\n`;
    }

    // Return only lambda if required
    if (options?.generateOnlyLambda) {
        return lambda.trimEnd();
    }

    return this.generateFullLambdaExpression(node, context, lambda, options);
}

export function generateFullLambdaExpression(
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
    lambda: string,
    options?: {
        isAssignment?: boolean;
        isClass?: boolean;
        noTypeSignature?: boolean;
    },
) {
    const isAssignment = options?.isAssignment || false;
    const noTypeSignature = options?.noTypeSignature || false;

    const isInsideGeneratorFunction = this.isGeneratorFunction(node);
    const isInsideAsyncFunction = this.isAsyncFunction(node);
    const isArrow = ts.isArrowFunction(node);

    let callable = lambda;
    let method = "";

    // Handle generator function
    if (isInsideGeneratorFunction) {
        if (isInsideAsyncFunction) {
            if (!noTypeSignature) {
                const signature =
                    "jspp::JsAsyncIterator<jspp::AnyValue>(const jspp::AnyValue&, std::span<const jspp::AnyValue>)";
                callable = `std::function<${signature}>(${lambda})`;
            }
            method = `jspp::AnyValue::make_async_generator`;
        } else {
            if (!noTypeSignature) {
                const signature =
                    "jspp::JsIterator<jspp::AnyValue>(const jspp::AnyValue&, std::span<const jspp::AnyValue>)";
                callable = `std::function<${signature}>(${lambda})`;
            }
            method = `jspp::AnyValue::make_generator`;
        }
    } // Handle async function
    else if (isInsideAsyncFunction) {
        if (!noTypeSignature) {
            const signature =
                "jspp::JsPromise(const jspp::AnyValue&, std::span<const jspp::AnyValue>)";
            callable = `std::function<${signature}>(${lambda})`;
        }
        method = `jspp::AnyValue::make_async_function`;
    } // Handle normal function
    else {
        if (!noTypeSignature) {
            const signature =
                `jspp::AnyValue(const jspp::AnyValue&, std::span<const jspp::AnyValue>)`;
            callable = `std::function<${signature}>(${lambda})`;
        }
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
