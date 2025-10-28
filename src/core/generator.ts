import * as ts from "typescript";

import { Scope } from "../analysis/scope";
import { TypeAnalyzer } from "../analysis/typeAnalyzer";
import type { Node } from "../ast/types";
// @ts-ignore
import prelude from "../library/prelude.h" with { type: "text" };

const CONTAINER_FUNCTION_NAME = "__jspp_code_container__";

export class CodeGenerator {
    private indentationLevel: number = 0;
    private typeAnalyzer!: TypeAnalyzer;
    private exceptionCounter = 0;

    private getDeclaredSymbols(node: ts.Node): Set<string> {
        const symbols = new Set<string>();
        const visitor = (child: ts.Node) => {
            if (ts.isVariableDeclaration(child)) {
                // Handles let, const, var
                symbols.add(child.name.getText());
            } else if (ts.isFunctionDeclaration(child) && child.name) {
                // Handles function declarations
                symbols.add(child.name.getText());
            } else if (ts.isParameter(child)) {
                // Handles function parameters
                symbols.add(child.name.getText());
            } else if (ts.isCatchClause(child) && child.variableDeclaration) {
                // Handles catch clause variable
                symbols.add(child.variableDeclaration.name.getText());
            }
            ts.forEachChild(child, visitor);
        };
        visitor(node);
        return symbols;
    }

    private generateUniqueName(
        prefix: string,
        namesToAvoid: Set<string>,
    ): string {
        let name = `${prefix}${this.exceptionCounter}`;
        while (namesToAvoid.has(name)) {
            this.exceptionCounter++;
            name = `${prefix}${this.exceptionCounter}`;
        }
        this.exceptionCounter++;
        return name;
    }

    private generateUniqueExceptionName(nameToAvoid?: string): string {
        let exceptionName = `__caught_exception_${this.exceptionCounter}`;
        while (exceptionName === nameToAvoid) {
            this.exceptionCounter++;
            exceptionName = `__caught_exception_${this.exceptionCounter}`;
        }
        this.exceptionCounter++;
        return exceptionName;
    }

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

        let containerCode = `JsVariant ${CONTAINER_FUNCTION_NAME}() {\n`;
        this.indentationLevel++;
        containerCode += this.visit(ast, {
            isMainContext: true,
            isInsideFunction: true,
            isFunctionBody: true,
        });
        this.indentationLevel--;
        containerCode += "  return undefined;\n";
        containerCode += "}\n";

        let mainCode = "int main() {\n";
        mainCode += `  ${CONTAINER_FUNCTION_NAME}();\n`;
        mainCode += "  return 0;\n}\n";

        return prelude + declarations + containerCode + mainCode;
    }

    private generateLambda(
        node: ts.ArrowFunction | ts.FunctionDeclaration,
        isAssignment: boolean = false,
    ): string {
        let lambda = "[=]";
        lambda += `(const std::vector<JsVariant>& args) mutable -> JsVariant `;

        const visitContext = {
            isMainContext: false,
            isInsideFunction: true,
            isFunctionBody: false,
        };

        if (node.body) {
            if (ts.isBlock(node.body)) {
                let paramExtraction = "";
                this.indentationLevel++;
                node.parameters.forEach((p, i) => {
                    const name = p.name.getText();
                    const defaultValue = p.initializer
                        ? this.visit(p.initializer, visitContext)
                        : "undefined";
                    paramExtraction +=
                        `${this.indent()}auto ${name} = args.size() > ${i} ? args[${i}] : ${defaultValue};\n`;
                });
                this.indentationLevel--;

                const blockContent = this.visit(node.body, {
                    isMainContext: false,
                    isInsideFunction: true,
                    isFunctionBody: true,
                });
                // The block visitor already adds braces, so we need to inject the param extraction.
                lambda += "{\n" + paramExtraction + blockContent.substring(2);
            } else {
                lambda += "{\n";
                this.indentationLevel++;
                node.parameters.forEach((p, i) => {
                    const name = p.name.getText();
                    const defaultValue = p.initializer
                        ? this.visit(p.initializer, visitContext)
                        : "undefined";
                    lambda +=
                        `${this.indent()}auto ${name} = args.size() > ${i} ? args[${i}] : ${defaultValue};\n`;
                });
                lambda += `${this.indent()}return ${
                    this.visit(node.body, {
                        isMainContext: false,
                        isInsideFunction: true,
                        isFunctionBody: false,
                    })
                };\n`;
                this.indentationLevel--;
                lambda += `${this.indent()}}`;
            }
        } else {
            lambda += "{ return undefined; }\n";
        }

        const signature = `JsVariant(const std::vector<JsVariant>&)`;
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
            exceptionName?: string;
            isInsideTryCatchLambda?: boolean;
            hasReturnedFlag?: string;
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
                    const isLetOrConst = (decl.parent.flags & (ts.NodeFlags.Let | ts.NodeFlags.Const)) !== 0;
                    const initializer = isLetOrConst ? "tdz_uninitialized" : "undefined";
                    code +=
                        `${this.indent()}auto ${name} = std::make_shared<JsVariant>(${initializer});\n`;
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
                        const isLetOrConst = (stmt.declarationList.flags & (ts.NodeFlags.Let | ts.NodeFlags.Const)) !== 0;
                        const contextForVisit = {
                            ...context,
                            isAssignmentOnly: !isLetOrConst,
                        };
                        const assignments = this.visit(
                            stmt.declarationList,
                            contextForVisit,
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
                    const isLetOrConst = (decl.parent.flags & (ts.NodeFlags.Let | ts.NodeFlags.Const)) !== 0;
                    const initializer = isLetOrConst ? "tdz_uninitialized" : "undefined";
                    code +=
                        `${this.indent()}auto ${name} = std::make_shared<JsVariant>(${initializer});\n`;
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
                        const isLetOrConst = (stmt.declarationList.flags & (ts.NodeFlags.Let | ts.NodeFlags.Const)) !== 0;
                        const contextForVisit = {
                            ...context,
                            isAssignmentOnly: !isLetOrConst,
                        };
                        const assignments = this.visit(
                            stmt.declarationList,
                            contextForVisit,
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
                    initializer = " = " +
                        this.visit(varDecl.initializer, context);
                }

                const isLetOrConst = (varDecl.parent.flags & (ts.NodeFlags.Let | ts.NodeFlags.Const)) !== 0;

                if (isLetOrConst) {
                    // If there's no initializer, assign undefined. Otherwise, use the initializer.
                    return `*${name}${initializer || " = undefined"}`;
                }

                // For 'var', it's a bit more complex.
                // If we are in a non-function-body block, 'var' is hoisted, so it's an assignment.
                // If we are at the top level or in a function body, it's a declaration if not already hoisted.
                // The current logic hoists at the function level, so we need to decide if this is the *hoisting* declaration or a later assignment.
                // The `isAssignmentOnly` flag helps here.
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

            case ts.SyntaxKind.ParenthesizedExpression: {
                const parenExpr = node as ts.ParenthesizedExpression;
                return `(${this.visit(parenExpr.expression, context)})`;
            }

            case ts.SyntaxKind.PropertyAccessExpression: {
                const propAccess = node as ts.PropertyAccessExpression;
                const exprText = this.visit(propAccess.expression, context);
                return `checkAndDeref(${exprText}, "${exprText}").${propAccess.name.getText()}`;
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
                    const scope = this.getScopeForNode(binExpr.left);
                    const typeInfo = this.typeAnalyzer.scopeManager
                        .lookupFromScope(
                            leftText,
                            scope,
                        );
                    if (typeInfo?.isConst) {
                        return `throw std::runtime_error("TypeError: Assignment to constant variable.")`;
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

                const finalLeft = leftIsIdentifier && leftTypeInfo &&
                        !leftTypeInfo.isParameter
                    ? `checkAndDeref(${leftText}, "${leftText}")`
                    : leftText;
                const finalRight = rightIsIdentifier && rightTypeInfo &&
                        !rightTypeInfo.isParameter
                    ? `checkAndDeref(${rightText}, "${rightText}")`
                    : rightText;

                if (op === "+" || op === "-" || op === "*") {
                    return `(${finalLeft} ${op} ${finalRight})`;
                }
                return `${finalLeft} ${op} ${finalRight}`;
            }

            case ts.SyntaxKind.ThrowStatement: {
                const throwStmt = node as ts.ThrowStatement;
                const expr = this.visit(throwStmt.expression, context);
                return `${this.indent()}throw std::runtime_error(${expr});\n`;
            }

            case ts.SyntaxKind.TryStatement: {
                const tryStmt = node as ts.TryStatement;

                if (tryStmt.finallyBlock) {
                    const declaredSymbols = new Set<string>();
                    this.getDeclaredSymbols(tryStmt.tryBlock).forEach(s => declaredSymbols.add(s));
                    if (tryStmt.catchClause) {
                        this.getDeclaredSymbols(tryStmt.catchClause).forEach(s => declaredSymbols.add(s));
                    }
                    this.getDeclaredSymbols(tryStmt.finallyBlock).forEach(s => declaredSymbols.add(s));

                    const finallyLambdaName = this.generateUniqueName('__finally_', declaredSymbols);
                    const resultVarName = this.generateUniqueName('__try_result_', declaredSymbols);
                    const hasReturnedFlagName = this.generateUniqueName('__try_has_returned_', declaredSymbols);

                    let code = `${this.indent()}{\n`;
                    this.indentationLevel++;

                    code += `${this.indent()}JsVariant ${resultVarName};\n`;
                    code += `${this.indent()}bool ${hasReturnedFlagName} = false;\n`;

                    const finallyBlockCode = this.visit(tryStmt.finallyBlock, { ...context, isFunctionBody: false });
                    code += `${this.indent()}auto ${finallyLambdaName} = [&]() ${finallyBlockCode.trim()};\n`;

                    code += `${this.indent()}try {\n`;
                    this.indentationLevel++;

                    code += `${this.indent()}${resultVarName} = ([&]() -> JsVariant {\n`;
                    this.indentationLevel++;

                    const innerContext = {
                        ...context,
                        isFunctionBody: false,
                        isInsideTryCatchLambda: true,
                        hasReturnedFlag: hasReturnedFlagName
                    };

                    code += `${this.indent()}try {\n`;
                    this.indentationLevel++;
                    code += this.visit(tryStmt.tryBlock, innerContext);
                    this.indentationLevel--;
                    code += `${this.indent()}}\n`;

                    if (tryStmt.catchClause) {
                        const exceptionName = this.generateUniqueExceptionName(tryStmt.catchClause.variableDeclaration?.name.getText());
                        const catchContext = { ...innerContext, exceptionName };
                        code += `${this.indent()}catch (const std::exception& ${exceptionName}) {\n`;
                        this.indentationLevel++;
                        code += this.visit(tryStmt.catchClause.block, catchContext);
                        this.indentationLevel--;
                        code += `${this.indent()}}\n`;
                    } else {
                        code += `${this.indent()}catch (...) { throw; }\n`;
                    }

                    code += `${this.indent()}return undefined;\n`;

                    this.indentationLevel--;
                    code += `${this.indent()}})();\n`;

                    this.indentationLevel--;
                    code += `${this.indent()}} catch (...) {\n`;
                    this.indentationLevel++;
                    code += `${this.indent()}${finallyLambdaName}();\n`;
                    code += `${this.indent()}throw;\n`;
                    this.indentationLevel--;
                    code += `${this.indent()}}\n`;

                    code += `${this.indent()}${finallyLambdaName}();\n`;

                    code += `${this.indent()}if (${hasReturnedFlagName}) {\n`;
                    this.indentationLevel++;
                    code += `${this.indent()}return ${resultVarName};\n`;
                    this.indentationLevel--;
                    code += `${this.indent()}}\n`;

                    this.indentationLevel--;
                    code += `${this.indent()}}\n`;
                    return code;
                } else {
                    const exceptionName = this.generateUniqueExceptionName(tryStmt.catchClause?.variableDeclaration?.name.getText());
                    const newContext = { ...context, isFunctionBody: false, exceptionName };
                    let code = `${this.indent()}try `;
                    code += this.visit(tryStmt.tryBlock, newContext);
                    if (tryStmt.catchClause) {
                        code += ` catch (const std::exception& ${exceptionName}) `;
                        code += this.visit(tryStmt.catchClause, newContext);
                    }
                    return code;
                }
            }

            case ts.SyntaxKind.CatchClause: {
                const catchClause = node as ts.CatchClause;
                const exceptionName = context.exceptionName;
                if (!exceptionName) {
                    // This should not happen if it's coming from a TryStatement
                    throw new Error(
                        "Compiler bug: exceptionName not found in context for CatchClause",
                    );
                }

                if (catchClause.variableDeclaration) {
                    const varName = catchClause.variableDeclaration.name
                        .getText();
                    let code = `{\n`;
                    this.indentationLevel++;
                    code += `${this.indent()}{\n`;
                    this.indentationLevel++;

                    // Always create the JS exception variable.
                    code +=
                        `${this.indent()}auto ${varName} = std::make_shared<JsVariant>(std::string(${exceptionName}.what()));\n`;

                    // Shadow the C++ exception variable *only if* the names don't clash.
                    if (varName !== exceptionName) {
                        code +=
                            `${this.indent()}auto ${exceptionName} = std::make_shared<JsVariant>(undefined);\n`;
                    }

                    code += this.visit(catchClause.block, context);
                    this.indentationLevel--;
                    code += `${this.indent()}}\n`;
                    this.indentationLevel--;
                    code += `${this.indent()}}\n`;
                    return code;
                } else {
                    // No variable in the catch clause, e.g., `catch { ... }`
                    let code = `{\n`; // Alway create block scope
                    code += this.visit(catchClause.block, context);
                    code += `${this.indent()}}\n`;
                    return code;
                }
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
                            return `checkAndDeref(${argText}, "${argText}")`;
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

                const calleeCode = this.visit(callee, context);
                const derefCallee = ts.isIdentifier(callee) 
                    ? `checkAndDeref(${calleeCode}, "${calleeCode}")` 
                    : calleeCode;
                return `std::any_cast<std::function<JsVariant(const std::vector<JsVariant>&)>>(${derefCallee})({${args}})`;
            }

            case ts.SyntaxKind.ReturnStatement: {
                if (context.isMainContext) {
                    return `${this.indent()}throw std::runtime_error("SyntaxError: Return statements are only valid inside functions.");\n`;
                }

                const returnStmt = node as ts.ReturnStatement;

                if (context.isInsideTryCatchLambda && context.hasReturnedFlag) {
                    let returnCode = `${this.indent()}${context.hasReturnedFlag} = true;\n`;
                    if (returnStmt.expression) {
                        const expr = returnStmt.expression;
                        const exprText = this.visit(expr, context);
                        if (ts.isIdentifier(expr)) {
                            const scope = this.getScopeForNode(expr);
                            const typeInfo = this.typeAnalyzer.scopeManager
                                .lookupFromScope(exprText, scope);
                            if (typeInfo && !typeInfo.isParameter) {
                                returnCode += `${this.indent()}return checkAndDeref(${exprText}, "${exprText}");\n`;
                            } else {
                                returnCode += `${this.indent()}return ${exprText};\n`;
                            }                            
                        } else {
                            returnCode += `${this.indent()}return ${exprText};\n`;
                        }
                    } else {
                        returnCode += `${this.indent()}return undefined;\n`;
                    }
                    return returnCode;
                }

                if (returnStmt.expression) {
                    const expr = returnStmt.expression;
                    const exprText = this.visit(expr, context);
                    if (ts.isIdentifier(expr)) {
                        const scope = this.getScopeForNode(expr);
                        const typeInfo = this.typeAnalyzer.scopeManager
                            .lookupFromScope(exprText, scope);
                        if (typeInfo && !typeInfo.isParameter) {
                            return `${this.indent()}return checkAndDeref(${exprText}, "${exprText}");\n`;
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
