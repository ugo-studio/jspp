import { TypeAnalyzer } from "../../analysis/typeAnalyzer";
import { DeclaredSymbols } from "../../ast/symbols";
import type { Node } from "../../ast/types";
import { generateLambda } from "./function-handlers";
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
  markSymbolAsChecked,
  prepareScopeSymbolsForVisit,
} from "./helpers";
import { visit } from "./visitor";

const CONTAINER_FUNCTION_NAME = "__container__";

export class CodeGenerator {
    public indentationLevel: number = 0;
    public typeAnalyzer!: TypeAnalyzer;
    public globalThisVar!: string;
    public exceptionCounter = 0;

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
    public markSymbolAsChecked = markSymbolAsChecked;

    // function handlers
    public generateLambda = generateLambda;

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

        let containerCode = `jspp::AnyValue ${CONTAINER_FUNCTION_NAME}(int argc, char** argv) {\n`;
        this.indentationLevel++;
        containerCode +=
            `${this.indent()}jspp::setup_process_argv(argc, argv);\n`;
        containerCode +=
            `${this.indent()}jspp::AnyValue ${this.globalThisVar} = global;\n`;
        containerCode += this.visit(ast, {
            isMainContext: true,
            isInsideFunction: true,
            isFunctionBody: true,
            topLevelScopeSymbols: new DeclaredSymbols(),
            localScopeSymbols: new DeclaredSymbols(),
        });
        this.indentationLevel--;
        containerCode += "  return jspp::Constants::UNDEFINED;\n";
        containerCode += "}\n\n";

        let mainCode = "int main(int argc, char** argv) {\n";
        // std::ios::sync_with_stdio(false); // Removed to fix console output buffering
        // std::cin.tie(nullptr);            // Removed to fix console output buffering
        mainCode += `  try {\n`;
        mainCode += `    ${CONTAINER_FUNCTION_NAME}(argc, argv);\n`;
        mainCode += `    jspp::Scheduler::instance().run();\n`;
        mainCode += `  } catch (const std::exception& ex) {\n`;
        mainCode +=
            "    auto error = std::make_shared<jspp::AnyValue>(jspp::Exception::exception_to_any_value(ex));\n{\n";
        mainCode +=
            `    ([&](){ auto __obj = console; return __obj.get_own_property("error").call(__obj, std::span<const jspp::AnyValue>((const jspp::AnyValue[]){*error}, 1), "console.error"); })();\n`;
        mainCode += `    return 1;\n}\n`;
        mainCode += `  }\n`;
        mainCode += "  return 0;\n}";

        return declarations + containerCode + mainCode;
    }
}
