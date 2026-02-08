import ts from "typescript";

import { DeclaredSymbols } from "../../ast/symbols.js";
import { constants } from "../constants.js";
import { CompilerError } from "../error.js";
import {
  collectBlockScopedDeclarations,
  collectFunctionScopedDeclarations,
  shouldIgnoreStatement,
} from "./helpers.js";
import { CodeGenerator } from "./index.js";
import type { VisitContext } from "./visitor.js";

export function visitSourceFile(
    this: CodeGenerator,
    node: ts.SourceFile,
    context: VisitContext,
): string {
    const sourceFile = node as ts.SourceFile;
    let code = "";

    // 1. Collect all var declarations (recursively) + top-level let/const
    const varDecls = collectFunctionScopedDeclarations(sourceFile);
    const topLevelLetConst = collectBlockScopedDeclarations(
        sourceFile.statements,
    );

    const funcDecls = sourceFile.statements.filter((s) =>
        ts.isFunctionDeclaration(s) && !!s.body
    ) as ts.FunctionDeclaration[];
    const classDecls = sourceFile.statements.filter(ts.isClassDeclaration);
    const enumDecls = sourceFile.statements.filter(ts.isEnumDeclaration);

    const hoistedSymbols = new DeclaredSymbols();

    // Hoist function declarations
    funcDecls.forEach((func) => {
        code += this.hoistDeclaration(func, hoistedSymbols, node);
    });

    // Hoist class declarations
    classDecls.forEach((cls) => {
        code += this.hoistDeclaration(cls, hoistedSymbols, node);
    });

    // Hoist enum declarations
    enumDecls.forEach((enm) => {
        code += this.hoistDeclaration(enm, hoistedSymbols, node);
    });

    // Hoist variable declarations (var)
    varDecls.forEach((decl) => {
        code += this.hoistDeclaration(decl, hoistedSymbols, node);
    });

    // Hoist top-level let/const
    topLevelLetConst.forEach((decl) => {
        code += this.hoistDeclaration(decl, hoistedSymbols, node);
    });

    // Compile symbols for other statements (excluding function)
    const globalScopeSymbols = this.prepareScopeSymbolsForVisit(
        context.globalScopeSymbols,
        context.localScopeSymbols,
    );
    const localScopeSymbols = new DeclaredSymbols(hoistedSymbols); // hoistedSymbols becomes new local

    // 2. Assign all hoisted functions first
    const contextForFunctions = {
        ...context,
        localScopeSymbols: new DeclaredSymbols(
            context.localScopeSymbols,
            hoistedSymbols,
        ),
    };

    funcDecls.forEach((stmt) => {
        const funcName = stmt.name?.getText();
        if (!funcName) return;
        const symbol = hoistedSymbols.get(funcName);
        if (!symbol) return;

        // Mark before further visits
        this.markSymbolAsInitialized(
            funcName,
            contextForFunctions.globalScopeSymbols,
            contextForFunctions.localScopeSymbols,
        );
        this.markSymbolAsInitialized(
            funcName,
            globalScopeSymbols,
            localScopeSymbols,
        );

        // Generate and update self name
        const nativeName = this.generateUniqueName(
            `__${funcName}_native_`,
            hoistedSymbols,
        );
        hoistedSymbols.update(funcName, {
            features: {
                nativeName,
                parameters: this.checkFunctionParams(stmt.parameters),
            },
        });

        // Generate lambda components
        const lambdaComps = this.generateLambdaComponents(
            stmt,
            contextForFunctions,
            {
                isAssignment: true,
                nativeName,
                noTypeSignature: true,
            },
        );

        // Generate native lambda
        const nativeLambda = this.generateNativeLambda(lambdaComps);
        code += `${this.indent()}auto ${nativeName} = ${nativeLambda};\n`;

        // Generate AnyValue wrapped lamda
        if (
            this.isFunctionUsedAsValue(stmt, node) ||
            this.isFunctionUsedBeforeDeclaration(funcName, node)
        ) {
            const wrappedLambda = this.generateWrappedLambda(lambdaComps);
            code += `${this.indent()}*${funcName} = ${wrappedLambda};\n`;
        }
    });

    // 3. Process other statements
    sourceFile.statements.forEach((stmt) => {
        if (ts.isFunctionDeclaration(stmt)) {
            // Already handled
        } else if (ts.isVariableStatement(stmt)) {
            code += this.visit(stmt, {
                ...context,
                globalScopeSymbols,
                localScopeSymbols,
            });
        } else {
            code += this.visit(stmt, {
                ...context,
                isFunctionBody: false,
                globalScopeSymbols,
                localScopeSymbols,
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
    context.currentScopeNode = node; // Update scope node

    let code = `${this.indent()}{\n`;
    this.indentationLevel++;
    const block = node as ts.Block;

    // Collect ONLY block-scoped declarations (let/const)
    const blockScopedDecls = collectBlockScopedDeclarations(block.statements);

    const funcDecls = block.statements.filter((s) =>
        ts.isFunctionDeclaration(s) && !!s.body
    ) as ts.FunctionDeclaration[];
    const classDecls = block.statements.filter(ts.isClassDeclaration);
    const enumDecls = block.statements.filter(ts.isEnumDeclaration);

    const hoistedSymbols = new DeclaredSymbols();

    // 1. Hoist all function declarations
    funcDecls.forEach((func) => {
        code += this.hoistDeclaration(func, hoistedSymbols, node);
    });

    // Hoist class declarations
    classDecls.forEach((cls) => {
        code += this.hoistDeclaration(cls, hoistedSymbols, node);
    });

    // Hoist enum declarations
    enumDecls.forEach((enm) => {
        code += this.hoistDeclaration(enm, hoistedSymbols, node);
    });

    // Hoist variable declarations (let/const only)
    blockScopedDecls.forEach((decl) => {
        code += this.hoistDeclaration(decl, hoistedSymbols, node);
    });

    // Compile symbols for other statements (excluding function)
    const globalScopeSymbols = this.prepareScopeSymbolsForVisit(
        context.globalScopeSymbols,
        context.localScopeSymbols,
    );
    const localScopeSymbols = new DeclaredSymbols(hoistedSymbols); // hoistedSymbols becomes new local

    // 2. Assign all hoisted functions first
    const contextForFunctions = {
        ...context,
        localScopeSymbols: new DeclaredSymbols(
            context.localScopeSymbols,
            hoistedSymbols,
        ),
    };

    funcDecls.forEach((stmt) => {
        const funcName = stmt.name?.getText();
        if (!funcName) return;
        const symbol = hoistedSymbols.get(funcName);
        if (!symbol) return;

        // Mark before further visits
        this.markSymbolAsInitialized(
            funcName,
            contextForFunctions.globalScopeSymbols,
            contextForFunctions.localScopeSymbols,
        );
        this.markSymbolAsInitialized(
            funcName,
            globalScopeSymbols,
            localScopeSymbols,
        );

        // Generate and update self name
        const nativeName = this.generateUniqueName(
            `__${funcName}_native_`,
            hoistedSymbols,
        );
        hoistedSymbols.update(funcName, {
            features: {
                nativeName,
                parameters: this.checkFunctionParams(stmt.parameters),
            },
        });

        // Generate lambda components
        const lambdaComps = this.generateLambdaComponents(
            stmt,
            contextForFunctions,
            {
                isAssignment: true,
                nativeName,
                noTypeSignature: true,
            },
        );

        // Generate native lambda
        const nativeLambda = this.generateNativeLambda(lambdaComps);
        code += `${this.indent()}auto ${nativeName} = ${nativeLambda};\n`;

        // Generate AnyValue wrapped lamda
        if (
            this.isFunctionUsedAsValue(stmt, node) ||
            this.isFunctionUsedBeforeDeclaration(funcName, node)
        ) {
            const wrappedLambda = this.generateWrappedLambda(lambdaComps);
            code += `${this.indent()}*${funcName} = ${wrappedLambda};\n`;
        }
    });

    // 3. Process other statements
    block.statements.forEach((stmt) => {
        if (ts.isFunctionDeclaration(stmt)) {
            // Do nothing, already handled
        } else if (ts.isVariableStatement(stmt)) {
            code += this.visit(stmt, {
                ...context,
                globalScopeSymbols,
                localScopeSymbols,
            });
        } else {
            code += this.visit(stmt, {
                ...context,
                isFunctionBody: false,
                globalScopeSymbols,
                localScopeSymbols,
            });
        }
    });

    if (context.isFunctionBody) {
        const lastStatement = block.statements[block.statements.length - 1];
        if (!lastStatement || !ts.isReturnStatement(lastStatement)) {
            code += `${this.indent()}${
                this.getReturnCommand(context)
            } jspp::Constants::UNDEFINED;\n`;
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
    if (shouldIgnoreStatement(node)) {
        return "";
    }
    const isLetOrConst = (node.declarationList.flags &
        (ts.NodeFlags.Let | ts.NodeFlags.Const)) !==
        0;
    const visitContext = {
        ...context,
        isAssignmentOnly: !isLetOrConst,
    };
    const assignments = this.visit(
        node.declarationList,
        visitContext,
    );
    if (assignments) {
        return `${this.indent()}${assignments};\n`;
    }
    return "";
}

export function visitTypeAliasDeclaration(
    this: CodeGenerator,
    node: ts.TypeAliasDeclaration,
    context: VisitContext,
): string {
    return "";
}

export function visitInterfaceDeclaration(
    this: CodeGenerator,
    node: ts.InterfaceDeclaration,
    context: VisitContext,
): string {
    return "";
}

export function visitEnumDeclaration(
    this: CodeGenerator,
    node: ts.EnumDeclaration,
    context: VisitContext,
): string {
    const name = node.name.getText();
    const scope = this.getScopeForNode(node);
    const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
        name,
        scope,
    )!;

    // Mark as initialized
    this.markSymbolAsInitialized(
        name,
        context.globalScopeSymbols,
        context.localScopeSymbols,
    );

    const enumVar = typeInfo.needsHeapAllocation ? `(*${name})` : name;

    let code =
        `${this.indent()}${enumVar} = jspp::AnyValue::make_object({});\n`;
    code += `${this.indent()}{\n`;
    this.indentationLevel++;
    code +=
        `${this.indent()}jspp::AnyValue lastVal = jspp::AnyValue::make_number(-1);\n`; // Previous value tracker

    for (const member of node.members) {
        const memberName = member.name.getText();
        let valueCode = "";

        // Handle member name (it could be a string literal or identifier)
        let key = "";
        if (ts.isIdentifier(member.name)) {
            key = `"${memberName}"`;
        } else if (ts.isStringLiteral(member.name)) {
            key = member.name.getText(); // Includes quotes
        } else {
            // Computed property names or numeric literals in enums are rarer but possible
            // For now assume simple enum
            key = `"${memberName}"`;
        }

        if (member.initializer) {
            // Visit initializer
            valueCode = this.visit(member.initializer, context);
        } else {
            // Auto-increment
            valueCode = `jspp::add(lastVal, 1)`;
        }

        code += `${this.indent()}lastVal = ${valueCode};\n`;
        code +=
            `${this.indent()}${enumVar}.set_own_property(${key}, lastVal);\n`;

        // Reverse mapping for numeric enums
        code += `${this.indent()}if (lastVal.is_number()) {\n`;
        this.indentationLevel++;
        code +=
            `${this.indent()}${enumVar}.set_own_property(lastVal, jspp::AnyValue::make_string(${key}));\n`;
        this.indentationLevel--;
        code += `${this.indent()}}\n`;
    }

    this.indentationLevel--;
    code += `${this.indent()}}\n`;
    return code;
}

export function visitModuleDeclaration(
    this: CodeGenerator,
    node: ts.ModuleDeclaration,
    context: VisitContext,
): string {
    return "";
}

export function visitImportDeclaration(
    this: CodeGenerator,
    node: ts.ImportDeclaration,
    context: VisitContext,
): string {
    return "";
}

export function visitImportEqualsDeclaration(
    this: CodeGenerator,
    node: ts.ImportEqualsDeclaration,
    context: VisitContext,
): string {
    return "";
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
    const isBinaryExpression = ts.isBinaryExpression(ifStmt.expression) &&
        constants.booleanOperators.includes(
            ifStmt.expression.operatorToken.kind,
        );

    const condition = this.visit(ifStmt.expression, {
        ...context,
        supportedNativeLiterals: isBinaryExpression ? ["boolean"] : undefined,
    });
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

    if (isBinaryExpression) {
        return `${this.indent()}if (${condition}) ${thenStmt}${elseStmt}`;
    }
    return `${this.indent()}if (jspp::is_truthy(${condition})) ${thenStmt}${elseStmt}`;
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
    return `${this.indent()}throw jspp::Exception(${expr});
`;
}

export function visitTryStatement(
    this: CodeGenerator,
    node: ts.TryStatement,
    context: VisitContext,
): string {
    const tryStmt = node as ts.TryStatement;

    if (context.isInsideAsyncFunction) {
        if (tryStmt.finallyBlock) {
            const declaredSymbols = new Set<string>(
                context.globalScopeSymbols.names,
            );
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

            const resultVarName = this.generateUniqueName(
                "__try_result_",
                declaredSymbols,
            );
            const hasReturnedFlagName = this.generateUniqueName(
                "__try_has_returned_",
                declaredSymbols,
            );
            const catchAllExPtrName = this.generateUniqueName(
                "__catch_all_exptr",
                declaredSymbols,
            );

            let code = `${this.indent()}{\n`;
            this.indentationLevel++;

            code += `${this.indent()}jspp::AnyValue ${resultVarName};\n`;
            code +=
                `${this.indent()}std::exception_ptr ${catchAllExPtrName} = nullptr;\n`;
            code += `${this.indent()}bool ${hasReturnedFlagName} = false;\n`;

            const returnType = "jspp::JsPromise";
            const returnCmd = "co_return";
            const callPrefix = "co_await ";

            code += `${this.indent()}try {\n`;
            this.indentationLevel++;

            code +=
                `${this.indent()}${resultVarName} = ${callPrefix}([=, &${hasReturnedFlagName}]() -> ${returnType} {\n`;
            this.indentationLevel++;

            const innerContext: VisitContext = {
                ...context,
                isFunctionBody: false,
                isInsideTryCatchLambda: true,
                hasReturnedFlag: hasReturnedFlagName,
            };

            const exPtr = this.generateUniqueName("__ex_ptr");
            code += `${this.indent()}std::exception_ptr ${exPtr} = nullptr;\n`;
            code += `${this.indent()}try {\n`;
            this.indentationLevel++;
            code += this.visit(tryStmt.tryBlock, innerContext);
            this.indentationLevel--;
            code +=
                `${this.indent()}} catch (...) { ${exPtr} = std::current_exception(); }\n`;

            if (tryStmt.catchClause) {
                const exceptionName = this.generateUniqueExceptionName(
                    tryStmt.catchClause.variableDeclaration?.name.getText(),
                    declaredSymbols,
                );

                const caughtValVar = this.generateUniqueName("__caught_val");
                const caughtFlagVar = this.generateUniqueName("__caught_flag");

                code +=
                    `${this.indent()}jspp::AnyValue ${caughtValVar} = jspp::Constants::UNDEFINED;\n`;
                code += `${this.indent()}bool ${caughtFlagVar} = false;\n`;

                code += `${this.indent()}if (${exPtr}) {\n`;
                this.indentationLevel++;
                code +=
                    `${this.indent()}try { std::rethrow_exception(${exPtr}); } catch (const std::exception& ${exceptionName}) {\n`;
                this.indentationLevel++;
                code +=
                    `${this.indent()}${caughtValVar} = jspp::Exception::exception_to_any_value(${exceptionName});\n`;
                code += `${this.indent()}${caughtFlagVar} = true;\n`;
                this.indentationLevel--;
                code += `${this.indent()}} catch (...) {\n`;
                this.indentationLevel++;
                code +=
                    `${this.indent()}${caughtValVar} = jspp::AnyValue::make_string("Unknown native exception");\n`;
                code += `${this.indent()}${caughtFlagVar} = true;\n`;
                this.indentationLevel--;
                code += `${this.indent()}}\n`;
                this.indentationLevel--;
                code += `${this.indent()}}\n`;

                code += `${this.indent()}if (${caughtFlagVar}) {\n`;
                this.indentationLevel++;

                code += `${this.indent()}{\n`; // Block scope
                this.indentationLevel++;

                if (tryStmt.catchClause.variableDeclaration) {
                    const varName = tryStmt.catchClause.variableDeclaration.name
                        .getText();
                    code +=
                        `${this.indent()}jspp::AnyValue ${varName} = ${caughtValVar};\n`;
                }

                const catchContext = { ...innerContext, exceptionName };
                code += this.visit(tryStmt.catchClause.block, catchContext);

                this.indentationLevel--;
                code += `${this.indent()}}\n`;

                this.indentationLevel--;
                code += `${this.indent()}}\n`;
            } else {
                code +=
                    `${this.indent()}if (${exPtr}) { std::rethrow_exception(${exPtr}); }\n`;
            }

            code +=
                `${this.indent()}${returnCmd} jspp::Constants::UNDEFINED;\n`;

            this.indentationLevel--;
            code += `${this.indent()}})();\n`;

            this.indentationLevel--;
            code +=
                `${this.indent()}} catch (...) { ${catchAllExPtrName} = std::current_exception(); }\n`;

            code += `${this.indent()}// finally block\n`;
            code += this.visit(tryStmt.finallyBlock, {
                ...context,
                isFunctionBody: false,
            });

            code += `${this.indent()}// re-throw or return\n`;
            code +=
                `${this.indent()}if (${catchAllExPtrName}) { std::rethrow_exception(${catchAllExPtrName}); }\n`;
            code +=
                `${this.indent()}if (${hasReturnedFlagName}) { ${returnCmd} ${resultVarName}; }\n`;

            this.indentationLevel--;
            code += `${this.indent()}}\n`;
            return code;
        } else {
            const exceptionName = this.generateUniqueExceptionName(
                tryStmt.catchClause?.variableDeclaration?.name.getText(),
                context.globalScopeSymbols,
                context.localScopeSymbols,
            );
            const newContext = {
                ...context,
                isFunctionBody: false,
                exceptionName,
            };
            const exPtr = this.generateUniqueName("__ex_ptr");

            let code = `${this.indent()}{\n`;
            this.indentationLevel++;
            code += `${this.indent()}std::exception_ptr ${exPtr} = nullptr;\n`;
            code += `${this.indent()}try {\n`;
            this.indentationLevel++;
            code += this.visit(tryStmt.tryBlock, newContext);
            this.indentationLevel--;
            code +=
                `${this.indent()}} catch (...) { ${exPtr} = std::current_exception(); }\n`;

            if (tryStmt.catchClause) {
                const caughtValVar = this.generateUniqueName("__caught_val");
                const caughtFlagVar = this.generateUniqueName("__caught_flag");

                code +=
                    `${this.indent()}jspp::AnyValue ${caughtValVar} = jspp::Constants::UNDEFINED;\n`;
                code += `${this.indent()}bool ${caughtFlagVar} = false;\n`;

                code += `${this.indent()}if (${exPtr}) {\n`;
                this.indentationLevel++;
                code +=
                    `${this.indent()}try { std::rethrow_exception(${exPtr}); } catch (const std::exception& ${exceptionName}) {\n`;
                this.indentationLevel++;
                code +=
                    `${this.indent()}${caughtValVar} = jspp::Exception::exception_to_any_value(${exceptionName});\n`;
                code += `${this.indent()}${caughtFlagVar} = true;\n`;
                this.indentationLevel--;
                code += `${this.indent()}} catch (...) {\n`;
                this.indentationLevel++;
                code +=
                    `${this.indent()}${caughtValVar} = jspp::AnyValue::make_string("Unknown native exception");\n`;
                code += `${this.indent()}${caughtFlagVar} = true;\n`;
                this.indentationLevel--;
                code += `${this.indent()}}\n`;
                this.indentationLevel--;
                code += `${this.indent()}}\n`;

                code += `${this.indent()}if (${caughtFlagVar}) {\n`;
                this.indentationLevel++;

                code += `${this.indent()}{\n`; // Block scope
                this.indentationLevel++;

                if (tryStmt.catchClause.variableDeclaration) {
                    const varName = tryStmt.catchClause.variableDeclaration.name
                        .getText();
                    code +=
                        `${this.indent()}jspp::AnyValue ${varName} = ${caughtValVar};\n`;
                }

                code += this.visit(tryStmt.catchClause.block, newContext);

                this.indentationLevel--;
                code += `${this.indent()}}\n`;

                this.indentationLevel--;
                code += `${this.indent()}}\n`;
            } else {
                code +=
                    `${this.indent()}if (${exPtr}) { std::rethrow_exception(${exPtr}); }\n`;
            }
            this.indentationLevel--;
            code += `${this.indent()}}\n`;
            return code;
        }
    }

    if (tryStmt.finallyBlock) {
        const declaredSymbols = new Set<string>(
            context.globalScopeSymbols.names,
        );
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

        const resultVarName = this.generateUniqueName(
            "__try_result_",
            declaredSymbols,
        );
        const hasReturnedFlagName = this.generateUniqueName(
            "__try_has_returned_",
            declaredSymbols,
        );
        const catchAllExPtrName = this.generateUniqueName(
            "__catch_all_exptr",
            declaredSymbols,
        );

        let code = `${this.indent()}{\n`;
        this.indentationLevel++;

        code += `${this.indent()}jspp::AnyValue ${resultVarName};\n`;
        code +=
            `${this.indent()}std::exception_ptr ${catchAllExPtrName} = nullptr;\n`;
        code += `${this.indent()}bool ${hasReturnedFlagName} = false;\n`;

        const returnType = "jspp::AnyValue";
        const returnCmd = "return";
        const callPrefix = "";

        code += `${this.indent()}try {\n`;
        this.indentationLevel++;

        code +=
            `${this.indent()}${resultVarName} = ${callPrefix}([=, &${hasReturnedFlagName}]() -> ${returnType} {\n`;
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
                context.globalScopeSymbols,
                context.localScopeSymbols,
            );
            const catchContext = { ...innerContext, exceptionName };
            code +=
                `${this.indent()}catch (const std::exception& ${exceptionName}) {\n`;
            this.indentationLevel++;
            // We cannot co_await here. For now, let's just visit the catch block.
            // If the catch block contains await, it will fail to compile.
            // TODO: properly handle async catch by moving it out of native C++ catch.
            code += this.visit(tryStmt.catchClause.block, catchContext);
            this.indentationLevel--;
            code += `${this.indent()}}\n`;
        } else {
            code += `${this.indent()}catch (...) { throw; }\n`;
        }

        code += `${this.indent()}${returnCmd} jspp::Constants::UNDEFINED;\n`;

        this.indentationLevel--;
        code += `${this.indent()}})();\n`;

        this.indentationLevel--;
        code +=
            `${this.indent()}} catch (...) { ${catchAllExPtrName} = std::current_exception(); }\n`;

        code += `${this.indent()}// finally block\n`;
        code += this.visit(tryStmt.finallyBlock, {
            ...context,
            isFunctionBody: false,
        });

        code += `${this.indent()}// re-throw or return\n`;
        code +=
            `${this.indent()}if (${catchAllExPtrName}) { std::rethrow_exception(${catchAllExPtrName}); }\n`;
        code +=
            `${this.indent()}if (${hasReturnedFlagName}) { ${returnCmd} ${resultVarName}; }\n`;

        this.indentationLevel--;
        code += `${this.indent()}}\n`;
        return code;
    } else {
        const exceptionName = this.generateUniqueExceptionName(
            tryStmt.catchClause?.variableDeclaration?.name.getText(),
            context.globalScopeSymbols,
            context.localScopeSymbols,
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
                `${this.indent()}catch (const std::exception& ${exceptionName}) `;
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
        throw new CompilerError(
            "exceptionName not found in context for CatchClause",
            node,
            "CompilerBug",
        );
    }

    if (catchClause.variableDeclaration) {
        const varName = catchClause.variableDeclaration.name.getText();
        let code = `{\n`;
        this.indentationLevel++;

        // The JS exception variable is always local to the catch block
        code +=
            `${this.indent()}jspp::AnyValue ${varName} = jspp::Exception::exception_to_any_value(${exceptionName});\n`;

        code += this.visit(catchClause.block, context);
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
        let exprText = this.visit(expr, context);
        if (ts.isIdentifier(expr)) {
            const scope = this.getScopeForNode(expr);
            const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                expr.text,
                scope,
            );
            if (!typeInfo) {
                return `${this.indent()}jspp::Exception::throw_unresolved_reference(${
                    this.getJsVarName(expr)
                })\n`; // THROWS, not returns
            }
            if (
                typeInfo &&
                !typeInfo.isParameter &&
                !typeInfo.isBuiltin
            ) {
                exprText = this.getDerefCode(
                    exprText,
                    this.getJsVarName(expr),
                    context,
                    typeInfo,
                );
            }
        }

        // Handle `yield*` expression
        if (!!node.asteriskToken) {
            let code = `${this.indent()}{\n`;
            this.indentationLevel++;

            const declaredSymbols = this.getDeclaredSymbols(expr);
            context.globalScopeSymbols.names.forEach((s) =>
                declaredSymbols.add(s)
            );
            const iterableRef = this.generateUniqueName(
                "__iter_ref",
                declaredSymbols,
            );
            const iterator = this.generateUniqueName("__iter", declaredSymbols);
            const nextFunc = this.generateUniqueName(
                "__next_func",
                declaredSymbols,
            );
            const nextRes = this.generateUniqueName(
                "__next_res",
                declaredSymbols,
            );

            const varName = this.getJsVarName(expr as ts.Identifier);
            code += `${this.indent()}auto ${iterableRef} = ${exprText};\n`;
            if (context.isInsideAsyncFunction) {
                code +=
                    `${this.indent()}auto ${iterator} = jspp::Access::get_async_object_value_iterator(${iterableRef}, ${varName});\n`;
            } else {
                code +=
                    `${this.indent()}auto ${iterator} = jspp::Access::get_object_value_iterator(${iterableRef}, ${varName});\n`;
            }
            code +=
                `${this.indent()}auto ${nextFunc} = ${iterator}.get_own_property("next");\n`;
            if (context.isInsideAsyncFunction) {
                code +=
                    `${this.indent()}auto ${nextRes} = co_await ${nextFunc}.call(${iterator}, {}, "next");\n`;
            } else {
                code +=
                    `${this.indent()}auto ${nextRes} = ${nextFunc}.call(${iterator}, {}, "next");\n`;
            }
            code +=
                `${this.indent()}while (!jspp::is_truthy(${nextRes}.get_own_property("done"))) {\n`;
            this.indentationLevel++;
            if (context.isInsideAsyncFunction) {
                code +=
                    `${this.indent()}co_yield co_await ${nextRes}.get_own_property("value");\n`;
            } else {
                code +=
                    `${this.indent()}co_yield ${nextRes}.get_own_property("value");\n`;
            }
            if (context.isInsideAsyncFunction) {
                code +=
                    `${this.indent()}${nextRes} = co_await ${nextFunc}.call(${iterator}, {}, "next");\n`;
            } else {
                code +=
                    `${this.indent()}${nextRes} = ${nextFunc}.call(${iterator}, {}, "next");\n`;
            }

            this.indentationLevel--;
            code += `${this.indent()}}\n`;

            return code;
        }

        const awaitPart = context.isInsideAsyncFunction ? "co_await " : "";
        return `${this.indent()}co_yield ${awaitPart}${exprText}`;
    }

    const awaitPart = context.isInsideAsyncFunction ? "co_await " : "";
    return `${this.indent()}co_yield ${awaitPart}jspp::Constants::UNDEFINED`;
}

export function visitReturnStatement(
    this: CodeGenerator,
    node: ts.ReturnStatement,
    context: VisitContext,
): string {
    if (context.isMainContext) {
        return `${this.indent()}jspp::Exception::throw_invalid_return_statement();\n`;
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
                        `${this.indent()}jspp::Exception::throw_unresolved_reference(${
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
                        context,
                        typeInfo,
                    );
                }
            }
            returnCode += `${this.indent()}${returnCmd} ${finalExpr};\n`;
        } else {
            returnCode +=
                `${this.indent()}${returnCmd} jspp::Constants::UNDEFINED;\n`;
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
                return `${this.indent()}jspp::Exception::throw_unresolved_reference(${
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
                    context,
                    typeInfo,
                );
            }
        }
        return `${this.indent()}${returnCmd} ${finalExpr};\n`;
    }
    return `${this.indent()}${returnCmd} jspp::Constants::UNDEFINED;\n`;
}
