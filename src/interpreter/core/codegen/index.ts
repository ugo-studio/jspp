import ts from "typescript";

import type { TypeAnalyzer } from "../../analysis/typeAnalyzer.js";
import { DeclaredSymbols } from "../../ast/symbols.js";
import type { Node } from "../../ast/types.js";
import { generateDestructuring } from "./destructuring-handlers.js";
import {
  generateLambdaComponents,
  generateNativeLambda,
  generateWrappedLambda,
} from "./function-handlers.js";
import {
  escapeString,
  generateUniqueExceptionName,
  generateUniqueName,
  getDeclaredSymbols,
  getDerefCode,
  getJsVarName,
  getReturnCommand,
  getScopeForNode,
  hoistDeclaration,
  indent,
  isAsyncFunction,
  isBuiltinObject,
  isDeclarationCalledAsFunction,
  isDeclarationUsedAsValue,
  isDeclarationUsedBeforeInitialization,
  isGeneratorFunction,
  isVariableUsedWithoutDeclaration,
  markSymbolAsInitialized,
  needsTopLevelAwait,
  prepareScopeSymbolsForVisit,
  validateFunctionParams,
} from "./helpers.js";
import { visit, type VisitContext } from "./visitor.js";

export class CodeGenerator {
    public indentationLevel: number = 0;
    public typeAnalyzer!: TypeAnalyzer;
    public isTypescript = false;
    public moduleFunctionName!: string;
    public globalThisVar!: string;
    public uniqueNameCounter = 0;
    public isWasm = false;
    public wasmExports: {
        jsName: string;
        nativeName: string;
        params: ts.ParameterDeclaration[];
    }[] = [];

    // visitor
    public visit = visit;

    // helpers
    public getDeclaredSymbols = getDeclaredSymbols;
    public generateUniqueName = generateUniqueName;
    public generateUniqueExceptionName = generateUniqueExceptionName;
    public hoistDeclaration = hoistDeclaration;
    public getScopeForNode = getScopeForNode;
    public indent = indent;
    public escapeString = escapeString;
    public getJsVarName = getJsVarName;
    public getDerefCode = getDerefCode;
    public getReturnCommand = getReturnCommand;
    public isBuiltinObject = isBuiltinObject;
    public isGeneratorFunction = isGeneratorFunction;
    public isAsyncFunction = isAsyncFunction;
    public prepareScopeSymbolsForVisit = prepareScopeSymbolsForVisit;
    public markSymbolAsInitialized = markSymbolAsInitialized;
    public isDeclarationCalledAsFunction = isDeclarationCalledAsFunction;
    public isDeclarationUsedAsValue = isDeclarationUsedAsValue;
    public isDeclarationUsedBeforeInitialization =
        isDeclarationUsedBeforeInitialization;
    public isVariableUsedWithoutDeclaration = isVariableUsedWithoutDeclaration;
    public validateFunctionParams = validateFunctionParams;
    public generateDestructuring = generateDestructuring;

    // function handlers
    public generateLambdaComponents = generateLambdaComponents;
    public generateNativeLambda = generateNativeLambda;
    public generateWrappedLambda = generateWrappedLambda;

    /**
     * Main entry point for the code generation process.
     */
    public generate(
        ast: Node,
        analyzer: TypeAnalyzer,
        isTypescript: boolean,
        isWasm: boolean = false,
    ): string {
        this.typeAnalyzer = analyzer;
        this.isTypescript = isTypescript;
        this.isWasm = isWasm;
        this.wasmExports = [];
        this.moduleFunctionName = this.generateUniqueName(
            "__module_entry_point_",
            this.getDeclaredSymbols(ast),
        );
        this.globalThisVar = this.generateUniqueName(
            "__this_val__",
            this.getDeclaredSymbols(ast),
        );

        const isAsyncModule = needsTopLevelAwait(ast);
        const moduleReturnType = isAsyncModule
            ? "jspp::JsPromise"
            : "jspp::AnyValue";
        const moduleReturnCommand = isAsyncModule ? "co_return" : "return";

        let declarations =
            `#include "jspp.hpp"\n#include "library/global_usings.hpp"\n`;

        if (isWasm) {
            declarations += `#include <emscripten.h>\n`;
        }
        declarations += `\n`;

        // module function code
        let moduleCode = `${moduleReturnType} ${this.moduleFunctionName}() {\n`;
        this.indentationLevel++;
        moduleCode +=
            `${this.indent()}jspp::AnyValue ${this.globalThisVar} = global;\n`;

        const context: VisitContext = {
            currentScopeNode: ast,
            isMainContext: true,
            isInsideFunction: true,
            isFunctionBody: true,
            isInsideAsyncFunction: isAsyncModule,
            globalScopeSymbols: new DeclaredSymbols(),
            localScopeSymbols: new DeclaredSymbols(),
        };

        const generatedBody = this.visit(ast, context);
        moduleCode += generatedBody;
        moduleCode +=
            `${this.indent()}${moduleReturnCommand} jspp::Constants::UNDEFINED;\n`;
        this.indentationLevel--;
        moduleCode += "}\n\n";

        // Wasm Exports
        let wasmGlobalPointers = "";
        let wasmWrappers = "";

        if (isWasm) {
            for (const exp of this.wasmExports) {
                const paramTypes = exp.params.map((_, i) => `jspp::AnyValue`);
                const paramList = paramTypes.length > 0 ? `, ${paramTypes.join(", ")}` : "";
                const pointerName = `__wasm_export_ptr_${exp.jsName}`;
                wasmGlobalPointers += `std::function<jspp::AnyValue(jspp::AnyValue${paramList})> ${pointerName} = nullptr;\n`;

                const wrapperParamList = exp.params.map((_, i) => `double p${i}`).join(", ");
                const callArgsList = exp.params.map((_, i) => `jspp::AnyValue::make_number(p${i})`);
                const callArgs = callArgsList.length > 0 ? `, ${callArgsList.join(", ")}` : "";

                wasmWrappers += `extern "C" EMSCRIPTEN_KEEPALIVE\n`;
                wasmWrappers += `double wasm_export_${exp.jsName}(${wrapperParamList}) {\n`;
                wasmWrappers += `    if (!${pointerName}) return 0;\n`;
                wasmWrappers += `    auto res = ${pointerName}(global${callArgs});\n`;
                wasmWrappers += `    return jspp::Operators_Private::ToNumber(res);\n`;
                wasmWrappers += `}\n\n`;
            }
        }

        // main function code
        let mainCode = "int main(int argc, char** argv) {\n";
        this.indentationLevel++;
        mainCode += `${this.indent()}try {\n`;
        this.indentationLevel++;
        mainCode += `${this.indent()}jspp::initialize_runtime();\n`;
        mainCode += `${this.indent()}jspp::setup_process_argv(argc, argv);\n`;

        if (isAsyncModule) {
            mainCode +=
                `${this.indent()}auto p = ${this.moduleFunctionName}();\n`;
            mainCode +=
                `${this.indent()}p.then(nullptr, [](jspp::AnyValue err) {\n`;
            this.indentationLevel++;
            mainCode +=
                `${this.indent()}auto error = std::make_shared<jspp::AnyValue>(err);\n`;
            this.indentationLevel++;
            mainCode +=
                `${this.indent()}console.call_own_property("error", std::span<const jspp::AnyValue>((const jspp::AnyValue[]){*error}, 1));\n`;
            mainCode += `${this.indent()}std::exit(1);\n`;
            this.indentationLevel--;
            mainCode += `${this.indent()}});\n`;
        } else {
            mainCode += `${this.indent()}${this.moduleFunctionName}();\n`;
        }

        mainCode += `${this.indent()}jspp::Scheduler::instance().run();\n`;
        this.indentationLevel--;
        mainCode += `${this.indent()}} catch (const std::exception& ex) {\n`;
        this.indentationLevel++;
        mainCode +=
            `${this.indent()}auto error = std::make_shared<jspp::AnyValue>(jspp::Exception::exception_to_any_value(ex));\n`;
        this.indentationLevel++;
        mainCode +=
            `${this.indent()}console.call_own_property("error", std::span<const jspp::AnyValue>((const jspp::AnyValue[]){*error}, 1));\n`;
        mainCode += `${this.indent()}return 1;\n`;
        this.indentationLevel--;
        mainCode += `${this.indent()}}\n`;
        mainCode += `${this.indent()}return 0;\n`;
        this.indentationLevel--;
        mainCode += `}`;

        return declarations + wasmGlobalPointers + wasmWrappers + moduleCode +
            mainCode;
    }
}
