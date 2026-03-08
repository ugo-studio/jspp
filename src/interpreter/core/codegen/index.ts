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
  prepareScopeSymbolsForVisit,
  validateFunctionParams,
} from "./helpers.js";
import { visit } from "./visitor.js";

export class CodeGenerator {
    public indentationLevel: number = 0;
    public typeAnalyzer!: TypeAnalyzer;
    public isTypescript = false;
    public moduleFunctionName!: string;
    public globalThisVar!: string;
    public uniqueNameCounter = 0;

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
    ): string {
        this.typeAnalyzer = analyzer;
        this.isTypescript = isTypescript;
        this.moduleFunctionName = this.generateUniqueName(
            "__module_entry_point_",
            this.getDeclaredSymbols(ast),
        );
        this.globalThisVar = this.generateUniqueName(
            "__this_val__",
            this.getDeclaredSymbols(ast),
        );

        const declarations =
            `#include "jspp.hpp"\n#include "library/global_usings.hpp"\n\n`;

        // module function code
        let moduleCode = `jspp::JsPromise ${this.moduleFunctionName}() {\n`;
        this.indentationLevel++;
        moduleCode +=
            `${this.indent()}jspp::AnyValue ${this.globalThisVar} = global;\n`;
        moduleCode += this.visit(ast, {
            currentScopeNode: ast,
            isMainContext: true,
            isInsideFunction: true,
            isFunctionBody: true,
            isInsideAsyncFunction: true,
            globalScopeSymbols: new DeclaredSymbols(),
            localScopeSymbols: new DeclaredSymbols(),
        });
        moduleCode += `${this.indent()}co_return jspp::Constants::UNDEFINED;\n`;
        this.indentationLevel--;
        moduleCode += "}\n\n";

        // main function code
        let mainCode = "int main(int argc, char** argv) {\n";
        this.indentationLevel++;
        mainCode += `${this.indent()}try {\n`;
        this.indentationLevel++;
        mainCode += `${this.indent()}jspp::initialize_runtime();\n`;
        mainCode += `${this.indent()}jspp::setup_process_argv(argc, argv);\n`;
        mainCode += `${this.indent()}auto p = ${this.moduleFunctionName}();\n`;
        mainCode +=
            `${this.indent()}p.then(nullptr, [](jspp::AnyValue err) {\n`;
        this.indentationLevel++;
        mainCode +=
            `${this.indent()}auto error = std::make_shared<jspp::AnyValue>(err);\n`;
        mainCode +=
            `${this.indent()}console.call_own_property("error", std::span<const jspp::AnyValue>((const jspp::AnyValue[]){*error}, 1));\n`;
        mainCode += `${this.indent()}std::exit(1);\n`;
        this.indentationLevel--;
        mainCode += `${this.indent()}});\n`;
        mainCode += `${this.indent()}jspp::Scheduler::instance().run();\n`;
        this.indentationLevel--;
        mainCode += `${this.indent()}} catch (const std::exception& ex) {\n`;
        this.indentationLevel++;
        mainCode +=
            `${this.indent()}auto error = std::make_shared<jspp::AnyValue>(jspp::Exception::exception_to_any_value(ex));\n`;
        mainCode +=
            `${this.indent()}console.call_own_property("error", std::span<const jspp::AnyValue>((const jspp::AnyValue[]){*error}, 1));\n`;
        mainCode += `${this.indent()}return 1;\n`;
        this.indentationLevel--;
        mainCode += `${this.indent()}}\n`;
        mainCode += `${this.indent()}return 0;\n`;
        this.indentationLevel--;
        mainCode += `}`;

        return declarations + moduleCode + mainCode;
    }
}
