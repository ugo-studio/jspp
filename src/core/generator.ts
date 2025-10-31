import ts from "typescript";

import { Scope } from "../analysis/scope";
import { TypeAnalyzer } from "../analysis/typeAnalyzer";
import type { Node } from "../ast/types";

const CONTAINER_FUNCTION_NAME = "__container__";

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

        const declarations = `#include "index.hpp"\n\n`;

        let containerCode = `jspp::JsValue ${CONTAINER_FUNCTION_NAME}() {\n`;
        this.indentationLevel++;
        containerCode += this.visit(ast, {
            isMainContext: true,
            isInsideFunction: true,
            isFunctionBody: true,
        });
        this.indentationLevel--;
        containerCode += "  return undefined;\n";
        containerCode += "}\n\n";

        let mainCode = "int main() {\n";
        mainCode += `  try {\n`;
        mainCode += `    ${CONTAINER_FUNCTION_NAME}();\n`;
        mainCode += `  } catch (const jspp::JsValue& e) {\n`;
        mainCode +=
            `    auto error = std::make_shared<jspp::JsValue>(jspp::Exception::parse_error_from_value(e));\n`;
        mainCode += `    console.error(error);\n`;
        mainCode += `    return 1;\n`;
        mainCode += `  }\n`;
        mainCode += "  return 0;\n}\n";

        return declarations + containerCode + mainCode;
    }

    private generateLambda(
        node: ts.ArrowFunction | ts.FunctionDeclaration | ts.FunctionExpression,
        isAssignment: boolean = false,
        capture: string = "[=]",
    ): string {
        const declaredSymbols = this.getDeclaredSymbols(node);
        const argsName = this.generateUniqueName("__args_", declaredSymbols);

        let lambda =
            `${capture}(const std::vector<jspp::JsValue>& ${argsName}) mutable -> jspp::JsValue `;

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
                        `${this.indent()}auto ${name} = ${argsName}.size() > ${i} ? ${argsName}[${i}] : ${defaultValue};\n`;
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
                        `${this.indent()}auto ${name} = ${argsName}.size() > ${i} ? ${argsName}[${i}] : ${defaultValue};\n`;
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

        const signature = `jspp::JsValue(const std::vector<jspp::JsValue>&)`;
        const callable = `std::function<${signature}>(${lambda})`;
        const fullExpression = `jspp::Object::make_function(${callable})`;

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

            case ts.SyntaxKind.FunctionExpression: {
                const funcExpr = node as ts.FunctionExpression;
                if (funcExpr.name) {
                    const funcName = funcExpr.name.getText();
                    let code = "([&]() -> jspp::JsValue {\n";
                    this.indentationLevel++;
                    code +=
                        `${this.indent()}auto ${funcName} = std::make_shared<jspp::JsValue>();\n`;
                    const lambda = this.generateLambda(
                        funcExpr,
                        false,
                        "[=]",
                    );
                    code += `${this.indent()}*${funcName} = ${lambda};\n`;
                    code += `${this.indent()}return *${funcName};\n`;
                    this.indentationLevel--;
                    code += `${this.indent()}})()`;
                    return code;
                }
                return this.generateLambda(node as ts.FunctionExpression);
            }

            case ts.SyntaxKind.SourceFile: {
                const sourceFile = node as ts.SourceFile;
                let code = "";
                const varDecls = sourceFile.statements
                    .filter(ts.isVariableStatement)
                    .flatMap((stmt) => stmt.declarationList.declarations);

                const funcDecls = sourceFile.statements.filter(
                    ts.isFunctionDeclaration,
                );

                const hoistedSymbols = new Set<string>();

                // 1. Hoist function declarations
                funcDecls.forEach((func) => {
                    const funcName = func.name?.getText();
                    if (funcName && !hoistedSymbols.has(funcName)) {
                        hoistedSymbols.add(funcName);
                        code +=
                            `${this.indent()}auto ${funcName} = std::make_shared<jspp::JsValue>(undefined);\n`;
                    }
                });

                // Hoist variable declarations
                varDecls.forEach((decl) => {
                    const name = decl.name.getText();
                    if (hoistedSymbols.has(name)) {
                        return;
                    }
                    hoistedSymbols.add(name);
                    const isLetOrConst = (decl.parent.flags &
                        (ts.NodeFlags.Let | ts.NodeFlags.Const)) !== 0;
                    const initializer = isLetOrConst
                        ? "jspp::uninitialized"
                        : "undefined";
                    code +=
                        `${this.indent()}auto ${name} = std::make_shared<jspp::JsValue>(${initializer});\n`;
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
                        const isLetOrConst = (stmt.declarationList.flags &
                            (ts.NodeFlags.Let | ts.NodeFlags.Const)) !== 0;
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

                const hoistedSymbols = new Set<string>();

                // 1. Hoist all function declarations
                funcDecls.forEach((func) => {
                    const funcName = func.name?.getText();
                    if (funcName && !hoistedSymbols.has(funcName)) {
                        hoistedSymbols.add(funcName);
                        code +=
                            `${this.indent()}auto ${funcName} = std::make_shared<jspp::JsValue>(undefined);\n`;
                    }
                });

                // Hoist variable declarations
                varDecls.forEach((decl) => {
                    const name = decl.name.getText();
                    if (hoistedSymbols.has(name)) {
                        return;
                    }
                    hoistedSymbols.add(name);
                    const isLetOrConst = (decl.parent.flags &
                        (ts.NodeFlags.Let | ts.NodeFlags.Const)) !== 0;
                    const initializer = isLetOrConst
                        ? "jspp::uninitialized"
                        : "undefined";
                    code +=
                        `${this.indent()}auto ${name} = std::make_shared<jspp::JsValue>(${initializer});\n`;
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
                        const isLetOrConst = (stmt.declarationList.flags &
                            (ts.NodeFlags.Let | ts.NodeFlags.Const)) !== 0;
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
                    const initExpr = varDecl.initializer;
                    let initText = this.visit(initExpr, context);
                    if (ts.isIdentifier(initExpr)) {
                        const scope = this.getScopeForNode(initExpr);
                        const typeInfo = this.typeAnalyzer.scopeManager
                            .lookupFromScope(
                                initExpr.text,
                                scope,
                            );
                        if (
                            typeInfo && !typeInfo.isParameter &&
                            !typeInfo.isBuiltin
                        ) {
                            initText = `jspp::Access::deref(${initText}, ${
                                this.getJsVarName(initExpr)
                            })`;
                        }
                    }
                    initializer = " = " + initText;
                }

                const isLetOrConst = (varDecl.parent.flags &
                    (ts.NodeFlags.Let | ts.NodeFlags.Const)) !== 0;

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
                    return `auto ${name} = std::make_shared<jspp::JsValue>(${initValue})`;
                }
            }

            case ts.SyntaxKind.ObjectLiteralExpression: {
                const obj = node as ts.ObjectLiteralExpression;
                let props = "";
                for (const prop of obj.properties) {
                    if (ts.isPropertyAssignment(prop)) {
                        const key = prop.name.getText();
                        const value = this.visit(prop.initializer, context);
                        props += `{"${key}", ${value}},`;
                    }
                }
                return `jspp::Object::make_object({${props}})`;
            }

            case ts.SyntaxKind.ArrayLiteralExpression: {
                const elements = (node as ts.ArrayLiteralExpression).elements
                    .map((elem) => this.visit(elem, context))
                    .join(", ");
                return `jspp::Object::make_array({${elements}})`;
            }

            case ts.SyntaxKind.ForStatement: {
                const forStmt = node as ts.ForStatement;
                let code = "";
                this.indentationLevel++; // Enter a new scope for the for loop

                // Handle initializer
                let initializerCode = "";
                if (forStmt.initializer) {
                    if (ts.isVariableDeclarationList(forStmt.initializer)) {
                        const varDeclList = forStmt.initializer;
                        const isLetOrConst = (varDeclList.flags &
                            (ts.NodeFlags.Let | ts.NodeFlags.Const)) !== 0;
                        if (isLetOrConst) {
                            // For `let` or `const` in for loop, they are block-scoped to the loop.
                            // Declare the variable within the loop's scope.
                            // The C++ for loop initializer can contain a declaration.
                            const decl = varDeclList.declarations[0]; // Assuming single declaration for simplicity
                            if (decl) {
                                const name = decl.name.getText();
                                const initValue = decl.initializer
                                    ? this.visit(decl.initializer, context)
                                    : "undefined";
                                initializerCode =
                                    `auto ${name} = std::make_shared<jspp::JsValue>(${initValue})`;
                            }
                        } else {
                            // For 'var', it's already hoisted, so this is an assignment.
                            initializerCode = this.visit(forStmt.initializer, {
                                ...context,
                                isAssignmentOnly: true,
                            });
                        }
                    } else {
                        // If it's an expression (e.g., `i = 0`)
                        initializerCode = this.visit(
                            forStmt.initializer,
                            context,
                        );
                    }
                }

                code += `${this.indent()}for (${initializerCode}; `;
                if (forStmt.condition) {
                    code += `jspp::Access::is_truthy(${
                        this.visit(forStmt.condition, context)
                    })`;
                }
                code += "; ";
                if (forStmt.incrementor) {
                    code += this.visit(forStmt.incrementor, context);
                }
                code += ") ";
                code += this.visit(forStmt.statement, {
                    ...context,
                    isFunctionBody: false,
                });
                this.indentationLevel--; // Exit the scope for the for loop
                return code;
            }

            case ts.SyntaxKind.ForInStatement: {
                const forIn = node as ts.ForInStatement;

                let code = "";
                this.indentationLevel++; // Enter a new scope for the for-in loop
                let varName = "";

                if (ts.isVariableDeclarationList(forIn.initializer)) {
                    const decl = forIn.initializer.declarations[0];
                    if (decl) {
                        varName = decl.name.getText();
                        // Declare the shared_ptr before the loop
                        code +=
                            `${this.indent()}auto ${varName} = std::make_shared<jspp::JsValue>(undefined);\n`;
                    }
                } else if (ts.isIdentifier(forIn.initializer)) {
                    varName = forIn.initializer.getText();
                    // Assume it's already declared in an outer scope, just assign to it.
                    // No explicit declaration here.
                }

                const expr = forIn.expression;
                const exprText = this.visit(expr, context);

                let derefExpr = exprText;
                if (ts.isIdentifier(expr)) {
                    derefExpr = `jspp::Access::deref(${exprText}, ${
                        this.getJsVarName(expr)
                    })`;
                }

                const keysVar = this.generateUniqueName("__keys_", new Set());
                code +=
                    `${this.indent()}{ std::vector<std::string> ${keysVar} = jspp::Access::get_object_keys(${derefExpr});\n`;
                code +=
                    `${this.indent()}for (const auto& ${varName}_str : ${keysVar}) {\n`;
                this.indentationLevel++;
                code +=
                    `${this.indent()}*${varName} = jspp::Object::make_string(${varName}_str);\n`;
                code += this.visit(forIn.statement, {
                    ...context,
                    isFunctionBody: false,
                });
                this.indentationLevel--;
                code += `${this.indent()}}}\n`;
                this.indentationLevel--; // Exit the scope for the for-in loop
                return code;
            }

            case ts.SyntaxKind.ForOfStatement: {
                const forOf = node as ts.ForOfStatement;

                let code = "";
                this.indentationLevel++; // Enter a new scope for the for-of loop
                let varName = "";

                if (ts.isVariableDeclarationList(forOf.initializer)) {
                    const decl = forOf.initializer.declarations[0];
                    if (decl) {
                        varName = decl.name.getText();
                        // Declare the shared_ptr before the loop
                        code +=
                            `${this.indent()}auto ${varName} = std::make_shared<jspp::JsValue>(undefined);\n`;
                    }
                } else if (ts.isIdentifier(forOf.initializer)) {
                    varName = forOf.initializer.getText();
                    // Assume it's already declared in an outer scope, just assign to it.
                    // No explicit declaration here.
                }

                const iterableExpr = this.visit(forOf.expression, context);
                const derefIterable = `jspp::Access::deref(${iterableExpr}, ${
                    this.getJsVarName(forOf.expression as ts.Identifier)
                })`;
                const arrayPtr = this.generateUniqueName(
                    "__array_ptr_",
                    new Set(),
                );

                code +=
                    `${this.indent()}{ auto ${arrayPtr} = std::any_cast<std::shared_ptr<jspp::JsArray>>(${derefIterable});\n`;
                code +=
                    `${this.indent()}for (const auto& ${varName}_val : ${arrayPtr}->properties) {\n`;
                this.indentationLevel++;
                code += `${this.indent()}*${varName} = ${varName}_val;\n`;
                code += this.visit(forOf.statement, {
                    ...context,
                    isFunctionBody: false,
                });
                this.indentationLevel--;
                code += `${this.indent()}}}\n`;
                this.indentationLevel--; // Exit the scope for the for-of loop

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
                return `${this.indent()}if (jspp::Access::is_truthy(${condition})) ${thenStmt}${elseStmt}`;
            }

            case ts.SyntaxKind.PrefixUnaryExpression: {
                const prefixUnaryExpr = node as ts.PrefixUnaryExpression;
                const operand = this.visit(prefixUnaryExpr.operand, context);
                const operator = ts.tokenToString(prefixUnaryExpr.operator);
                if (operator === "++" || operator === "--") {
                    return `${operator}(*${operand})`;
                }
                if (operator === "~") {
                    return `${operator}(*${operand})`;
                }
                return `${operator}${operand}`;
            }

            case ts.SyntaxKind.PostfixUnaryExpression: {
                const postfixUnaryExpr = node as ts.PostfixUnaryExpression;
                const operand = this.visit(postfixUnaryExpr.operand, context);
                const operator = ts.tokenToString(postfixUnaryExpr.operator);
                return `(*${operand})${operator}`;
            }

            case ts.SyntaxKind.ParenthesizedExpression: {
                const parenExpr = node as ts.ParenthesizedExpression;
                return `(${this.visit(parenExpr.expression, context)})`;
            }

            case ts.SyntaxKind.PropertyAccessExpression: {
                const propAccess = node as ts.PropertyAccessExpression;
                const exprText = this.visit(propAccess.expression, context);
                const propName = propAccess.name.getText();

                if (exprText === "console") {
                    return `console.${propName}`;
                }

                const scope = this.getScopeForNode(propAccess.expression);
                const typeInfo = ts.isIdentifier(propAccess.expression)
                    ? this.typeAnalyzer.scopeManager.lookupFromScope(
                        propAccess.expression.getText(),
                        scope,
                    )
                    : null;
                let finalExpr = "";

                if (ts.isIdentifier(propAccess.expression) && !typeInfo) {
                    finalExpr = `jspp::Exception::throw_unresolved_reference(${
                        this.getJsVarName(
                            propAccess.expression,
                        )
                    })`;
                } else if (
                    typeInfo &&
                    !typeInfo.isParameter && !typeInfo.isBuiltin
                ) {
                    finalExpr = `jspp::Access::deref(${exprText}, ${
                        this.getJsVarName(
                            propAccess.expression as ts.Identifier,
                        )
                    })`;
                } else {
                    finalExpr = exprText;
                }

                return `jspp::Access::get_property(${finalExpr}, "${propName}")`;
            }

            case ts.SyntaxKind.ElementAccessExpression: {
                const elemAccess = node as ts.ElementAccessExpression;
                const exprText = this.visit(elemAccess.expression, context);
                let argText = this.visit(
                    elemAccess.argumentExpression,
                    context,
                );

                // Dereference the expression being accessed
                const exprScope = this.getScopeForNode(elemAccess.expression);
                const exprTypeInfo = ts.isIdentifier(elemAccess.expression)
                    ? this.typeAnalyzer.scopeManager.lookupFromScope(
                        elemAccess.expression.getText(),
                        exprScope,
                    )
                    : null;
                const finalExpr = exprTypeInfo &&
                        !exprTypeInfo.isParameter && !exprTypeInfo.isBuiltin
                    ? `jspp::Access::deref(${exprText}, ${
                        this.getJsVarName(
                            elemAccess.expression as ts.Identifier,
                        )
                    })`
                    : exprText;

                // Dereference the argument expression if it's an identifier
                if (ts.isIdentifier(elemAccess.argumentExpression)) {
                    const argScope = this.getScopeForNode(
                        elemAccess.argumentExpression,
                    );
                    const argTypeInfo = this.typeAnalyzer.scopeManager
                        .lookupFromScope(
                            elemAccess.argumentExpression.getText(),
                            argScope,
                        );
                    if (
                        argTypeInfo && !argTypeInfo.isParameter &&
                        !argTypeInfo.isBuiltin
                    ) {
                        argText = `jspp::Access::deref(${argText}, ${
                            this.getJsVarName(
                                elemAccess.argumentExpression as ts.Identifier,
                            )
                        })`;
                    }
                }

                return `jspp::Access::get_property(${finalExpr}, ${argText})`;
            }

            case ts.SyntaxKind.ExpressionStatement:
                return this.indent() +
                    this.visit(
                        (node as ts.ExpressionStatement).expression,
                        context,
                    ) + ";\n";

            case ts.SyntaxKind.BinaryExpression: {
                const binExpr = node as ts.BinaryExpression;
                const opToken = binExpr.operatorToken;
                let op = opToken.getText();

                if (
                    opToken.kind === ts.SyntaxKind.PlusEqualsToken ||
                    opToken.kind === ts.SyntaxKind.MinusEqualsToken ||
                    opToken.kind === ts.SyntaxKind.AsteriskEqualsToken ||
                    opToken.kind === ts.SyntaxKind.SlashEqualsToken ||
                    opToken.kind === ts.SyntaxKind.PercentEqualsToken
                ) {
                    const leftText = this.visit(binExpr.left, context);
                    const rightText = this.visit(binExpr.right, context);
                    return `*${leftText} ${op} ${rightText}`;
                }

                if (opToken.kind === ts.SyntaxKind.EqualsToken) {
                    const rightText = this.visit(binExpr.right, context);

                    if (ts.isPropertyAccessExpression(binExpr.left)) {
                        const propAccess = binExpr.left;
                        const objExprText = this.visit(
                            propAccess.expression,
                            context,
                        );
                        const propName = propAccess.name.getText();

                        const scope = this.getScopeForNode(
                            propAccess.expression,
                        );
                        const typeInfo = ts.isIdentifier(propAccess.expression)
                            ? this.typeAnalyzer.scopeManager.lookupFromScope(
                                propAccess.expression.getText(),
                                scope,
                            )
                            : null;

                        const finalObjExpr = typeInfo &&
                                !typeInfo.isParameter && !typeInfo.isBuiltin
                            ? `jspp::Access::deref(${objExprText}, ${
                                this.getJsVarName(
                                    propAccess.expression as ts.Identifier,
                                )
                            })`
                            : objExprText;

                        return `jspp::Access::set_property(${finalObjExpr}, "${propName}", ${rightText})`;
                    } else if (ts.isElementAccessExpression(binExpr.left)) {
                        const elemAccess = binExpr.left;
                        const objExprText = this.visit(
                            elemAccess.expression,
                            context,
                        );
                        const argText = this.visit(
                            elemAccess.argumentExpression,
                            context,
                        );

                        const scope = this.getScopeForNode(
                            elemAccess.expression,
                        );
                        const typeInfo = ts.isIdentifier(elemAccess.expression)
                            ? this.typeAnalyzer.scopeManager.lookupFromScope(
                                elemAccess.expression.getText(),
                                scope,
                            )
                            : null;

                        const finalObjExpr = typeInfo &&
                                !typeInfo.isParameter && !typeInfo.isBuiltin
                            ? `jspp::Access::deref(${objExprText}, ${
                                this.getJsVarName(
                                    elemAccess.expression as ts.Identifier,
                                )
                            })`
                            : objExprText;

                        return `jspp::Access::set_property(${finalObjExpr}, ${argText}, ${rightText})`;
                    }

                    const leftText = this.visit(binExpr.left, context);
                    const scope = this.getScopeForNode(binExpr.left);
                    const typeInfo = this.typeAnalyzer.scopeManager
                        .lookupFromScope(
                            (binExpr.left as ts.Identifier).text,
                            scope,
                        );
                    if (!typeInfo) {
                        return `jspp::Exception::throw_unresolved_reference(${
                            this.getJsVarName(binExpr.left as ts.Identifier)
                        })`;
                    }
                    if (typeInfo?.isConst) {
                        return `jspp::Exception::throw_immutable_assignment()`;
                    }
                    return `*${leftText} ${op} ${rightText}`;
                }

                const leftText = this.visit(binExpr.left, context);
                const rightText = this.visit(binExpr.right, context);

                const leftIsIdentifier = ts.isIdentifier(binExpr.left);
                const rightIsIdentifier = ts.isIdentifier(binExpr.right);

                const scope = this.getScopeForNode(node);
                const leftTypeInfo = leftIsIdentifier
                    ? this.typeAnalyzer.scopeManager.lookupFromScope(
                        binExpr.left.getText(),
                        scope,
                    )
                    : null;
                const rightTypeInfo = rightIsIdentifier
                    ? this.typeAnalyzer.scopeManager.lookupFromScope(
                        binExpr.right.getText(),
                        scope,
                    )
                    : null;

                const finalLeft = leftIsIdentifier && leftTypeInfo &&
                        !leftTypeInfo.isParameter && !leftTypeInfo.isBuiltin
                    ? `jspp::Access::deref(${leftText}, ${
                        this.getJsVarName(binExpr.left as ts.Identifier)
                    })`
                    : leftText;
                const finalRight = rightIsIdentifier && rightTypeInfo &&
                        !rightTypeInfo.isParameter && !rightTypeInfo.isBuiltin
                    ? `jspp::Access::deref(${rightText}, ${
                        this.getJsVarName(binExpr.right as ts.Identifier)
                    })`
                    : rightText;

                if (leftIsIdentifier && !leftTypeInfo) {
                    return `jspp::Exception::throw_unresolved_reference(${
                        this.getJsVarName(binExpr.left as ts.Identifier)
                    })`;
                }
                if (rightIsIdentifier && !rightTypeInfo) {
                    return `jspp::Exception::throw_unresolved_reference(${
                        this.getJsVarName(binExpr.right as ts.Identifier)
                    })`;
                }

                if (opToken.kind === ts.SyntaxKind.EqualsEqualsEqualsToken) {
                    return `jspp::Access::strict_equals(${finalLeft}, ${finalRight})`;
                }
                if (opToken.kind === ts.SyntaxKind.EqualsEqualsToken) {
                    return `jspp::Access::equals(${finalLeft}, ${finalRight})`;
                }
                if (
                    opToken.kind === ts.SyntaxKind.ExclamationEqualsEqualsToken
                ) {
                    return `!jspp::Access::strict_equals(${finalLeft}, ${finalRight})`;
                }
                if (opToken.kind === ts.SyntaxKind.ExclamationEqualsToken) {
                    return `!jspp::Access::equals(${finalLeft}, ${finalRight})`;
                }
                if (opToken.kind === ts.SyntaxKind.AsteriskAsteriskToken) {
                    return `jspp::pow(${finalLeft}, ${finalRight})`;
                }

                if (
                    op === "+" || op === "-" || op === "*" || op === "/" ||
                    op === "%" || op === "^" || op === "&" || op === "|"
                ) {
                    return `(${finalLeft} ${op} ${finalRight})`;
                }
                return `${finalLeft} ${op} ${finalRight}`;
            }

            case ts.SyntaxKind.ThrowStatement: {
                const throwStmt = node as ts.ThrowStatement;
                const expr = this.visit(throwStmt.expression, context);
                return `${this.indent()}throw jspp::JsValue(${expr});\n`;
            }

            case ts.SyntaxKind.TryStatement: {
                const tryStmt = node as ts.TryStatement;

                if (tryStmt.finallyBlock) {
                    const declaredSymbols = new Set<string>();
                    this.getDeclaredSymbols(tryStmt.tryBlock).forEach((s) =>
                        declaredSymbols.add(s)
                    );
                    if (tryStmt.catchClause) {
                        this.getDeclaredSymbols(tryStmt.catchClause).forEach(
                            (s) => declaredSymbols.add(s),
                        );
                    }
                    this.getDeclaredSymbols(tryStmt.finallyBlock).forEach((s) =>
                        declaredSymbols.add(s)
                    );

                    const finallyLambdaName = this.generateUniqueName(
                        "__finally_",
                        declaredSymbols,
                    );
                    const resultVarName = this.generateUniqueName(
                        "__try_result_",
                        declaredSymbols,
                    );
                    const hasReturnedFlagName = this.generateUniqueName(
                        "__try_has_returned_",
                        declaredSymbols,
                    );

                    let code = `${this.indent()}{\n`;
                    this.indentationLevel++;

                    code += `${this.indent()}jspp::JsValue ${resultVarName};\n`;
                    code +=
                        `${this.indent()}bool ${hasReturnedFlagName} = false;\n`;

                    const finallyBlockCode = this.visit(tryStmt.finallyBlock, {
                        ...context,
                        isFunctionBody: false,
                    });
                    code +=
                        `${this.indent()}auto ${finallyLambdaName} = [&]() ${finallyBlockCode.trim()};\n`;

                    code += `${this.indent()}try {\n`;
                    this.indentationLevel++;

                    code +=
                        `${this.indent()}${resultVarName} = ([&]() -> jspp::JsValue {\n`;
                    this.indentationLevel++;

                    const innerContext = {
                        ...context,
                        isFunctionBody: false,
                        isInsideTryCatchLambda: true,
                        hasReturnedFlag: hasReturnedFlagName,
                    };

                    code += `${this.indent()}try {\n`;
                    this.indentationLevel++;
                    code += this.visit(tryStmt.tryBlock, innerContext);
                    this.indentationLevel--;
                    code += `${this.indent()}}\n`;

                    if (tryStmt.catchClause) {
                        const exceptionName = this.generateUniqueExceptionName(
                            tryStmt.catchClause.variableDeclaration?.name
                                .getText(),
                        );
                        const catchContext = { ...innerContext, exceptionName };
                        code +=
                            `${this.indent()}catch (const jspp::JsValue& ${exceptionName}) {\n`;
                        this.indentationLevel++;
                        code += this.visit(
                            tryStmt.catchClause.block,
                            catchContext,
                        );
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
                    const exceptionName = this.generateUniqueExceptionName(
                        tryStmt.catchClause?.variableDeclaration?.name
                            .getText(),
                    );
                    const newContext = {
                        ...context,
                        isFunctionBody: false,
                        exceptionName,
                    };
                    let code = `${this.indent()}try `;
                    code += this.visit(tryStmt.tryBlock, newContext);
                    if (tryStmt.catchClause) {
                        code +=
                            ` catch (const jspp::JsValue& ${exceptionName}) `;
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
                        `${this.indent()}auto ${varName} = std::make_shared<jspp::JsValue>(jspp::Exception::parse_error_from_value(${exceptionName}));\n`;

                    // Shadow the C++ exception variable *only if* the names don't clash.
                    if (varName !== exceptionName) {
                        code +=
                            `${this.indent()}auto ${exceptionName} = std::make_shared<jspp::JsValue>(undefined);\n`;
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
                            .lookupFromScope(
                                arg.text,
                                scope,
                            );
                        if (!typeInfo) {
                            return `jspp::Exception::throw_unresolved_reference(${
                                this.getJsVarName(arg)
                            })`;
                        }
                        if (
                            typeInfo && !typeInfo.isParameter &&
                            !typeInfo.isBuiltin
                        ) {
                            return `jspp::Access::deref(${argText}, ${
                                this.getJsVarName(arg)
                            })`;
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
                let derefCallee;
                if (ts.isIdentifier(callee)) {
                    const scope = this.getScopeForNode(callee);
                    const typeInfo = this.typeAnalyzer.scopeManager
                        .lookupFromScope(
                            callee.text,
                            scope,
                        );
                    if (!typeInfo) {
                        return `jspp::Exception::throw_unresolved_reference(${
                            this.getJsVarName(callee)
                        })`;
                    }
                    if (typeInfo.isBuiltin) {
                        derefCallee = calleeCode;
                    } else {
                        derefCallee = `jspp::Access::deref(${calleeCode}, ${
                            this.getJsVarName(callee)
                        })`;
                    }
                } else {
                    derefCallee = calleeCode;
                }
                return `std::any_cast<std::shared_ptr<jspp::JsFunction>>(${derefCallee})->call({${args}})`;
            }

            case ts.SyntaxKind.ReturnStatement: {
                if (context.isMainContext) {
                    return `${this.indent()}jspp::Exception::throw_invalid_return_statement();\n`;
                }

                const returnStmt = node as ts.ReturnStatement;

                if (context.isInsideTryCatchLambda && context.hasReturnedFlag) {
                    let returnCode =
                        `${this.indent()}${context.hasReturnedFlag} = true;\n`;
                    if (returnStmt.expression) {
                        const expr = returnStmt.expression;
                        const exprText = this.visit(expr, context);
                        if (ts.isIdentifier(expr)) {
                            const scope = this.getScopeForNode(expr);
                            const typeInfo = this.typeAnalyzer.scopeManager
                                .lookupFromScope(expr.text, scope);
                            if (!typeInfo) {
                                returnCode +=
                                    `${this.indent()}jspp::Exception::throw_unresolved_reference(${
                                        this.getJsVarName(expr)
                                    });\n`; // THROWS, not returns
                            }
                            if (
                                typeInfo && !typeInfo.isParameter &&
                                !typeInfo.isBuiltin
                            ) {
                                returnCode +=
                                    `${this.indent()}return jspp::Access::deref(${exprText}, ${
                                        this.getJsVarName(expr)
                                    });\n`;
                            } else {
                                returnCode +=
                                    `${this.indent()}return ${exprText};\n`;
                            }
                        } else {
                            returnCode +=
                                `${this.indent()}return ${exprText};\n`;
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
                            .lookupFromScope(expr.text, scope);
                        if (!typeInfo) {
                            return `${this.indent()}jspp::Exception::throw_unresolved_reference(${
                                this.getJsVarName(expr)
                            });\n`; // THROWS, not returns
                        }
                        if (
                            typeInfo && !typeInfo.isParameter &&
                            !typeInfo.isBuiltin
                        ) {
                            return `${this.indent()}return jspp::Access::deref(${exprText}, ${
                                this.getJsVarName(expr)
                            });\n`;
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
                return `jspp::Object::make_string("${
                    this.escapeString((node as ts.StringLiteral).text)
                }")`;
            case ts.SyntaxKind.NoSubstitutionTemplateLiteral: {
                const templateLiteral =
                    node as ts.NoSubstitutionTemplateLiteral;
                return `jspp::Object::make_string("${
                    this.escapeString(templateLiteral.text)
                }")`;
            }
            case ts.SyntaxKind.TemplateExpression: {
                const templateExpr = node as ts.TemplateExpression;

                let result = `jspp::Object::make_string("${
                    this.escapeString(templateExpr.head.text)
                }")`;

                for (const span of templateExpr.templateSpans) {
                    const expr = span.expression;
                    const exprText = this.visit(expr, context);
                    let finalExpr = exprText;

                    if (ts.isIdentifier(expr)) {
                        const scope = this.getScopeForNode(expr);
                        const typeInfo = this.typeAnalyzer.scopeManager
                            .lookupFromScope(
                                expr.text,
                                scope,
                            );
                        if (!typeInfo) {
                            finalExpr =
                                `jspp::Exception::throw_unresolved_reference(${
                                    this.getJsVarName(expr as ts.Identifier)
                                })`;
                        } else if (
                            typeInfo &&
                            !typeInfo.isParameter &&
                            !typeInfo.isBuiltin
                        ) {
                            finalExpr = `jspp::Access::deref(${exprText}, ${
                                this.getJsVarName(expr as ts.Identifier)
                            })`;
                        }
                    }

                    result += ` + (${finalExpr})`;

                    if (span.literal.text) {
                        result += ` + jspp::Object::make_string("${
                            this.escapeString(span.literal.text)
                        }")`;
                    }
                }
                return result;
            }
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

    private escapeString(str: string): string {
        return str.replace(/\\/g, "\\\\").replace(/"/g, '\\"').replace(
            /\n/g,
            "\\n",
        ).replace(/\r/g, "\\r").replace(/\t/g, "\\t");
    }

    private getJsVarName(node: ts.Identifier): string {
        return `"${node.text}"`;
    }
}
