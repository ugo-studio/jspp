import type { TypeAnalyzer } from "../../analysis/typeAnalyzer.js";
import { DeclaredSymbols } from "../../ast/symbols.js";
import type { Node } from "../../ast/types.js";
import {
  generateFullLambdaExpression,
  generateLambda,
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
  isGeneratorFunction,
  markSymbolAsInitialized,
  prepareScopeSymbolsForVisit,
} from "./helpers.js";
import { visit } from "./visitor.js";

const MODULE_NAME = "__main_function__";

export class CodeGenerator {
    public indentationLevel: number = 0;
    public typeAnalyzer!: TypeAnalyzer;
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

    // function handlers
    public generateLambda = generateLambda;
    public generateFullLambdaExpression = generateFullLambdaExpression;

    /**
     * Main entry point for the code generation process.
     */
    public generate(ast: Node, analyzer: TypeAnalyzer): string {
        this.typeAnalyzer = analyzer;
        this.globalThisVar = this.generateUniqueName(
            "__this_val__",
            this.getDeclaredSymbols(ast),
        );

        const declarations = `#include "index.hpp"\n\n`;

        let containerCode = `jspp::JsPromise ${MODULE_NAME}() {\n`;
        this.indentationLevel++;
        containerCode +=
            `${this.indent()}jspp::AnyValue ${this.globalThisVar} = global;\n`;
        containerCode += this.visit(ast, {
            isMainContext: true,
            isInsideFunction: true,
            isFunctionBody: true,
            isInsideAsyncFunction: true,
            topLevelScopeSymbols: new DeclaredSymbols(),
            localScopeSymbols: new DeclaredSymbols(),
        });
        this.indentationLevel--;
        containerCode += "  co_return jspp::Constants::UNDEFINED;\n";
        containerCode += "}\n\n";

        let mainCode = "int main(int argc, char** argv) {\n";
        mainCode += `  try {\n`;
        mainCode += `    jspp::setup_process_argv(argc, argv);\n`;
        mainCode += `    auto p = ${MODULE_NAME}();\n`;
        mainCode += `    p.then(nullptr, [](const jspp::AnyValue& err) {\n`;
        mainCode +=
            `        auto error = std::make_shared<jspp::AnyValue>(err);\n`;
        mainCode +=
            `        console.call_own_property("error", std::span<const jspp::AnyValue>((const jspp::AnyValue[]){*error}, 1));\n`;
        mainCode += `        std::exit(1);\n`;
        mainCode += `    });\n`;
        mainCode += `    jspp::Scheduler::instance().run();\n`;
        mainCode += `  } catch (const std::exception& ex) {\n`;
        mainCode +=
            "    auto error = std::make_shared<jspp::AnyValue>(jspp::Exception::exception_to_any_value(ex));\n    {\n";
        mainCode +=
            `      console.call_own_property("error", std::span<const jspp::AnyValue>((const jspp::AnyValue[]){*error}, 1));\n`;
        mainCode += `      return 1;\n    }\n`;
        mainCode += `  }\n`;
        mainCode += "  return 0;\n}";

        return declarations + containerCode + mainCode;
    }
}
