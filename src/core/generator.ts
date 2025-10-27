import * as ts from "typescript";

import type { Node } from "../ast/types";

export class CodeGenerator {
    private indentationLevel: number = 0;
    private functionDeclarations: string = "";
    private mainCode: string = "";
    private currentOutput: "main" | "function" = "main";
    private symbolTable = new Map<string, string>();

    public generate(node: Node): string {
        this.visit(node);
        let finalCode =
            "#include <iostream>\n#include <string>\n#include <vector>\n\n";
        finalCode += this.functionDeclarations;
        finalCode += "\nint main() {\n";
        finalCode += this.mainCode;
        finalCode += "  return 0;\n}\n";
        return finalCode;
    }

    private indent() {
        return "  ".repeat(this.indentationLevel);
    }

    private appendTo(code: string) {
        if (this.currentOutput === "main") {
            this.mainCode += code;
        } else {
            this.functionDeclarations += code;
        }
    }

    private inferType(node: ts.Expression): string {
        if (ts.isStringLiteral(node)) {
            return "std::string";
        }
        if (ts.isNumericLiteral(node)) {
            // Basic check for float/double
            return node.text.includes(".") ? "double" : "int";
        }
        // Default or more complex inference can be added here
        return "auto";
    }

    private visit(node: Node) {
        switch (node.kind) {
            case ts.SyntaxKind.SourceFile:
                ts.forEachChild(node, (child) => this.visit(child));
                break;

            case ts.SyntaxKind.VariableStatement:
                this.visit((node as ts.VariableStatement).declarationList);
                this.appendTo(";\n");
                break;

            case ts.SyntaxKind.VariableDeclarationList:
                const varList = node as ts.VariableDeclarationList;
                varList.declarations.forEach((declaration, index) => {
                    this.visit(declaration);
                    if (index < varList.declarations.length - 1) {
                        this.appendTo(", ");
                    }
                });
                break;

            case ts.SyntaxKind.VariableDeclaration:
                const varDecl = node as ts.VariableDeclaration;
                const varName = varDecl.name.getText();
                let varType = "auto";
                if (varDecl.initializer) {
                    varType = this.inferType(varDecl.initializer);
                    this.symbolTable.set(varName, varType);
                    this.appendTo(`${this.indent()}${varType} ${varName} = `);
                    this.visit(varDecl.initializer);
                } else {
                    // Default to int for uninitialized variables, or handle differently
                    this.symbolTable.set(varName, "int");
                    this.appendTo(`${this.indent()}int ${varName}`);
                }
                break;

            case ts.SyntaxKind.FunctionDeclaration:
                this.currentOutput = "function";
                const funcDecl = node as ts.FunctionDeclaration;
                const funcName = funcDecl.name?.getText() ?? "anonymous";
                // Simple return type inference, defaults to void
                let returnType = "void";
                if (funcDecl.body) {
                    funcDecl.body.statements.forEach((statement) => {
                        if (
                            ts.isReturnStatement(statement) &&
                            statement.expression
                        ) {
                            returnType = this.inferType(statement.expression);
                        }
                    });
                }
                this.appendTo(`${returnType} ${funcName}(`);
                funcDecl.parameters.forEach((param, index) => {
                    // Defaulting parameter types to auto, a real implementation would need more info
                    this.appendTo(`auto ${param.name.getText()}`);
                    if (index < funcDecl.parameters.length - 1) {
                        this.appendTo(", ");
                    }
                });
                this.appendTo(")");
                this.visit(funcDecl.body!);
                this.appendTo("\n");
                this.currentOutput = "main";
                break;

            case ts.SyntaxKind.IfStatement:
                const ifStmt = node as ts.IfStatement;
                this.appendTo(`${this.indent()}if (`);
                this.visit(ifStmt.expression);
                this.appendTo(") ");
                this.visit(ifStmt.thenStatement);
                if (ifStmt.elseStatement) {
                    this.appendTo(" else ");
                    this.visit(ifStmt.elseStatement);
                }
                break;

            case ts.SyntaxKind.ForStatement:
                const forStmt = node as ts.ForStatement;
                this.appendTo(`${this.indent()}for (`);
                if (forStmt.initializer) this.visit(forStmt.initializer);
                this.appendTo("; ");
                if (forStmt.condition) this.visit(forStmt.condition);
                this.appendTo("; ");
                if (forStmt.incrementor) this.visit(forStmt.incrementor);
                this.appendTo(") ");
                this.visit(forStmt.statement);
                break;

            case ts.SyntaxKind.BinaryExpression:
                const binExpr = node as ts.BinaryExpression;
                this.visit(binExpr.left);
                this.appendTo(` ${binExpr.operatorToken.getText()} `);
                this.visit(binExpr.right);
                break;

            case ts.SyntaxKind.PrefixUnaryExpression: // handles i++
                const prefixUnary = node as ts.PrefixUnaryExpression;
                this.appendTo(ts.SyntaxKind[prefixUnary.operator]);
                this.visit(prefixUnary.operand);
                break;

            case ts.SyntaxKind.PostfixUnaryExpression: // handles i++
                const postfixUnary = node as ts.PostfixUnaryExpression;
                this.visit(postfixUnary.operand);
                this.appendTo(
                    postfixUnary.operator === ts.SyntaxKind.PlusPlusToken
                        ? "++"
                        : "--",
                );
                break;

            case ts.SyntaxKind.Block:
                this.appendTo("{\n");
                this.indentationLevel++;
                (node as ts.Block).statements.forEach((stmt) => {
                    this.appendTo(this.indent());
                    this.visit(stmt);
                });
                this.indentationLevel--;
                this.appendTo(`${this.indent()}}\n`);
                break;

            case ts.SyntaxKind.ExpressionStatement:
                this.visit((node as ts.ExpressionStatement).expression);
                this.appendTo(";\n");
                break;

            case ts.SyntaxKind.CallExpression:
                const callExpr = node as ts.CallExpression;
                // Special handling for console.log
                if (
                    ts.isPropertyAccessExpression(callExpr.expression) &&
                    callExpr.expression.name.getText() === "log"
                ) {
                    this.appendTo("std::cout << ");
                    callExpr.arguments.forEach((arg, index) => {
                        this.visit(arg);
                        if (index < callExpr.arguments.length - 1) {
                            this.appendTo(' << " " << ');
                        }
                    });
                    this.appendTo(" << std::endl");
                } else {
                    this.visit(callExpr.expression);
                    this.appendTo("(");
                    callExpr.arguments.forEach((arg, index) => {
                        this.visit(arg);
                        if (index < callExpr.arguments.length - 1) {
                            this.appendTo(", ");
                        }
                    });
                    this.appendTo(")");
                }
                break;

            case ts.SyntaxKind.Identifier:
                this.appendTo((node as ts.Identifier).text);
                break;
            case ts.SyntaxKind.StringLiteral:
                this.appendTo(`"${(node as ts.StringLiteral).text}"`);
                break;
            case ts.SyntaxKind.NumericLiteral:
                this.appendTo((node as ts.NumericLiteral).text);
                break;

            default:
                // For debugging unhandled nodes
                // console.log("Unhandled node type:", ts.SyntaxKind[node.kind]);
                ts.forEachChild(node, (child) => this.visit(child));
                break;
        }
    }
}
