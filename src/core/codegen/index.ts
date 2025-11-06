import { TypeAnalyzer } from "../../analysis/typeAnalyzer";
import type { Node } from "../../ast/types";
import { generateLambda } from "./function-handlers";
import {
  escapeString,
  generateUniqueExceptionName,
  generateUniqueName,
  getDeclaredSymbols,
  getJsVarName,
  getScopeForNode,
  indent,
  isBuiltinObject,
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
    public getScopeForNode = getScopeForNode;
    public indent = indent;
    public escapeString = escapeString;
    public getJsVarName = getJsVarName;
    public isBuiltinObject = isBuiltinObject;

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
        containerCode += this.visit(ast, {
            isMainContext: true,
            isInsideFunction: true,
            isFunctionBody: true,
        });
        this.indentationLevel--;
        containerCode += "  return jspp::NonValues::undefined;\n";
        containerCode += "}\n\n";

        let mainCode = "int main() {\n";
        // mainCode += `  try {\n`;
        mainCode += `    ${CONTAINER_FUNCTION_NAME}();\n`;
        // mainCode += `  } catch (const jspp::AnyValue& e) {\n`;
        // mainCode +=
        //     "    auto error = std::make_shared<jspp::AnyValue>(jspp::Exception::parse_error_from_value(e));\n{\n";
        // mainCode +=
        //     `    jspp::Access::call_function(jspp::Access::get_property(console, "error"),{error},"console.error");\n`;
        // mainCode += `    return 1;\n}\n`;
        // mainCode += `  }\n`;
        mainCode += "  return 0;\n}";

        return declarations + containerCode + mainCode;
    }
}
