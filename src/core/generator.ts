import * as ts from "typescript";

import { TypeAnalyzer, type TypeInfo } from "../analysis/typeAnalyzer";
import type { Node } from "../ast/types";

export class CodeGenerator {
    private indentationLevel: number = 0;
    private typeAnalyzer!: TypeAnalyzer;

    /**
     * Main entry point for the code generation process.
     */
    public generate(ast: Node, analyzer: TypeAnalyzer): string {
        this.typeAnalyzer = analyzer;

        const allFuncs = this.collectFunctions(ast);

        const topLevelFuncs = allFuncs.filter(f => f.parent.kind === ts.SyntaxKind.SourceFile);

        const closures = topLevelFuncs.filter(f => this.typeAnalyzer.functionTypeInfo.get(f)?.isClosure);
        const regularFuncs = topLevelFuncs.filter(f => !this.typeAnalyzer.functionTypeInfo.get(f)?.isClosure);

        let declarations = this.generateStructs() + "\n"; // This now works

        // Topological sort
        const sortedFuncs: ts.FunctionDeclaration[] = [];
        const visited = new Set<string>();
        const visiting = new Set<string>(); // for cycle detection

        const visitFunc = (funcNode: ts.FunctionDeclaration) => {
            const funcName = funcNode.name!.getText();
            if (visited.has(funcName)) return;
            if (visiting.has(funcName)) {
                console.error(`Cycle detected in function calls involving ${funcName}`);
                return;
            }

            visiting.add(funcName);

            const dependencies = this.typeAnalyzer.dependencyGraph.get(funcName) || [];
            dependencies.forEach(depName => {
                const depNode = regularFuncs.find(f => f.name?.getText() === depName);
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
        mainCode += this.visit(ast, { isMainContext: true, isInsideFunction: false });
        this.indentationLevel--;
        mainCode += "  return 0;\n}\n";

        const includes =
            "#include <iostream>\n#include <string>\n#include <vector>\n#include <variant>\n#include <functional>\n\n";

        return includes + declarations + mainCode;
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
        const returnType = info?.returnsValue ? "auto" : "void";
        const params = node.parameters.map((p) => `auto ${p.name.getText()}`)
            .join(", ");

        if (info?.isClosure && info.captures) {
            const captureNames = [...info.captures.keys()];
            const templateParams = captureNames.map((_, i) => `typename T${i}`).join(', ');

            let code = `template<${templateParams}>\nstruct ${funcName}_functor {
`;
            this.indentationLevel++;
            captureNames.forEach((name, i) => {
                code += `${this.indent()}T${i} ${name};\n`;
            });

            const constructorParams = captureNames.map((name, i) => `T${i} ${name}_arg`).join(", ");
            const initializers = captureNames.map((name) => `${name}(${name}_arg)`).join(", ");
            code += `\n${this.indent()}${funcName}_functor(${constructorParams}) : ${initializers} {}

`;

            code += `${this.indent()}${returnType} operator()(${params}) `;
            if (node.body) {
                code += this.visit(node.body, { isMainContext: false, isInsideFunction: true });
            } else {
                code += "{}\n";
            }
            this.indentationLevel--;
            code += `};\n`;
            return code;
        } else {
            let code = `${returnType} ${funcName}(${params}) `;
            if (node.body) {
                code += this.visit(node.body, { isMainContext: false, isInsideFunction: true });
            } else {
                code += "{}\n";
            }
            return code;
        }
    }

    private generateLambda(node: ts.FunctionDeclaration): string {
        const info = this.typeAnalyzer.functionTypeInfo.get(node);
        const funcName = node.name?.getText();

        let lambda = "";
        if (info?.isClosure && info.captures) {
            const capturedArgs = [...info.captures.keys()].map(n => `&${n}`).join(', ');
            lambda += `[${capturedArgs}]`;
        } else {
            lambda += `[]`;
        }

        const params = node.parameters.map((p) => `auto ${p.name.getText()}`).join(", ");
        lambda += `(${params}) `;

        if (node.body) {
            lambda += this.visit(node.body, { isMainContext: false, isInsideFunction: true });
        } else {
            lambda += "{}\n";
        }

        if (funcName) {
            return `${this.indent()}auto ${funcName} = ${lambda};\n`;
        }
        return `${this.indent()}${lambda};\n`;
    }

    /**
     * The core recursive visitor that translates a TypeScript AST node into a C++ code string.
     */
    private visit(node: Node, context: { isMainContext: boolean; isInsideFunction: boolean }): string {
        if (ts.isFunctionDeclaration(node)) {
            if (context.isInsideFunction) {
                return this.generateLambda(node as ts.FunctionDeclaration);
            }
            return "";
        }

        switch (node.kind) {
            case ts.SyntaxKind.SourceFile:
                return (node as ts.SourceFile).statements.map((stmt) =>
                    this.visit(stmt, context)
                ).join("");

            case ts.SyntaxKind.Block: {
                let code = "{\n";
                this.indentationLevel++;
                code += (node as ts.Block).statements.map((stmt) =>
                    this.visit(stmt, context)
                ).join("");
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
                ).join(", ");

            case ts.SyntaxKind.VariableDeclaration: {
                const varDecl = node as ts.VariableDeclaration;
                const name = varDecl.name.getText();
                const info = this.typeAnalyzer.scopeManager.lookup(name);

                let type = "auto";
                if (info) {
                    type = info.structName ?? info.type;
                }

                let initializer = "";
                if (varDecl.initializer) {
                    initializer = " = " +
                        this.visit(varDecl.initializer, context);
                }
                return `${type} ${name}${initializer}`;
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
                    callee.expression.getText() === "console" &&
                    callee.name.getText() === "log"
                ) {
                    return `std::cout << ${ 
                        args.replace(/, /g, ' << " " << ') 
                    } << std::endl`;
                }

                const calleeName = callee.getText();
                const funcInfo = this.findFunctionInfo(calleeName);
                if (funcInfo?.isClosure) {
                    const capturedArgs = [...funcInfo.captures!.keys()].join(
                        ", ",
                    );
                    return `${calleeName}_functor(${capturedArgs})`;
                }

                return `${calleeName}(${args})`;
            }

            case ts.SyntaxKind.ReturnStatement: {
                const returnStmt = node as ts.ReturnStatement;
                if (returnStmt.expression) {
                    const exprText = this.visit(returnStmt.expression, context);
                    const funcInfo = this.findFunctionInfo(exprText);
                    if (funcInfo?.isClosure) {
                        const capturedArgs = [...funcInfo.captures!.keys()]
                            .join(", ");
                        return `${this.indent()}return ${exprText}_functor(${capturedArgs});\n`;
                    }
                    return `${this.indent()}return ${exprText};\n`;
                }
                return `${this.indent()}return;\n`;
            }

            case ts.SyntaxKind.Identifier:
                return (node as ts.Identifier).text;
            case ts.SyntaxKind.NumericLiteral:
                return (node as ts.NumericLiteral).text;
            case ts.SyntaxKind.StringLiteral:
                return `"${(node as ts.StringLiteral).text}"`;
            case ts.SyntaxKind.TrueKeyword:
                return "true";
            case ts.SyntaxKind.FalseKeyword:
                return "false";

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

    private findFunctionInfo(name: string): TypeInfo | undefined {
        for (
            const [funcNode, info] of this.typeAnalyzer.functionTypeInfo
                .entries()
        ) {
            if (funcNode.name?.getText() === name) {
                return info;
            }
        }
        return undefined;
    }
}
