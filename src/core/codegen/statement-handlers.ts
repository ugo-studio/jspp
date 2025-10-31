import ts from "typescript";

import { CodeGenerator } from "./";
import type { VisitContext } from "./visitor";

export function visitSourceFile(
    this: CodeGenerator,
    node: ts.SourceFile,
    context: VisitContext,
): string {
    const sourceFile = node as ts.SourceFile;
    let code = "";
    const varDecls = sourceFile.statements
        .filter(ts.isVariableStatement)
        .flatMap((stmt) => stmt.declarationList.declarations);

    const funcDecls = sourceFile.statements.filter(ts.isFunctionDeclaration);

    const hoistedSymbols = new Set<string>();

    // 1. Hoist function declarations
    funcDecls.forEach((func) => {
        const funcName = func.name?.getText();
        if (funcName && !hoistedSymbols.has(funcName)) {
            hoistedSymbols.add(funcName);
            code +=
                `${this.indent()}auto ${funcName} = std::make_shared<jspp::JsValue>(undefined);
`;
        }
    });

    // Hoist variable declarations
    varDecls.forEach((decl) => {
        const name = decl.name.getText();
        if (hoistedSymbols.has(name)) {
            return;
        }
        hoistedSymbols.add(name);
        const isLetOrConst =
            (decl.parent.flags & (ts.NodeFlags.Let | ts.NodeFlags.Const)) !==
                0;
        const initializer = isLetOrConst ? "jspp::uninitialized" : "undefined";
        code +=
            `${this.indent()}auto ${name} = std::make_shared<jspp::JsValue>(${initializer});
`;
    });

    // 2. Assign all hoisted functions first
    funcDecls.forEach((stmt) => {
        const funcName = stmt.name?.getText();
        if (funcName) {
            const lambda = this.generateLambda(stmt, true);
            code += `${this.indent()}*${funcName} = ${lambda};
`;
        }
    });

    // 3. Process other statements
    sourceFile.statements.forEach((stmt) => {
        if (ts.isFunctionDeclaration(stmt)) {
            // Already handled
        } else if (ts.isVariableStatement(stmt)) {
            const isLetOrConst = (stmt.declarationList.flags &
                (ts.NodeFlags.Let | ts.NodeFlags.Const)) !==
                0;
            const contextForVisit = {
                ...context,
                isAssignmentOnly: !isLetOrConst,
            };
            const assignments = this.visit(
                stmt.declarationList,
                contextForVisit,
            );
            if (assignments) {
                code += `${this.indent()}${assignments};
`;
            }
        } else {
            code += this.visit(stmt, context);
        }
    });
    return code;
}

export function visitBlock(
    this: CodeGenerator,
    node: ts.Block,
    context: VisitContext,
): string {
    let code = "{\n";
    this.indentationLevel++;
    const block = node as ts.Block;

    const varDecls = block.statements
        .filter(ts.isVariableStatement)
        .flatMap((stmt) => stmt.declarationList.declarations);

    const funcDecls = block.statements.filter(ts.isFunctionDeclaration);

    const hoistedSymbols = new Set<string>();

    // 1. Hoist all function declarations
    funcDecls.forEach((func) => {
        const funcName = func.name?.getText();
        if (funcName && !hoistedSymbols.has(funcName)) {
            hoistedSymbols.add(funcName);
            code +=
                `${this.indent()}auto ${funcName} = std::make_shared<jspp::JsValue>(undefined);
`;
        }
    });

    // Hoist variable declarations
    varDecls.forEach((decl) => {
        const name = decl.name.getText();
        if (hoistedSymbols.has(name)) {
            return;
        }
        hoistedSymbols.add(name);
        const isLetOrConst =
            (decl.parent.flags & (ts.NodeFlags.Let | ts.NodeFlags.Const)) !==
                0;
        const initializer = isLetOrConst ? "jspp::uninitialized" : "undefined";
        code +=
            `${this.indent()}auto ${name} = std::make_shared<jspp::JsValue>(${initializer});
`;
    });

    // 2. Assign all hoisted functions first
    funcDecls.forEach((stmt) => {
        const funcName = stmt.name?.getText();
        if (funcName) {
            const lambda = this.generateLambda(stmt, true);
            code += `${this.indent()}*${funcName} = ${lambda};
`;
        }
    });

    // 3. Process other statements
    block.statements.forEach((stmt) => {
        if (ts.isFunctionDeclaration(stmt)) {
            // Do nothing, already handled
        } else if (ts.isVariableStatement(stmt)) {
            const isLetOrConst = (stmt.declarationList.flags &
                (ts.NodeFlags.Let | ts.NodeFlags.Const)) !==
                0;
            const contextForVisit = {
                ...context,
                isAssignmentOnly: !isLetOrConst,
            };
            const assignments = this.visit(
                stmt.declarationList,
                contextForVisit,
            );
            if (assignments) {
                code += `${this.indent()}${assignments};
`;
            }
        } else {
            code += this.visit(stmt, context);
        }
    });

    if (context.isFunctionBody) {
        const lastStatement = block.statements[block.statements.length - 1];
        if (!lastStatement || !ts.isReturnStatement(lastStatement)) {
            code += `${this.indent()}return undefined;
`;
        }
    }

    this.indentationLevel--;
    code += `${this.indent()}}
`;
    return code;
}

export function visitVariableStatement(
    this: CodeGenerator,
    node: ts.VariableStatement,
    context: VisitContext,
): string {
    return (
        this.indent() +
        this.visit(node.declarationList, context) +
        ";\n"
    );
}

export function visitForStatement(
    this: CodeGenerator,
    node: ts.ForStatement,
    context: VisitContext,
): string {
    const forStmt = node as ts.ForStatement;
    let code = "";
    this.indentationLevel++; // Enter a new scope for the for loop

    // Handle initializer
    let initializerCode = "";
    if (forStmt.initializer) {
        if (ts.isVariableDeclarationList(forStmt.initializer)) {
            const varDeclList = forStmt.initializer;
            const isLetOrConst = (varDeclList.flags &
                (ts.NodeFlags.Let | ts.NodeFlags.Const)) !==
                0;
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
            initializerCode = this.visit(forStmt.initializer, context);
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

export function visitForInStatement(
    this: CodeGenerator,
    node: ts.ForInStatement,
    context: VisitContext,
): string {
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
                `${this.indent()}auto ${varName} = std::make_shared<jspp::JsValue>(undefined);
`;
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
        `${this.indent()}{ std::vector<std::string> ${keysVar} = jspp::Access::get_object_keys(${derefExpr});
`;
    code += `${this.indent()}for (const auto& ${varName}_str : ${keysVar}) {
`;
    this.indentationLevel++;
    code +=
        `${this.indent()}*${varName} = jspp::Object::make_string(${varName}_str);
`;
    code += this.visit(forIn.statement, {
        ...context,
        isFunctionBody: false,
    });
    this.indentationLevel--;
    code += `${this.indent()}}}
`;
    this.indentationLevel--; // Exit the scope for the for-in loop
    return code;
}

export function visitForOfStatement(
    this: CodeGenerator,
    node: ts.ForOfStatement,
    context: VisitContext,
): string {
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
                `${this.indent()}auto ${varName} = std::make_shared<jspp::JsValue>(undefined);
`;
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
    const arrayPtr = this.generateUniqueName("__array_ptr_", new Set());

    code +=
        `${this.indent()}{ auto ${arrayPtr} = std::any_cast<std::shared_ptr<jspp::JsArray>>(${derefIterable});
`;
    code +=
        `${this.indent()}for (const auto& ${varName}_val : ${arrayPtr}->properties) {
`;
    this.indentationLevel++;
    code += `${this.indent()}*${varName} = ${varName}_val;
`;
    code += this.visit(forOf.statement, {
        ...context,
        isFunctionBody: false,
    });
    this.indentationLevel--;
    code += `${this.indent()}}}
`;
    this.indentationLevel--; // Exit the scope for the for-of loop

    return code;
}

export function visitIfStatement(
    this: CodeGenerator,
    node: ts.IfStatement,
    context: VisitContext,
): string {
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

export function visitExpressionStatement(
    this: CodeGenerator,
    node: ts.ExpressionStatement,
    context: VisitContext,
): string {
    return (
        this.indent() +
        this.visit(node.expression, context) +
        ";\n"
    );
}

export function visitThrowStatement(
    this: CodeGenerator,
    node: ts.ThrowStatement,
    context: VisitContext,
): string {
    const throwStmt = node as ts.ThrowStatement;
    const expr = this.visit(throwStmt.expression, context);
    return `${this.indent()}throw jspp::JsValue(${expr});
`;
}

export function visitTryStatement(
    this: CodeGenerator,
    node: ts.TryStatement,
    context: VisitContext,
): string {
    const tryStmt = node as ts.TryStatement;

    if (tryStmt.finallyBlock) {
        const declaredSymbols = new Set<string>();
        this.getDeclaredSymbols(tryStmt.tryBlock).forEach((s) =>
            declaredSymbols.add(s)
        );
        if (tryStmt.catchClause) {
            this.getDeclaredSymbols(tryStmt.catchClause).forEach((s) =>
                declaredSymbols.add(s)
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

        let code = `${this.indent()}{
`;
        this.indentationLevel++;

        code += `${this.indent()}jspp::JsValue ${resultVarName};
`;
        code += `${this.indent()}bool ${hasReturnedFlagName} = false;
`;

        const finallyBlockCode = this.visit(tryStmt.finallyBlock, {
            ...context,
            isFunctionBody: false,
        });
        code +=
            `${this.indent()}auto ${finallyLambdaName} = [&]() ${finallyBlockCode.trim()};
`;

        code += `${this.indent()}try {
`;
        this.indentationLevel++;

        code += `${this.indent()}${resultVarName} = ([&]() -> jspp::JsValue {
`;
        this.indentationLevel++;

        const innerContext: VisitContext = {
            ...context,
            isFunctionBody: false,
            isInsideTryCatchLambda: true,
            hasReturnedFlag: hasReturnedFlagName,
        };

        code += `${this.indent()}try {
`;
        this.indentationLevel++;
        code += this.visit(tryStmt.tryBlock, innerContext);
        this.indentationLevel--;
        code += `${this.indent()}}
`;

        if (tryStmt.catchClause) {
            const exceptionName = this.generateUniqueExceptionName(
                tryStmt.catchClause.variableDeclaration?.name.getText(),
            );
            const catchContext = { ...innerContext, exceptionName };
            code +=
                `${this.indent()}catch (const jspp::JsValue& ${exceptionName}) {
`;
            this.indentationLevel++;
            code += this.visit(tryStmt.catchClause.block, catchContext);
            this.indentationLevel--;
            code += `${this.indent()}}
`;
        } else {
            code += `${this.indent()}catch (...) { throw; }
`;
        }

        code += `${this.indent()}return undefined;
`;

        this.indentationLevel--;
        code += `${this.indent()}})();
`;

        this.indentationLevel--;
        code += `${this.indent()}} catch (...) {
`;
        this.indentationLevel++;
        code += `${this.indent()}${finallyLambdaName}();
`;
        code += `${this.indent()}throw;
`;
        this.indentationLevel--;
        code += `${this.indent()}}
`;

        code += `${this.indent()}${finallyLambdaName}();
`;

        code += `${this.indent()}if (${hasReturnedFlagName}) {
`;
        this.indentationLevel++;
        code += `${this.indent()}return ${resultVarName};
`;
        this.indentationLevel--;
        code += `${this.indent()}}
`;

        this.indentationLevel--;
        code += `${this.indent()}}
`;
        return code;
    } else {
        const exceptionName = this.generateUniqueExceptionName(
            tryStmt.catchClause?.variableDeclaration?.name.getText(),
        );
        const newContext = {
            ...context,
            isFunctionBody: false,
            exceptionName,
        };
        let code = `${this.indent()}try `;
        code += this.visit(tryStmt.tryBlock, newContext);
        if (tryStmt.catchClause) {
            code += ` catch (const jspp::JsValue& ${exceptionName}) `;
            code += this.visit(tryStmt.catchClause, newContext);
        }
        return code;
    }
}

export function visitCatchClause(
    this: CodeGenerator,
    node: ts.CatchClause,
    context: VisitContext,
): string {
    const catchClause = node as ts.CatchClause;
    const exceptionName = context.exceptionName;
    if (!exceptionName) {
        // This should not happen if it's coming from a TryStatement
        throw new Error(
            "Compiler bug: exceptionName not found in context for CatchClause",
        );
    }

    if (catchClause.variableDeclaration) {
        const varName = catchClause.variableDeclaration.name.getText();
        let code = `{
`;
        this.indentationLevel++;
        code += `${this.indent()}{
`;
        this.indentationLevel++;

        // Always create the JS exception variable.
        code +=
            `${this.indent()}auto ${varName} = std::make_shared<jspp::JsValue>(jspp::Exception::parse_error_from_value(${exceptionName}));
`;

        // Shadow the C++ exception variable *only if* the names don't clash.
        if (varName !== exceptionName) {
            code +=
                `${this.indent()}auto ${exceptionName} = std::make_shared<jspp::JsValue>(undefined);
`;
        }

        code += this.visit(catchClause.block, context);
        this.indentationLevel--;
        code += `${this.indent()}}
`;
        this.indentationLevel--;
        code += `${this.indent()}}
`;
        return code;
    } else {
        // No variable in the catch clause, e.g., `catch { ... }`
        let code = `{
`; // Alway create block scope
        code += this.visit(catchClause.block, context);
        code += `${this.indent()}}
`;
        return code;
    }
}

export function visitReturnStatement(
    this: CodeGenerator,
    node: ts.ReturnStatement,
    context: VisitContext,
): string {
    if (context.isMainContext) {
        return `${this.indent()}jspp::Exception::throw_invalid_return_statement();
`;
    }

    const returnStmt = node as ts.ReturnStatement;

    if (context.isInsideTryCatchLambda && context.hasReturnedFlag) {
        let returnCode = `${this.indent()}${context.hasReturnedFlag} = true;
`;
        if (returnStmt.expression) {
            const expr = returnStmt.expression;
            const exprText = this.visit(expr, context);
            if (ts.isIdentifier(expr)) {
                const scope = this.getScopeForNode(expr);
                const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                    expr.text,
                    scope,
                );
                if (!typeInfo) {
                    returnCode +=
                        `${this.indent()}jspp::Exception::throw_unresolved_reference(${
                            this.getJsVarName(expr)
                        });
`; // THROWS, not returns
                }
                if (
                    typeInfo &&
                    !typeInfo.isParameter &&
                    !typeInfo.isBuiltin
                ) {
                    returnCode +=
                        `${this.indent()}return jspp::Access::deref(${exprText}, ${
                            this.getJsVarName(expr)
                        });
`;
                } else {
                    returnCode += `${this.indent()}return ${exprText};
`;
                }
            } else {
                returnCode += `${this.indent()}return ${exprText};
`;
            }
        } else {
            returnCode += `${this.indent()}return undefined;
`;
        }
        return returnCode;
    }

    if (returnStmt.expression) {
        const expr = returnStmt.expression;
        const exprText = this.visit(expr, context);
        if (ts.isIdentifier(expr)) {
            const scope = this.getScopeForNode(expr);
            const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                expr.text,
                scope,
            );
            if (!typeInfo) {
                return `${this.indent()}jspp::Exception::throw_unresolved_reference(${
                    this.getJsVarName(expr)
                });
`; // THROWS, not returns
            }
            if (
                typeInfo &&
                !typeInfo.isParameter &&
                !typeInfo.isBuiltin
            ) {
                return `${this.indent()}return jspp::Access::deref(${exprText}, ${
                    this.getJsVarName(expr)
                });
`;
            }
        }
        return `${this.indent()}return ${exprText};
`;
    }
    return `${this.indent()}return undefined;
`;
}
