import * as ts from "typescript";

import { TypeAnalyzer, type TypeInfo } from "../analysis/typeAnalyzer";
import type { Node } from "../ast/types";
// @ts-ignore
import prelude from "../library/prelude.h" with { type: "text" };

export class CodeGenerator {
    private indentationLevel: number = 0;
    private typeAnalyzer!: TypeAnalyzer;
    private ast!: Node;

    /**
     * Main entry point for the code generation process.
     */
    public generate(ast: Node, analyzer: TypeAnalyzer): string {
        this.ast = ast;
        this.typeAnalyzer = analyzer;

        const allFuncs = this.collectFunctions(ast);

        const topLevelFuncs = allFuncs.filter((f) =>
            f.parent.kind === ts.SyntaxKind.SourceFile
        );

        const closures = topLevelFuncs.filter((f) =>
            this.typeAnalyzer.functionTypeInfo.get(f)?.isClosure
        );
        const regularFuncs = topLevelFuncs.filter((f) =>
            !this.typeAnalyzer.functionTypeInfo.get(f)?.isClosure
        );

        let declarations = this.generateStructs() + "\n"; // This now works

        // Topological sort
        const sortedFuncs: ts.FunctionDeclaration[] = [];
        const visited = new Set<string>();
        const visiting = new Set<string>(); // for cycle detection

        const visitFunc = (funcNode: ts.FunctionDeclaration) => {
            const funcName = funcNode.name!.getText();
            if (visited.has(funcName)) return;
            if (visiting.has(funcName)) {
                console.error(
                    `Cycle detected in function calls involving ${funcName}`,
                );
                return;
            }

            visiting.add(funcName);

            const dependencies =
                this.typeAnalyzer.dependencyGraph.get(funcName) || [];
            dependencies.forEach((depName) => {
                const depNode = regularFuncs.find((f) =>
                    f.name?.getText() === depName
                );
                if (depNode) {
                    visitFunc(depNode);
                }
            });

            visiting.delete(funcName);
            visited.add(funcName);
            sortedFuncs.push(funcNode);
        };

        regularFuncs.forEach(visitFunc);

        closures.forEach((funcNode) => {
            declarations += this.generateFunctionOrClosure(funcNode) + "\n";
        });

        sortedFuncs.forEach((funcNode) => {
            declarations += this.generateFunctionOrClosure(funcNode) + "\n";
        });

        let mainCode = "int main() {\n";
        this.indentationLevel++;
        mainCode += this.visit(ast, {
            isMainContext: true,
            isInsideFunction: false,
            isFunctionBody: false,
        });
        this.indentationLevel--;
        mainCode += "  return 0;\n}\n";

        return prelude + declarations + mainCode;
    }

    /**
     * Generates all C++ struct definitions based on the TypeAnalyzer's findings.
     */
    private generateStructs(): string {
        const structDefs = new Set<string>();
        for (const scope of this.typeAnalyzer.scopeManager.getAllScopes()) {
            for (const info of scope.symbols.values()) {
                if (
                    info.type === "struct" && info.structName && info.properties
                ) {
                    let structDef = `struct ${info.structName} {\n`;
                    for (
                        const [propName, propType] of info.properties.entries()
                    ) {
                        structDef += `  ${propType} ${propName};\n`;
                    }
                    structDef += "};\n";
                    structDefs.add(structDef);
                }
            }
        }
        return Array.from(structDefs).join("\n");
    }

    /**
     * Generates the C++ code for a given JavaScript function or closure.
     */
    private generateFunctionOrClosure(node: ts.FunctionDeclaration): string {
        const info = this.typeAnalyzer.functionTypeInfo.get(node);
        const funcName = node.name?.getText() ?? "anonymous";
        const returnType = "auto";
        const params = node.parameters.map((p) => `auto ${p.name.getText()}`)
            .join(", ");

        const isTopLevel = node.parent.kind === ts.SyntaxKind.SourceFile;

        if (info?.isClosure || isTopLevel) {
            const captures = info?.captures;
            const captureNames = captures ? [...captures.keys()] : [];
            const hasCaptures = captureNames.length > 0;

            let code = "";
            if (hasCaptures) {
                const templateParams = captureNames.map((_, i) =>
                    `typename T${i}`
                ).join(", ");
                code += `template<${templateParams}>\n`;
            }
            code += `struct ${funcName}_functor {\n`;

            this.indentationLevel++;

            if (hasCaptures) {
                captureNames.forEach((name, i) => {
                    code += `${this.indent()}T${i}& ${name};\n`;
                });

                const constructorParams = captureNames.map((name, i) =>
                    `T${i}& ${name}_arg`
                ).join(", ");
                const initializers = captureNames.map((name) =>
                    `${name}(${name}_arg)`
                ).join(", ");
                code +=
                    `\n${this.indent()}${funcName}_functor(${constructorParams}) : ${initializers} {}\n\n`;
            } else {
                code += `\n${this.indent()}${funcName}_functor() {}\n\n`;
            }

            code += `${this.indent()}${returnType} operator()(${params}) `;
            if (node.body) {
                code += this.visit(node.body, {
                    isMainContext: false,
                    isInsideFunction: true,
                    isFunctionBody: true,
                });
            } else {
                code += "{ return undefined; }\n";
            }
            this.indentationLevel--;
            code += `};\n`;
            return code;
        } else {
            let code = `${returnType} ${funcName}(${params}) `;
            if (node.body) {
                code += this.visit(node.body, {
                    isMainContext: false,
                    isInsideFunction: true,
                    isFunctionBody: true,
                });
            } else {
                code += "{ return undefined; }\n";
            }
            return code;
        }
    }

    private generateLambda(
        node: ts.ArrowFunction | ts.FunctionDeclaration,
        isAssignment: boolean = false,
    ): string {
        const info = this.typeAnalyzer.functionTypeInfo.get(node);
        const captures = info?.captures ? [...info.captures.keys()] : [];

        let lambda = "[";
        if (captures.length > 0) {
            lambda += captures.join(", ");
        } else {
            lambda += "&";
        }
        lambda += "]";

        const params = node.parameters.map((p) => `auto ${p.name.getText()}`)
            .join(", ");
        lambda += `(${params}) mutable `;

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

            case ts.SyntaxKind.SourceFile:
                return (node as ts.SourceFile).statements.map((stmt) =>
                    this.visit(stmt, context)
                ).join("");

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
                varDecls.forEach(decl => {
                    const name = decl.name.getText();
                    if (this.typeAnalyzer.capturedVariables.has(decl)) {
                        code += `${this.indent()}auto ${name} = std::make_shared<JsVariant>(undefined);\n`;
                    } else {
                        code += `${this.indent()}JsVariant ${name} = undefined;\n`;
                    }
                });
                funcDecls.forEach(func => {
                    const funcName = func.name?.getText();
                    if (funcName) {
                        code += `${this.indent()}JsVariant ${funcName} = undefined;\n`;
                    }
                });

                // 2. Assign all hoisted functions first
                funcDecls.forEach((stmt) => {
                    const funcName = stmt.name?.getText();
                    if (funcName) {
                        const lambda = this.generateLambda(stmt, true);
                        code += `${this.indent()}${funcName} = ${lambda};\n`;
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
                const isCaptured = this.typeAnalyzer.capturedVariables.has(varDecl);

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
                } else if (!context.isAssignmentOnly) { // Add undefined only if declaring
                    initializer = " = undefined";
                }

                if (context.isAssignmentOnly) {
                    // Only return the assignment part, and only if there is one.
                    if (!initializer) return "";
                    if (isCaptured) {
                        return `*${name}${initializer}`;
                    }
                    return `${name}${initializer}`;
                } else {
                    // Full declaration
                    if (isCaptured) {
                        const initValue = initializer ? initializer.substring(3) : "undefined";
                        return `auto ${name} = std::make_shared<JsVariant>(${initValue})`;
                    }
                    const type = "JsVariant";
                    return `${type} ${name}${initializer}`;
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

            case ts.SyntaxKind.PropertyAccessExpression: {
                const propAccess = node as ts.PropertyAccessExpression;
                return `${
                    this.visit(propAccess.expression, context)
                }.${propAccess.name.getText()}`;
            }

            case ts.SyntaxKind.ExpressionStatement:
                return this.indent() +
                    this.visit(
                        (node as ts.ExpressionStatement).expression,
                        context,
                    ) + ";\n";

            case ts.SyntaxKind.BinaryExpression: {
                const binExpr = node as ts.BinaryExpression;
                const op = binExpr.operatorToken.kind ===
                        ts.SyntaxKind.EqualsEqualsEqualsToken
                    ? "=="
                    : binExpr.operatorToken.getText();

                if (binExpr.operatorToken.kind === ts.SyntaxKind.EqualsToken) {
                    if (ts.isArrowFunction(binExpr.right)) {
                        const arrowFunc = binExpr.right;
                        const signature = `JsVariant(${
                            arrowFunc.parameters.map(() => "JsVariant").join(
                                ", ",
                            )
                        })`;
                        const lambda = this.visit(arrowFunc, context);
                        return `${
                            this.visit(binExpr.left, context)
                        } ${op} std::function<${signature}>(${lambda})`;
                    }

                    const leftText = this.visit(binExpr.left, context);
                    const rightText = this.visit(binExpr.right, context);

                    return `${leftText} ${op} ${rightText}`;
                }

                return `${this.visit(binExpr.left, context)} ${op} ${
                    this.visit(binExpr.right, context)
                }`;
            }

            case ts.SyntaxKind.CallExpression: {
                const callExpr = node as ts.CallExpression;
                const callee = callExpr.expression;
                const args = callExpr.arguments.map((arg) =>
                    this.visit(arg, context)
                ).join(", ");

                if (
                    ts.isPropertyAccessExpression(callee) &&
                    callee.expression.getText() === "console"
                ) {
                    const methodName = callee.name.getText();
                    return `console.${methodName}(${args})`;
                }

                if (ts.isIdentifier(callee)) {
                    const declaration = this.findDeclarationNode(callee);

                    if (
                        declaration &&
                        ts.isFunctionDeclaration(declaration)
                    ) {
                        const funcInfo = this.typeAnalyzer.functionTypeInfo.get(
                            declaration,
                        );
                        if (
                            declaration.parent.kind ===
                                ts.SyntaxKind.SourceFile
                        ) {
                            const calleeName = callee.getText();
                            if (funcInfo?.isClosure) {
                                const capturedArgs = [
                                    ...funcInfo.captures!.keys(),
                                ].join(", ");
                                const instanceName = `${calleeName}_instance`;
                                return `auto ${instanceName} = ${calleeName}_functor(${capturedArgs});\n${this.indent()}${instanceName}(${args})`;
                            } else {
                                return `${calleeName}_functor()(${args})`;
                            }
                        }
                    }
                }

                const paramTypes = callExpr.arguments.map(() => "JsVariant")
                    .join(", ");
                const calleeCode = this.visit(callee, context);
                return `std::any_cast<std::function<JsVariant(${paramTypes})>>(${calleeCode})(${args})`;
            }

            case ts.SyntaxKind.ReturnStatement: {
                const returnStmt = node as ts.ReturnStatement;
                if (returnStmt.expression) {
                    const exprText = this.visit(returnStmt.expression, context);
                    return `${this.indent()}return ${exprText};\n`;
                }
                return `${this.indent()}return undefined;\n`;
            }

            case ts.SyntaxKind.Identifier: {
                const identifier = node as ts.Identifier;
                const declaration = this.findDeclarationNode(identifier);

                if (
                    declaration &&
                    this.typeAnalyzer.capturedVariables.has(declaration)
                ) {
                     return `(*${identifier.text})`;
                }

                if (
                    declaration &&
                    ts.isFunctionDeclaration(declaration) &&
                    declaration.parent.kind === ts.SyntaxKind.SourceFile
                ) {
                    // If the identifier is being called, don't wrap it in std::function.
                    // The CallExpression handler will deal with it.
                    if (
                        identifier.parent &&
                        ts.isCallExpression(identifier.parent) &&
                        identifier.parent.expression === identifier
                    ) {
                        return identifier.text;
                    }

                    const funcInfo = this.findFunctionInfo(declaration);
                    if (funcInfo) {
                        const signature = `JsVariant(${
                            declaration.parameters
                                .map(() => "JsVariant")
                                .join(", ")
                        })`;
                        return `std::function<${signature}>(${identifier.text}_functor())`;
                    }
                }
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

    private collectFunctions(node: Node): ts.FunctionDeclaration[] {
        const funcs: ts.FunctionDeclaration[] = [];
        const visitor = (child: Node) => {
            if (ts.isFunctionDeclaration(child)) {
                funcs.push(child);
            }
            ts.forEachChild(child, visitor);
        };
        ts.forEachChild(node, visitor);
        return funcs;
    }

    private findDeclarationNode(
        identifier: ts.Identifier,
    ): ts.Node | undefined {
        return this.typeAnalyzer.identifierToDeclaration.get(identifier);
    }

    private findFunctionInfo(
        node: ts.FunctionDeclaration,
    ): TypeInfo | undefined {
        return this.typeAnalyzer.functionTypeInfo.get(node);
    }
}
