import ts from "typescript";

import type { DeclaredSymbols } from "../../ast/types";
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

    const hoistedSymbols: DeclaredSymbols = new Map();

    // 1. Hoist function declarations
    funcDecls.forEach((func) => {
        code += this.hoistDeclaration(func, hoistedSymbols);
    });

    // Hoist variable declarations
    varDecls.forEach((decl) => {
        code += this.hoistDeclaration(decl, hoistedSymbols);
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
        const topLevelScopeSymbols = new Map(context.topLevelScopeSymbols);
        context.localScopeSymbols?.forEach((v, k) =>
            topLevelScopeSymbols.set(k, v) //  local now becomes top
        );
        const localScopeSymbols = hoistedSymbols; // hoistedSymbols becomes new local

        if (ts.isFunctionDeclaration(stmt)) {
            // Already handled
        } else if (ts.isVariableStatement(stmt)) {
            const isLetOrConst = (stmt.declarationList.flags &
                (ts.NodeFlags.Let | ts.NodeFlags.Const)) !==
                0;
            const contextForVisit = {
                ...context,
                topLevelScopeSymbols,
                localScopeSymbols,
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
            code += this.visit(stmt, {
                ...context,
                isFunctionBody: false,
                topLevelScopeSymbols,
                localScopeSymbols,
                // localScopeSymbols: undefined, // clear the localScopeSymbols for nested visit
                // topLevelScopeSymbols: undefined, // clear the topLevelScopeSymbols for nested visit
            });
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

    const hoistedSymbols: DeclaredSymbols = new Map();

    // 1. Hoist all function declarations
    funcDecls.forEach((func) => {
        code += this.hoistDeclaration(func, hoistedSymbols);
    });

    // Hoist variable declarations
    varDecls.forEach((decl) => {
        code += this.hoistDeclaration(decl, hoistedSymbols);
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
        const topLevelScopeSymbols = new Map(context.topLevelScopeSymbols);
        context.localScopeSymbols?.forEach((v, k) =>
            topLevelScopeSymbols.set(k, v) //  local now becomes top
        );
        const localScopeSymbols = hoistedSymbols; // hoistedSymbols becomes new local

        if (ts.isFunctionDeclaration(stmt)) {
            // Do nothing, already handled
        } else if (ts.isVariableStatement(stmt)) {
            const isLetOrConst = (stmt.declarationList.flags &
                (ts.NodeFlags.Let | ts.NodeFlags.Const)) !==
                0;
            const contextForVisit = {
                ...context,
                topLevelScopeSymbols,
                localScopeSymbols,
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
            code += this.visit(stmt, {
                ...context,
                isFunctionBody: false,
                topLevelScopeSymbols,
                localScopeSymbols,
                // localScopeSymbols: undefined, // clear the localScopeSymbols for nested visit
                // topLevelScopeSymbols: undefined, // clear the topLevelScopeSymbols for nested visit
            });
        }
    });

    if (context.isFunctionBody) {
        const lastStatement = block.statements[block.statements.length - 1];
        if (!lastStatement || !ts.isReturnStatement(lastStatement)) {
            code += `${this.indent()}${
                this.getReturnCommand(context)
            } jspp::AnyValue::make_undefined();\n`;
        }
    }

    this.indentationLevel--;
    code += `${this.indent()}}\n`;
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

export function visitBreakStatement(
    this: CodeGenerator,
    node: ts.BreakStatement,
    context: VisitContext,
): string {
    if (node.label) {
        return `${this.indent()}goto ${node.label.text}_break;\n`;
    }
    if (context.switchBreakLabel) {
        return `${this.indent()}goto ${context.switchBreakLabel};\n`;
    }
    return `${this.indent()}break;\n`;
}

export function visitContinueStatement(
    this: CodeGenerator,
    node: ts.ContinueStatement,
    context: VisitContext,
): string {
    if (node.label) {
        return `${this.indent()}goto ${node.label.text}_continue;\n`;
    }
    return `${this.indent()}continue;\n`;
}

export function visitLabeledStatement(
    this: CodeGenerator,
    node: ts.LabeledStatement,
    context: VisitContext,
): string {
    const label = node.label.text;
    const statement = node.statement;

    const isLoop = ts.isForStatement(statement) ||
        ts.isForInStatement(statement) ||
        ts.isForOfStatement(statement) ||
        ts.isWhileStatement(statement) ||
        ts.isDoStatement(statement);

    const statementContext = { ...context, currentLabel: label };
    if (ts.isSwitchStatement(statement)) {
        return this.visit(statement, statementContext);
    }

    const statementCode = this.visit(statement, statementContext);

    if (isLoop) {
        return statementCode;
    }

    // A non-loop statement can only be broken from.
    // We wrap it in a labeled block.
    let code = `${this.indent()}${label}: {\n`;
    this.indentationLevel++;
    code += statementCode;
    this.indentationLevel--;
    code += `${this.indent()}}\n`;
    code += `${this.indent()}${label}_break:; // break target for ${label}\n`;
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
    return `${this.indent()}if ((${condition}).is_truthy()) ${thenStmt}${elseStmt}`;
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
    return `${this.indent()}throw jspp::RuntimeError(${expr});
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

        let code = `${this.indent()}{\n`;
        this.indentationLevel++;

        code += `${this.indent()}jspp::AnyValue ${resultVarName};\n`;
        code += `${this.indent()}bool ${hasReturnedFlagName} = false;\n`;

        const finallyBlockCode = this.visit(tryStmt.finallyBlock, {
            ...context,
            isFunctionBody: false,
        });
        code +=
            `${this.indent()}auto ${finallyLambdaName} = [=]() ${finallyBlockCode.trim()};\n`;

        code += `${this.indent()}try {\n`;
        this.indentationLevel++;

        code +=
            `${this.indent()}${resultVarName} = ([=, &${hasReturnedFlagName}]() -> jspp::AnyValue {\n`;
        this.indentationLevel++;

        const innerContext: VisitContext = {
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
                tryStmt.catchClause.variableDeclaration?.name.getText(),
            );
            const catchContext = { ...innerContext, exceptionName };
            code +=
                `${this.indent()}catch (const std::exception& ${exceptionName}) {\n`;
            this.indentationLevel++;
            code += this.visit(tryStmt.catchClause.block, catchContext);
            this.indentationLevel--;
            code += `${this.indent()}}\n`;
        } else {
            code += `${this.indent()}catch (...) { throw; }\n`;
        }

        code += `${this.indent()}${
            this.getReturnCommand(context)
        } jspp::AnyValue::make_undefined();\n`;

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
            code += ` catch (const std::exception& ${exceptionName}) `;
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
        let code = `{\n`;
        this.indentationLevel++;
        code += `${this.indent()}{\n`;
        this.indentationLevel++;

        // The JS exception variable is always local to the catch block
        code +=
            `${this.indent()}jspp::AnyValue ${varName} = jspp::RuntimeError::error_to_value(${exceptionName});\n`;

        // Shadow the C++ exception variable *only if* the names don't clash.
        if (varName !== exceptionName) {
            code +=
                `${this.indent()}auto ${exceptionName} = std::make_shared<jspp::AnyValue>(jspp::AnyValue::make_undefined());\n`;
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

export function visitYieldExpression(
    this: CodeGenerator,
    node: ts.YieldExpression,
    context: VisitContext,
): string {
    if (node.expression) {
        const expr = node.expression;
        const exprText = this.visit(expr, context);
        if (ts.isIdentifier(expr)) {
            const scope = this.getScopeForNode(expr);
            const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                expr.text,
                scope,
            );
            if (!typeInfo) {
                return `${this.indent()}jspp::RuntimeError::throw_unresolved_reference_error(${
                    this.getJsVarName(expr)
                })\n`; // THROWS, not returns
            }
            if (
                typeInfo &&
                !typeInfo.isParameter &&
                !typeInfo.isBuiltin
            ) {
                return `${this.indent()}co_yield ${
                    this.getDerefCode(
                        exprText,
                        this.getJsVarName(expr),
                        typeInfo,
                    )
                }`;
            }
        }
        return `${this.indent()}co_yield ${exprText}`;
    }

    return `${this.indent()}co_yield jspp::AnyValue::make_undefined()`;
}

export function visitReturnStatement(
    this: CodeGenerator,
    node: ts.ReturnStatement,
    context: VisitContext,
): string {
    if (context.isMainContext) {
        return `${this.indent()}jspp::RuntimeError::throw_invalid_return_statement_error();\n`;
    }

    const returnStmt = node as ts.ReturnStatement;
    const returnCmd = this.getReturnCommand(context);

    if (context.isInsideTryCatchLambda && context.hasReturnedFlag) {
        let returnCode = `${this.indent()}${context.hasReturnedFlag} = true;\n`;
        if (returnStmt.expression) {
            const expr = returnStmt.expression;
            const exprText = this.visit(expr, context);
            let finalExpr = exprText;
            if (ts.isIdentifier(expr)) {
                const scope = this.getScopeForNode(expr);
                const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                    expr.text,
                    scope,
                );
                if (!typeInfo) {
                    returnCode +=
                        `${this.indent()}jspp::RuntimeError::throw_unresolved_reference_error(${
                            this.getJsVarName(expr)
                        });\n`; // THROWS, not returns
                } else if (
                    typeInfo &&
                    !typeInfo.isParameter &&
                    !typeInfo.isBuiltin
                ) {
                    finalExpr = this.getDerefCode(
                        exprText,
                        this.getJsVarName(expr),
                        typeInfo,
                    );
                }
            }
            returnCode += `${this.indent()}${returnCmd} ${finalExpr};\n`;
        } else {
            returnCode +=
                `${this.indent()}${returnCmd} jspp::AnyValue::make_undefined();\n`;
        }
        return returnCode;
    }

    if (returnStmt.expression) {
        const expr = returnStmt.expression;
        const exprText = this.visit(expr, context);
        let finalExpr = exprText;
        if (ts.isIdentifier(expr)) {
            const scope = this.getScopeForNode(expr);
            const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                expr.text,
                scope,
            );
            if (!typeInfo) {
                return `${this.indent()}jspp::RuntimeError::throw_unresolved_reference_error(${
                    this.getJsVarName(expr)
                });\n`; // THROWS, not returns
            }
            if (
                typeInfo &&
                !typeInfo.isParameter &&
                !typeInfo.isBuiltin
            ) {
                finalExpr = this.getDerefCode(
                    exprText,
                    this.getJsVarName(expr),
                    typeInfo,
                );
            }
        }
        return `${this.indent()}${returnCmd} ${finalExpr};\n`;
    }
    return `${this.indent()}${returnCmd} jspp::AnyValue::make_undefined();\n`;
}
