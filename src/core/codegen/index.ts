import { TypeAnalyzer } from "../../analysis/typeAnalyzer";
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
  isBuiltinObject,
  isGeneratorFunction,
  prepareScopeSymbolsForVisit,
} from "./helpers";
import { visit } from "./visitor";

const CONTAINER_FUNCTION_NAME = "__container__";

export class CodeGenerator {
    public indentationLevel: number = 0;
    public typeAnalyzer!: TypeAnalyzer;
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
    public prepareScopeSymbolsForVisit = prepareScopeSymbolsForVisit;

    // function handlers
    public generateLambda = generateLambda;

    /**
     * Main entry point for the code generation process.
     */
    public generate(ast: Node, analyzer: TypeAnalyzer): string {
        this.typeAnalyzer = analyzer;

        const declarations = `#include "index.hpp"\n\n`;

        let containerCode = `jspp::AnyValue ${CONTAINER_FUNCTION_NAME}() {\n`;
        this.indentationLevel++;
        containerCode += `${this.indent()}jspp::AnyValue __this_val__ = global;\n`;
        containerCode += this.visit(ast, {
            isMainContext: true,
            isInsideFunction: true,
            isFunctionBody: true,
            topLevelScopeSymbols: new Map(),
            localScopeSymbols: new Map(),
        });
        this.indentationLevel--;
        containerCode += "  return jspp::AnyValue::make_undefined();\n";
        containerCode += "}\n\n";

        let mainCode = "int main() {\n";
        mainCode += `  std::ios::sync_with_stdio(false);\n`;
        mainCode += `  std::cin.tie(nullptr);\n`;
        mainCode += `  try {\n`;
        mainCode += `    ${CONTAINER_FUNCTION_NAME}();\n`;
        mainCode += `  } catch (const std::exception& ex) {\n`;
        mainCode +=
            "    auto error = std::make_shared<jspp::AnyValue>(jspp::RuntimeError::error_to_value(ex));\n{\n";
        mainCode +=
            `    ([&](){ auto __obj = console; return __obj.get_own_property("error").as_function("console.error")->call(__obj, {*error}); })();\n`;
        mainCode += `    return 1;\n}\n`;
        mainCode += `  }\n`;
        mainCode += "  return 0;\n}";

        return declarations + containerCode + mainCode;
    }
}
