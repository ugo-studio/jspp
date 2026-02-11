import ts from "typescript";

import { DeclarationType, DeclaredSymbols } from "../../ast/symbols.js";
import { collectFunctionScopedDeclarations } from "./helpers.js";
import { CodeGenerator } from "./index.js";
import type { VisitContext } from "./visitor.js";

export function generateLambdaComponents(
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
        noTypeSignature?: boolean;
    },
) {
    const declaredSymbols = this.getDeclaredSymbols(node);
    const argsName = this.generateUniqueName(
        "__args_",
        declaredSymbols,
        context.globalScopeSymbols,
        context.localScopeSymbols,
    );

    const isInsideGeneratorFunction = this.isGeneratorFunction(node);
    const isInsideAsyncFunction = this.isAsyncFunction(node);
    const isArrow = ts.isArrowFunction(node);

    const returnCommand = this.getReturnCommand({
        isInsideGeneratorFunction: isInsideGeneratorFunction,
        isInsideAsyncFunction: isInsideAsyncFunction,
    });
    const funcReturnType = (isInsideGeneratorFunction && isInsideAsyncFunction)
        ? "jspp::JsAsyncIterator<jspp::AnyValue>"
        : (isInsideGeneratorFunction
            ? "jspp::JsIterator<jspp::AnyValue>"
            : (isInsideAsyncFunction ? "jspp::JsPromise" : "jspp::AnyValue"));

    // Lambda arguments are ALWAYS const references/spans to avoid copy overhead for normal functions.
    // For generators/async, we manually copy them inside the body.
    const paramThisType = "const jspp::AnyValue&";
    const paramArgsType = "std::span<const jspp::AnyValue>";

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

    const thisArgParam = isArrow
        ? "const jspp::AnyValue&" // Arrow functions use captured 'this' or are never generators in this parser context
        : `${paramThisType} ${finalThisParamName}`;
    const funcArgs = `${paramArgsType} ${finalArgsParamName}`;

    // Preamble to copy arguments for coroutines
    let preamble = "";
    let nativePreamble = "";
    if (isInsideGeneratorFunction || isInsideAsyncFunction) {
        if (!isArrow) {
            preamble +=
                `${this.indent()}jspp::AnyValue ${this.globalThisVar} = ${finalThisParamName};\n`;
            nativePreamble +=
                `${this.indent()}jspp::AnyValue ${this.globalThisVar} = ${finalThisParamName};\n`;
        }
        // Note: Do not add argument copy to native lambda
        preamble +=
            `${this.indent()}std::vector<jspp::AnyValue> ${argsName}(${finalArgsParamName}.begin(), ${finalArgsParamName}.end());\n`;
    }

    // Native function arguments for native lambda
    let nativeFuncArgs = "";
    let nativeParamsContent = "";
    this.validateFunctionParams(node.parameters).forEach((p, i) => {
        const name = p.name.getText();
        const defaultValue = p.initializer
            ? this.visit(p.initializer, visitContext)
            : !!p.dotDotDotToken
            ? "jspp::AnyValue::make_array(std::vector<jspp::AnyValue>{})"
            : "jspp::Constants::UNDEFINED";
        nativeFuncArgs += `, const jspp::AnyValue& ${name} = ${defaultValue}`;
    });

    // Extract lambda parameters from arguments span/vector
    const generateParamsBuilder = () => {
        let paramsCode = "";
        this.validateFunctionParams(node.parameters).forEach(
            (p, i) => {
                const name = p.name.getText();
                const defaultValue = p.initializer
                    ? this.visit(p.initializer, visitContext)
                    : "jspp::Constants::UNDEFINED";

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
                    const initValue =
                        `jspp::AnyValue::make_array(${argsName}.subspan(${i}))`;
                    if (typeInfo.needsHeapAllocation) {
                        paramsCode +=
                            `${this.indent()}auto ${name} = std::make_shared<jspp::AnyValue>(${initValue});\n`;
                    } else {
                        paramsCode +=
                            `${this.indent()}jspp::AnyValue ${name} = ${initValue};\n`;
                    }
                    return;
                }

                // Normal parameter
                const initValue =
                    `${argsName}.size() > ${i} ? ${argsName}[${i}] : ${defaultValue}`;
                if (typeInfo && typeInfo.needsHeapAllocation) {
                    paramsCode +=
                        `${this.indent()}auto ${name} = std::make_shared<jspp::AnyValue>(${initValue});\n`;
                } else {
                    paramsCode +=
                        `${this.indent()}jspp::AnyValue ${name} = ${initValue};\n`;
                }
            },
        );
        return paramsCode;
    };

    // Generate params and function body
    let paramsContent = "";
    let blockContentWithoutOpeningBrace = "";

    if (node.body) {
        if (ts.isBlock(node.body)) {
            // Hoist var declarations in the function body
            const varDecls = collectFunctionScopedDeclarations(node.body);
            varDecls.forEach((decl) => {
                const hoistCode = this.hoistDeclaration(
                    decl,
                    visitContext.localScopeSymbols,
                    node.body as ts.Node,
                );
                preamble += hoistCode;
                nativePreamble += hoistCode;
            });

            this.indentationLevel++;
            paramsContent = generateParamsBuilder();
            this.indentationLevel--;

            // The block visitor already adds braces, so we need to remove the opening brace to inject the preamble and param extraction.
            blockContentWithoutOpeningBrace = this.visit(node.body, {
                ...visitContext,
                isMainContext: false,
                isInsideFunction: true,
                isFunctionBody: true,
                isInsideGeneratorFunction: isInsideGeneratorFunction,
                isInsideAsyncFunction: isInsideAsyncFunction,
            }).trimStart().substring(2);
        } else {
            this.indentationLevel++;
            paramsContent = generateParamsBuilder();
            blockContentWithoutOpeningBrace =
                `${this.indent()}${returnCommand} ${
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
            blockContentWithoutOpeningBrace += `${this.indent()}}`;
        }
    } else {
        blockContentWithoutOpeningBrace =
            `${returnCommand} jspp::Constants::UNDEFINED; }\n`;
    }

    return {
        node,
        context,
        options,
        visitContext,
        thisArgParam,
        funcArgs,
        nativeFuncArgs,
        funcReturnType,
        preamble,
        nativePreamble,
        paramsContent,
        nativeParamsContent,
        blockContentWithoutOpeningBrace,
        isInsideGeneratorFunction,
        isInsideAsyncFunction,
    };
}

export function generateNativeLambda(
    this: CodeGenerator,
    comps: ReturnType<typeof generateLambdaComponents>,
) {
    const {
        options,
        thisArgParam,
        nativeFuncArgs,
        funcReturnType,
        nativePreamble,
        nativeParamsContent,
        blockContentWithoutOpeningBrace,
    } = comps;

    const capture = options?.capture || "[=]";
    const nativeName = options?.nativeName;

    const selfParam = nativeName ? `this auto&& ${nativeName}, ` : "";
    const mutableLabel = nativeName ? "" : "mutable ";

    let lambda =
        `${capture}(${selfParam}${thisArgParam}${nativeFuncArgs}) ${mutableLabel}-> ${funcReturnType} {\n`;
    lambda += nativePreamble;
    lambda += nativeParamsContent;
    lambda += blockContentWithoutOpeningBrace;

    return lambda;
}

export function generateWrappedLambda(
    this: CodeGenerator,
    comps: ReturnType<typeof generateLambdaComponents>,
) {
    const {
        node,
        context,
        options,
        thisArgParam,
        funcArgs,
        funcReturnType,
        preamble,
        paramsContent,
        blockContentWithoutOpeningBrace,
        isInsideGeneratorFunction,
        isInsideAsyncFunction,
    } = comps;

    const capture = options?.capture || "[=]";
    const isAssignment = options?.isAssignment || false;
    const noTypeSignature = options?.noTypeSignature || false;

    let lambda =
        `${capture}(${thisArgParam}, ${funcArgs}) mutable -> ${funcReturnType} {\n`;
    lambda += preamble;
    lambda += paramsContent;
    lambda += blockContentWithoutOpeningBrace;

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

    const isArrow = ts.isArrowFunction(node);
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
        return this.generateWrappedLambda(
            this.generateLambdaComponents(
                node as ts.FunctionDeclaration,
                context,
            ),
        );
    }
    return "";
}

export function visitArrowFunction(
    this: CodeGenerator,
    node: ts.ArrowFunction,
    context: VisitContext,
): string {
    return this.generateWrappedLambda(
        this.generateLambdaComponents(node as ts.ArrowFunction, context),
    );
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
        const lambda = this.generateWrappedLambda(
            this.generateLambdaComponents(funcExpr, {
                ...context,
                lambdaName: funcName,
            }, {
                isAssignment: true,
                capture: "[=]",
            }),
        );
        code += `${this.indent()}*${funcName} = ${lambda};\n`;
        code += `${this.indent()}return *${funcName};\n`;
        this.indentationLevel--;
        code += `${this.indent()}})()`;
        return code;
    }
    return this.generateWrappedLambda(
        this.generateLambdaComponents(node as ts.FunctionExpression, context),
    );
}
