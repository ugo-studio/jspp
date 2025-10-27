import * as ts from "typescript";

import { Scope } from "../analysis/scope";
import { TypeAnalyzer, type TypeInfo } from "../analysis/typeAnalyzer";
import type { Node } from "../ast/types";
// @ts-ignore
import prelude from "../library/prelude.h" with { type: "text" };

export class CodeGenerator {
    private indentationLevel: number = 0;
    private typeAnalyzer!: TypeAnalyzer;

    private getScopeForNode(node: ts.Node): Scope {
        let current: ts.Node | undefined = node;
        while (current) {
            const scope = this.typeAnalyzer.nodeToScope.get(current);
            if (scope) {
                return scope;
            }
            current = current.parent;
        }
        const rootScope = this.typeAnalyzer.scopeManager.getAllScopes()[0];
        if (!rootScope) {
            throw new Error("Compiler bug: Could not find a root scope.");
        }
        return rootScope;
    }

    /**
     * Main entry point for the code generation process.
     */
    public generate(ast: Node, analyzer: TypeAnalyzer): string {
        this.typeAnalyzer = analyzer;

        const declarations = "\n";

        let mainCode = "int main() {\n";
        this.indentationLevel++;
        // The entire script is treated as a single function body now.
        mainCode += this.visit(ast, {
            isMainContext: true,
            isInsideFunction: true,
            isFunctionBody: true,
        });
        this.indentationLevel--;
        mainCode += "  return 0;\n}\n";

        return prelude + declarations + mainCode;
    }

    private generateLambda(
        node: ts.ArrowFunction | ts.FunctionDeclaration,
        isAssignment: boolean = false,
    ): string {
        let lambda = "[=]";

        const params = node.parameters.map((p) => `auto ${p.name.getText()}`)
            .join(", ");
        lambda += `(${params}) mutable -> JsVariant `;

        if (node.body) {
            if (ts.isBlock(node.body)) {
                lambda += this.visit(node.body, {
                    isMainContext: false,
                    isInsideFunction: true,
                    isFunctionBody: true,
                });
            } else {
                lambda += `{ return ${
                    this.visit(node.body, {
                        isMainContext: false,
                        isInsideFunction: true,
                        isFunctionBody: false,
                    })
                }; }`;
            }
        } else {
            lambda += "{ return undefined; }\n";
        }

        const signature = `JsVariant(${
            node.parameters.map(() => "JsVariant").join(", ")
        })`;
        const fullExpression = `std::function<${signature}>(${lambda})`;

        if (ts.isFunctionDeclaration(node) && !isAssignment) {
            const funcName = node.name?.getText();
            if (funcName) {
                return `${this.indent()}auto ${funcName} = ${fullExpression};\n`;
            }
        }
        return fullExpression;
    }

    /**
     * The core recursive visitor that translates a TypeScript AST node into a C++ code string.
     */
    private visit(
        node: Node,
        context: {
            isMainContext: boolean;
            isInsideFunction: boolean;
            isFunctionBody: boolean;
            isAssignmentOnly?: boolean;
        },
    ): string {
        if (ts.isFunctionDeclaration(node)) {
            if (context.isInsideFunction) {
                // This will now be handled by the Block visitor for hoisting.
                // However, we still need to generate the lambda for assignment.
                // The block visitor will wrap this in an assignment.
                return this.generateLambda(node as ts.FunctionDeclaration);
            }
            return "";
        }

        switch (node.kind) {
            case ts.SyntaxKind.ArrowFunction:
                return this.generateLambda(node as ts.ArrowFunction);

            case ts.SyntaxKind.SourceFile: {
                // Treat the source file as the top-level block
                const sourceFile = node as ts.SourceFile;
                let code = "";
                const varDecls = sourceFile.statements
                    .filter(ts.isVariableStatement)
                    .flatMap((stmt) => stmt.declarationList.declarations);

                const funcDecls = sourceFile.statements.filter(
                    ts.isFunctionDeclaration,
                );

                // 1. Hoist all variable and function declarations
                varDecls.forEach((decl) => {
                    const name = decl.name.getText();
                    code +=
                        `${this.indent()}auto ${name} = std::make_shared<JsVariant>(undefined);\n`;
                });
                funcDecls.forEach((func) => {
                    const funcName = func.name?.getText();
                    if (funcName) {
                        code +=
                            `${this.indent()}auto ${funcName} = std::make_shared<JsVariant>(undefined);\n`;
                    }
                });

                // 2. Assign all hoisted functions first
                funcDecls.forEach((stmt) => {
                    const funcName = stmt.name?.getText();
                    if (funcName) {
                        const lambda = this.generateLambda(stmt, true);
                        code += `${this.indent()}*${funcName} = ${lambda};\n`;
                    }
                });

                // 3. Process other statements
                sourceFile.statements.forEach((stmt) => {
                    if (ts.isFunctionDeclaration(stmt)) {
                        // Already handled
                    } else if (ts.isVariableStatement(stmt)) {
                        const assignmentContext = {
                            ...context,
                            isAssignmentOnly: true,
                        };
                        const assignments = this.visit(
                            stmt.declarationList,
                            assignmentContext,
                        );
                        if (assignments) {
                            code += `${this.indent()}${assignments};\n`;
                        }
                    } else {
                        code += this.visit(stmt, context);
                    }
                });
                return code;
            }

            case ts.SyntaxKind.Block: {
                let code = "{\n";
                this.indentationLevel++;
                const block = node as ts.Block;

                const varDecls = block.statements
                    .filter(ts.isVariableStatement)
                    .flatMap((stmt) => stmt.declarationList.declarations);

                const funcDecls = block.statements.filter(
                    ts.isFunctionDeclaration,
                );

                // 1. Hoist all variable and function declarations
                varDecls.forEach((decl) => {
                    const name = decl.name.getText();
                    code +=
                        `${this.indent()}auto ${name} = std::make_shared<JsVariant>(undefined);\n`;
                });
                funcDecls.forEach((func) => {
                    const funcName = func.name?.getText();
                    if (funcName) {
                        code +=
                            `${this.indent()}auto ${funcName} = std::make_shared<JsVariant>(undefined);\n`;
                    }
                });

                // 2. Assign all hoisted functions first
                funcDecls.forEach((stmt) => {
                    const funcName = stmt.name?.getText();
                    if (funcName) {
                        const lambda = this.generateLambda(stmt, true);
                        code += `${this.indent()}*${funcName} = ${lambda};\n`;
                    }
                });

                // 3. Process other statements
                block.statements.forEach((stmt) => {
                    if (ts.isFunctionDeclaration(stmt)) {
                        // Do nothing, already handled
                    } else if (ts.isVariableStatement(stmt)) {
                        const assignmentContext = {
                            ...context,
                            isAssignmentOnly: true,
                        };
                        const assignments = this.visit(
                            stmt.declarationList,
                            assignmentContext,
                        );
                        if (assignments) {
                            code += `${this.indent()}${assignments};\n`;
                        }
                    } else {
                        code += this.visit(stmt, context);
                    }
                });

                if (context.isFunctionBody) {
                    const lastStatement =
                        block.statements[block.statements.length - 1];
                    if (
                        !lastStatement || !ts.isReturnStatement(lastStatement)
                    ) {
                        code += `${this.indent()}return undefined;\n`;
                    }
                }

                this.indentationLevel--;
                code += `${this.indent()}}\n`;
                return code;
            }

            case ts.SyntaxKind.VariableStatement:
                return this.indent() +
                    this.visit(
                        (node as ts.VariableStatement).declarationList,
                        context,
                    ) + ";\n";

            case ts.SyntaxKind.VariableDeclarationList:
                return (node as ts.VariableDeclarationList).declarations.map(
                    (d) => this.visit(d, context),
                ).filter(Boolean).join(", ");

            case ts.SyntaxKind.VariableDeclaration: {
                const varDecl = node as ts.VariableDeclaration;
                const name = varDecl.name.getText();

                let initializer = "";
                if (varDecl.initializer) {
                    if (ts.isArrowFunction(varDecl.initializer)) {
                        const arrowFunc = varDecl.initializer;
                        const signature = `JsVariant(${
                            arrowFunc.parameters.map(() => "JsVariant").join(
                                ", ",
                            )
                        })`;
                        initializer = ` = std::function<${signature}>(${
                            this.visit(arrowFunc, context)
                        })`;
                    } else {
                        initializer = " = " +
                            this.visit(varDecl.initializer, context);
                    }
                }

                if (context.isAssignmentOnly) {
                    if (!initializer) return "";
                    return `*${name}${initializer}`;
                } else {
                    const initValue = initializer
                        ? initializer.substring(3)
                        : "undefined";
                    return `auto ${name} = std::make_shared<JsVariant>(${initValue})`;
                }
            }

            case ts.SyntaxKind.ObjectLiteralExpression: {
                const props = (node as ts.ObjectLiteralExpression).properties
                    .map((prop) => {
                        if (ts.isPropertyAssignment(prop)) {
                            return this.visit(prop.initializer, context);
                        }
                        return "";
                    })
                    .join(", ");
                return `{${props}}`;
            }

            case ts.SyntaxKind.ArrayLiteralExpression: {
                const elements = (node as ts.ArrayLiteralExpression).elements
                    .map((elem) => this.visit(elem, context))
                    .join(", ");
                return `{${elements}}`;
            }

            case ts.SyntaxKind.ForOfStatement: {
                const forOf = node as ts.ForOfStatement;
                let varName = "";
                if (
                    ts.isVariableDeclarationList(forOf.initializer) &&
                    forOf.initializer.declarations[0]
                ) {
                    varName = forOf.initializer.declarations[0].name.getText();
                }
                let code = `${this.indent()}for (const auto& ${varName} : ${
                    this.visit(forOf.expression, context)
                }) `;
                code += this.visit(forOf.statement, context);
                return code;
            }

            case ts.SyntaxKind.IfStatement: {
                const ifStmt = node as ts.IfStatement;
                const condition = this.visit(ifStmt.expression, context);
                const thenStmt = this.visit(ifStmt.thenStatement, {
                    ...context,
                    isFunctionBody: false,
                });
                let elseStmt = "";
                if (ifStmt.elseStatement) {
                    elseStmt = " else " +
                        this.visit(ifStmt.elseStatement, {
                            ...context,
                            isFunctionBody: false,
                        });
                }
                // return `${this.indent()}if (${condition}) ${thenStmt}${elseStmt}`;
                return `${this.indent()}if (std::any_cast<bool>(${condition})) ${thenStmt}${elseStmt}`;
            }

            case ts.SyntaxKind.PrefixUnaryExpression: {
                const prefixUnaryExpr = node as ts.PrefixUnaryExpression;
                const operand = this.visit(prefixUnaryExpr.operand, context);
                const operator = ts.tokenToString(prefixUnaryExpr.operator);
                return `${operator}${operand}`;
            }

            case ts.SyntaxKind.PropertyAccessExpression: {
                const propAccess = node as ts.PropertyAccessExpression;
                return `(*${
                    this.visit(propAccess.expression, context)
                }).${propAccess.name.getText()}`;
            }

            case ts.SyntaxKind.ExpressionStatement:
                return this.indent() +
                    this.visit(
                        (node as ts.ExpressionStatement).expression,
                        context,
                    ) + ";\n";

            case ts.SyntaxKind.BinaryExpression: {
                const binExpr = node as ts.BinaryExpression;
                let op = binExpr.operatorToken.getText();
                if (
                    binExpr.operatorToken.kind ===
                        ts.SyntaxKind.EqualsEqualsEqualsToken
                ) {
                    op = "==";
                }

                const leftText = this.visit(binExpr.left, context);
                const rightText = this.visit(binExpr.right, context);

                if (binExpr.operatorToken.kind === ts.SyntaxKind.EqualsToken) {
                    if (ts.isArrowFunction(binExpr.right)) {
                        const arrowFunc = binExpr.right;
                        const signature = `JsVariant(${
                            arrowFunc.parameters.map(() => "JsVariant").join(
                                ", ",
                            )
                        })`;
                        const lambda = this.visit(arrowFunc, context);
                        return `*${leftText} ${op} std::function<${signature}>(${lambda})`;
                    }

                    if (ts.isBinaryExpression(binExpr.right)) {
                        return `*${leftText} ${op} ${
                            this.visit(binExpr.right, context)
                        }`;
                    }

                    return `*${leftText} ${op} ${rightText}`;
                }

                const leftIsIdentifier = ts.isIdentifier(binExpr.left);
                const rightIsIdentifier = ts.isIdentifier(binExpr.right);

                const scope = this.getScopeForNode(node);
                const leftTypeInfo = leftIsIdentifier
                    ? this.typeAnalyzer.scopeManager.lookupFromScope(
                        leftText,
                        scope,
                    )
                    : null;
                const rightTypeInfo = rightIsIdentifier
                    ? this.typeAnalyzer.scopeManager.lookupFromScope(
                        rightText,
                        scope,
                    )
                    : null;

                const finalLeft =
                    leftIsIdentifier && leftTypeInfo &&
                        !leftTypeInfo.isParameter
                        ? `(*${leftText})`
                        : leftText;
                const finalRight =
                    rightIsIdentifier && rightTypeInfo &&
                        !rightTypeInfo.isParameter
                        ? `(*${rightText})`
                        : rightText;

                if (op === "+" || op === "-" || op === "*") {
                    return `(${finalLeft} ${op} ${finalRight})`;
                }
                return `${finalLeft} ${op} ${finalRight}`;
            }

            case ts.SyntaxKind.CallExpression: {
                const callExpr = node as ts.CallExpression;
                const callee = callExpr.expression;
                const args = callExpr.arguments.map((arg) => {
                    const argText = this.visit(arg, context);
                    if (ts.isIdentifier(arg)) {
                        const scope = this.getScopeForNode(arg);
                        const typeInfo = this.typeAnalyzer.scopeManager
                            .lookupFromScope(arg.text, scope);
                        if (typeInfo && !typeInfo.isParameter) {
                            return `*${argText}`;
                        }
                    }
                    return argText;
                }).join(", ");

                if (
                    ts.isPropertyAccessExpression(callee) &&
                    this.visit(callee.expression, context) === "console"
                ) {
                    const methodName = callee.name.getText();
                    return `console.${methodName}(${args})`;
                }

                const paramTypes = callExpr.arguments.map(() => "JsVariant")
                    .join(", ");
                const calleeCode = this.visit(callee, context);
                return `std::any_cast<std::function<JsVariant(${paramTypes})>>(*${calleeCode})(${args})`;
            }

            case ts.SyntaxKind.ReturnStatement: {
                const returnStmt = node as ts.ReturnStatement;
                if (returnStmt.expression) {
                    const expr = returnStmt.expression;
                    const exprText = this.visit(expr, context);
                    if (ts.isIdentifier(expr)) {
                        const scope = this.getScopeForNode(expr);
                        const typeInfo = this.typeAnalyzer.scopeManager
                            .lookupFromScope(exprText, scope);
                        if (typeInfo && !typeInfo.isParameter) {
                            return `${this.indent()}return *${exprText};\n`;
                        }
                    }
                    return `${this.indent()}return ${exprText};\n`;
                }
                return `${this.indent()}return undefined;\n`;
            }

            case ts.SyntaxKind.Identifier: {
                const identifier = node as ts.Identifier;
                return identifier.text;
            }
            case ts.SyntaxKind.NumericLiteral:
                return (node as ts.NumericLiteral).text;
            case ts.SyntaxKind.StringLiteral:
                return `"${(node as ts.StringLiteral).text}"`;
            case ts.SyntaxKind.TrueKeyword:
                return "true";
            case ts.SyntaxKind.FalseKeyword:
                return "false";
            case ts.SyntaxKind.VoidExpression: {
                const voidExpr = node as ts.VoidExpression;
                const exprText = this.visit(voidExpr.expression, context);
                return `(${exprText}, undefined)`;
            }
            case ts.SyntaxKind.UndefinedKeyword:
                return "undefined";
            case ts.SyntaxKind.NullKeyword:
                return "null";

            default:
                return `/* Unhandled node: ${ts.SyntaxKind[node.kind]} */`;
        }
    }

    private indent() {
        return "  ".repeat(this.indentationLevel);
    }
}
