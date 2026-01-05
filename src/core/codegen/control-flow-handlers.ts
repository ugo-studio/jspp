import ts from "typescript";

import { DeclarationType, DeclaredSymbols } from "../../ast/symbols.js";
import { CodeGenerator } from "./index.js";
import type { VisitContext } from "./visitor.js";

export function visitForStatement(
    this: CodeGenerator,
    node: ts.ForStatement,
    context: VisitContext,
): string {
    const forStmt = node as ts.ForStatement;
    let code = "";

    if (context.currentLabel) {
        code += `${this.indent()}${context.currentLabel}: {\n`;
        this.indentationLevel++;
    }

    this.indentationLevel++; // Enter a new scope for the for loop

    // Handle initializer
    let initializerCode = "";
    let conditionContext: VisitContext = {
        ...context,
        topLevelScopeSymbols: this.prepareScopeSymbolsForVisit(
            context.topLevelScopeSymbols,
            context.localScopeSymbols,
        ),
    };
    if (forStmt.initializer) {
        if (ts.isVariableDeclarationList(forStmt.initializer)) {
            const varDeclList = forStmt.initializer;
            const isLetOrConst = (varDeclList.flags &
                (ts.NodeFlags.Let | ts.NodeFlags.Const)) !==
                0;
            if (isLetOrConst) {
                const decl = varDeclList.declarations[0];
                if (decl) {
                    const name = decl.name.getText();
                    const initValue = decl.initializer
                        ? this.visit(decl.initializer, context)
                        : "jspp::Constants::UNDEFINED";

                    const scope = this.getScopeForNode(decl);
                    const typeInfo = this.typeAnalyzer.scopeManager
                        .lookupFromScope(
                            name,
                            scope,
                        )!;

                    conditionContext.localScopeSymbols = new DeclaredSymbols();

                    const declType = (varDeclList.flags &
                            (ts.NodeFlags.Let)) !==
                            0
                        ? DeclarationType.let
                        : DeclarationType.const;
                    conditionContext.localScopeSymbols.add(name, {
                        type: declType,
                        checked: { initialized: true },
                    });

                    if (typeInfo.needsHeapAllocation) {
                        initializerCode =
                            `auto ${name} = std::make_shared<jspp::AnyValue>(${initValue})`;
                    } else {
                        initializerCode =
                            `jspp::AnyValue ${name} = ${initValue}`;
                    }
                }
            } else {
                initializerCode = this.visit(forStmt.initializer, {
                    ...context,
                    isAssignmentOnly: true,
                });
            }
        } else {
            initializerCode = this.visit(forStmt.initializer, context);
        }
    }

    code += `${this.indent()}for (${initializerCode}; `;
    if (forStmt.condition) {
        code += `is_truthy(${this.visit(forStmt.condition, conditionContext)})`;
    }
    code += "; ";
    if (forStmt.incrementor) {
        code += this.visit(forStmt.incrementor, context);
    }
    code += ") ";

    const statementCode = this.visit(forStmt.statement, {
        ...context,
        currentLabel: undefined,
        isFunctionBody: false,
    }).trim();

    if (ts.isBlock(node.statement)) {
        let blockContent = statementCode.substring(
            1,
            statementCode.length - 2,
        ); // remove curly braces
        if (context.currentLabel) {
            blockContent +=
                `${this.indent()}${context.currentLabel}_continue:;\n`;
        }
        code += `{\n${blockContent}}\n`;
    } else {
        code += `{\n`;
        code += statementCode;
        if (context.currentLabel) {
            code += `${this.indent()}${context.currentLabel}_continue:;\n`;
        }
        code += `${this.indent()}}\n`;
    }

    this.indentationLevel--; // Exit the scope for the for loop

    if (context.currentLabel) {
        this.indentationLevel--;
        code += `${this.indent()}}\n`;
        code +=
            `${this.indent()}${context.currentLabel}_break:; // break target\n`;
    }

    return code;
}

export function visitForInStatement(
    this: CodeGenerator,
    node: ts.ForInStatement,
    context: VisitContext,
): string {
    const forIn = node as ts.ForInStatement;

    let code = "";
    if (context.currentLabel) {
        code += `${this.indent()}${context.currentLabel}: {\n`;
        this.indentationLevel++;
    }

    code += `${this.indent()}{\n`;
    this.indentationLevel++; // Enter a new scope for the for-in loop
    let varName = "";
    let assignmentTarget = "";

    if (ts.isVariableDeclarationList(forIn.initializer)) {
        const decl = forIn.initializer.declarations[0];
        if (decl) {
            varName = decl.name.getText();
            const scope = this.getScopeForNode(decl);
            const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                varName,
                scope,
            )!;
            if (typeInfo.needsHeapAllocation) {
                code +=
                    `${this.indent()}auto ${varName} = std::make_shared<jspp::AnyValue>(jspp::Constants::UNDEFINED);\n`;
                assignmentTarget = `*${varName}`;
            } else {
                code +=
                    `${this.indent()}jspp::AnyValue ${varName} = jspp::Constants::UNDEFINED;\n`;
                assignmentTarget = varName;
            }
        }
    } else if (ts.isIdentifier(forIn.initializer)) {
        varName = forIn.initializer.getText();
        const scope = this.getScopeForNode(forIn.initializer);
        const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
            varName,
            scope,
        )!;
        assignmentTarget = typeInfo.needsHeapAllocation
            ? `*${varName}`
            : varName;
    }

    const expr = forIn.expression;
    const exprText = this.visit(expr, context);

    let derefExpr = exprText;
    if (ts.isIdentifier(expr)) {
        const scope = this.getScopeForNode(expr);
        const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
            expr.getText(),
            scope,
        )!;
        derefExpr = this.getDerefCode(
            exprText,
            this.getJsVarName(expr),
            context,
            typeInfo,
        );
    }

    const keysVar = this.generateUniqueName("__keys_", new Set([varName]));
    code +=
        `${this.indent()}std::vector<std::string> ${keysVar} = jspp::Access::get_object_keys(${derefExpr});\n`;
    code += `${this.indent()}for (const auto& ${varName}_str : ${keysVar}) {\n`;
    this.indentationLevel++;
    code +=
        `${this.indent()}${assignmentTarget} = jspp::AnyValue::make_string(${varName}_str);\n`;
    code += this.visit(forIn.statement, {
        ...context,
        currentLabel: undefined,
        isFunctionBody: false,
    });
    this.indentationLevel--;
    if (context.currentLabel) {
        code += `${this.indent()}${context.currentLabel}_continue:;\n`;
    }
    code += `${this.indent()}}\n`;
    this.indentationLevel--; // Exit the scope for the for-in loop
    code += `${this.indent()}}\n`;

    if (context.currentLabel) {
        this.indentationLevel--;
        code += `${this.indent()}}\n`;
        code +=
            `${this.indent()}${context.currentLabel}_break:; // break target\n`;
    }

    return code;
}

export function visitForOfStatement(
    this: CodeGenerator,
    node: ts.ForOfStatement,
    context: VisitContext,
): string {
    const forOf = node as ts.ForOfStatement;

    let code = "";
    if (context.currentLabel) {
        code += `${this.indent()}${context.currentLabel}: {\n`;
        this.indentationLevel++;
    }

    this.indentationLevel++; // Enter a new scope for the for-of loop
    let elemName = "";
    let assignmentTarget = "";

    code += `${this.indent()}{\n`;
    if (ts.isVariableDeclarationList(forOf.initializer)) {
        const decl = forOf.initializer.declarations[0];
        if (decl) {
            elemName = decl.name.getText();
            const scope = this.getScopeForNode(decl);
            const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                elemName,
                scope,
            )!;
            if (typeInfo.needsHeapAllocation) {
                code +=
                    `${this.indent()}auto ${elemName} = std::make_shared<jspp::AnyValue>(jspp::Constants::UNDEFINED);\n`;
                assignmentTarget = `*${elemName}`;
            } else {
                code +=
                    `${this.indent()}jspp::AnyValue ${elemName} = jspp::Constants::UNDEFINED;\n`;
                assignmentTarget = elemName;
            }
        }
    } else if (ts.isIdentifier(forOf.initializer)) {
        elemName = forOf.initializer.getText();
        const scope = this.getScopeForNode(forOf.initializer);
        const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
            elemName,
            scope,
        )!;
        assignmentTarget = typeInfo.needsHeapAllocation
            ? `*${elemName}`
            : elemName;
    }

    const iterableExpr = this.visit(forOf.expression, context);
    let derefIterable = iterableExpr;
    if (ts.isIdentifier(forOf.expression)) {
        const scope = this.getScopeForNode(forOf.expression);
        const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
            forOf.expression.getText(),
            scope,
        )!;
        const varName = this.getJsVarName(forOf.expression as ts.Identifier);
        derefIterable = this.getDerefCode(
            iterableExpr,
            varName,
            context,
            typeInfo,
        );
    }

    const declaredSymbols = this.getDeclaredSymbols(forOf.statement);
    const iterableRef = this.generateUniqueName("__iter_ref", declaredSymbols);
    const iterator = this.generateUniqueName("__iter", declaredSymbols);
    const nextFunc = this.generateUniqueName("__next_func", declaredSymbols);
    const nextRes = this.generateUniqueName("__next_res", declaredSymbols);

    const isAwait = forOf.awaitModifier !== undefined;

    const varName = this.getJsVarName(forOf.expression as ts.Identifier);
    code += `${this.indent()}auto ${iterableRef} = ${derefIterable};\n`;
    if (isAwait) {
        code +=
            `${this.indent()}auto ${iterator} = jspp::Access::get_async_object_value_iterator(${iterableRef}, ${varName});\n`;
    } else {
        code +=
            `${this.indent()}auto ${iterator} = jspp::Access::get_object_value_iterator(${iterableRef}, ${varName});\n`;
    }
    code +=
        `${this.indent()}auto ${nextFunc} = ${iterator}.get_own_property("next");\n`;
    if (isAwait) {
        code +=
            `${this.indent()}auto ${nextRes} = co_await ${nextFunc}.call(${iterator}, {}, "next");\n`;
    } else {
        code +=
            `${this.indent()}auto ${nextRes} = ${nextFunc}.call(${iterator}, {}, "next");\n`;
    }
    code +=
        `${this.indent()}while (!is_truthy(${nextRes}.get_own_property("done"))) {\n`;
    this.indentationLevel++;
    code +=
        `${this.indent()}${assignmentTarget} = ${nextRes}.get_own_property("value");\n`;
    code += this.visit(forOf.statement, {
        ...context,
        currentLabel: undefined,
        isFunctionBody: false,
    });
    if (context.currentLabel) {
        code += `${this.indent()}${context.currentLabel}_continue:;\n`;
    }
    if (isAwait) {
        code +=
            `${this.indent()}${nextRes} = co_await ${nextFunc}.call(${iterator}, {}, "next");\n`;
    } else {
        code +=
            `${this.indent()}${nextRes} = ${nextFunc}.call(${iterator}, {}, "next");\n`;
    }
    this.indentationLevel--;
    code += `${this.indent()}}\n`;
    this.indentationLevel--; // Exit the scope for the for-of loop
    code += `${this.indent()}}\n`;

    if (context.currentLabel) {
        this.indentationLevel--;
        code += `${this.indent()}}\n`;
        code +=
            `${this.indent()}${context.currentLabel}_break:; // break target\n`;
    }

    return code;
}

export function visitWhileStatement(
    this: CodeGenerator,
    node: ts.WhileStatement,
    context: VisitContext,
): string {
    const condition = node.expression;
    const conditionText = condition.kind === ts.SyntaxKind.TrueKeyword ||
            condition.kind === ts.SyntaxKind.FalseKeyword
        ? condition.getText()
        : `is_truthy(${this.visit(condition, context)})`;

    let code = "";
    if (context.currentLabel) {
        code += `${this.indent()}${context.currentLabel}: {\n`;
        this.indentationLevel++;
    }

    code += `${this.indent()}while (${conditionText}) `;

    const statementCode = this.visit(node.statement, {
        ...context,
        currentLabel: undefined,
        isFunctionBody: false,
    }).trim();

    if (ts.isBlock(node.statement)) {
        let blockContent = statementCode.substring(
            1,
            statementCode.length - 2,
        ); // remove curly braces
        if (context.currentLabel) {
            blockContent +=
                `${this.indent()}${context.currentLabel}_continue:;\n`;
        }
        code += `{\n${blockContent}}\n`;
    } else {
        code += `{\n`;
        this.indentationLevel++;
        code += statementCode;
        if (context.currentLabel) {
            code += `${this.indent()}${context.currentLabel}_continue:;\n`;
        }
        this.indentationLevel--;
        code += `${this.indent()}}\n`;
    }

    if (context.currentLabel) {
        this.indentationLevel--;
        code += `${this.indent()}}\n`;
        code +=
            `${this.indent()}${context.currentLabel}_break:; // break target\n`;
    }

    return code;
}

export function visitDoStatement(
    this: CodeGenerator,
    node: ts.DoStatement,
    context: VisitContext,
): string {
    const condition = node.expression;
    const conditionText = `is_truthy(${this.visit(condition, context)})`;

    let code = "";
    if (context.currentLabel) {
        code += `${this.indent()}${context.currentLabel}: {\n`;
        this.indentationLevel++;
    }

    code += `${this.indent()}do `;

    const statementCode = this.visit(node.statement, {
        ...context,
        currentLabel: undefined,
        isFunctionBody: false,
    }).trim();

    if (ts.isBlock(node.statement)) {
        let blockContent = statementCode.substring(
            1,
            statementCode.length - 2,
        ); // remove curly braces
        if (context.currentLabel) {
            blockContent +=
                `${this.indent()}${context.currentLabel}_continue:;\n`;
        }
        code += `{\n${blockContent}}`;
    } else {
        code += `{\n`;
        this.indentationLevel++;
        code += statementCode;
        if (context.currentLabel) {
            code += `${this.indent()}${context.currentLabel}_continue:;\n`;
        }
        this.indentationLevel--;
        code += `${this.indent()}}`;
    }

    code += ` while (${conditionText});\n`;

    if (context.currentLabel) {
        this.indentationLevel--;
        code += `${this.indent()}}\n`;
        code +=
            `${this.indent()}${context.currentLabel}_break:; // break target\n`;
    }

    return code;
}

export function visitSwitchStatement(
    this: CodeGenerator,
    node: ts.SwitchStatement,
    context: VisitContext,
): string {
    const switchStmt = node as ts.SwitchStatement;
    let code = "";

    const declaredSymbols = this.getDeclaredSymbols(switchStmt.caseBlock);
    const switchBreakLabel = this.generateUniqueName(
        "__switch_break_",
        declaredSymbols,
    );
    const fallthroughVar = this.generateUniqueName(
        "__switch_fallthrough_",
        declaredSymbols,
    );

    if (context.currentLabel) {
        code += `${this.indent()}${context.currentLabel}: {\n`;
        this.indentationLevel++;
    }

    code += `${this.indent()}{\n`; // Wrap the entire switch logic in a block
    this.indentationLevel++;

    // Evaluate the switch expression once
    const expressionCode = this.visit(switchStmt.expression, context);
    const switchValueVar = this.generateUniqueName(
        "__switch_value_",
        declaredSymbols,
    );
    code +=
        `${this.indent()}const jspp::AnyValue ${switchValueVar} = ${expressionCode};\n`;
    code += `${this.indent()}bool ${fallthroughVar} = false;\n`;

    // Hoist variable declarations
    const hoistedSymbols = new DeclaredSymbols();
    for (const clause of switchStmt.caseBlock.clauses) {
        if (ts.isCaseClause(clause) || ts.isDefaultClause(clause)) {
            for (const stmt of clause.statements) {
                if (ts.isVariableStatement(stmt)) {
                    const varDecls = stmt.declarationList.declarations;
                    for (const decl of varDecls) {
                        code += this.hoistDeclaration(
                            decl,
                            hoistedSymbols,
                        );
                    }
                } else if (ts.isFunctionDeclaration(stmt)) {
                    code += this.hoistDeclaration(
                        stmt,
                        hoistedSymbols,
                    );
                }
            }
        }
    }

    // Prepare scope symbols for the switch block
    const topLevelScopeSymbols = this.prepareScopeSymbolsForVisit(
        context.topLevelScopeSymbols,
        context.localScopeSymbols,
    );

    let firstIf = true;

    for (const clause of switchStmt.caseBlock.clauses) {
        if (ts.isCaseClause(clause)) {
            const caseExprCode = this.visit(clause.expression, {
                ...context,
                currentLabel: undefined, // Clear currentLabel for nested visits
            });
            let condition = "";

            if (firstIf) {
                condition =
                    `(${fallthroughVar} || jspp::is_strictly_equal_to_primitive(${switchValueVar}, ${caseExprCode}))`;
                code += `${this.indent()}if ${condition} {\n`;
                firstIf = false;
            } else {
                condition =
                    `(${fallthroughVar} || jspp::is_strictly_equal_to_primitive(${switchValueVar}, ${caseExprCode}))`;
                code += `${this.indent()}if ${condition} {\n`;
            }

            this.indentationLevel++;
            code += `${this.indent()}${fallthroughVar} = true;\n`;
            for (const stmt of clause.statements) {
                if (ts.isFunctionDeclaration(stmt)) {
                    const funcName = stmt.name?.getText();
                    if (funcName) {
                        const contextForFunction = {
                            ...context,
                            topLevelScopeSymbols,
                            localScopeSymbols: hoistedSymbols,
                        };
                        const lambda = this.generateLambda(
                            stmt,
                            contextForFunction,
                            { isAssignment: true },
                        );
                        code += `${this.indent()}*${funcName} = ${lambda};\n`;
                    }
                } else {
                    code += this.visit(stmt, {
                        ...context,
                        switchBreakLabel,
                        currentLabel: undefined, // Clear currentLabel for nested visits
                        topLevelScopeSymbols,
                        localScopeSymbols: hoistedSymbols,
                        derefBeforeAssignment: true,
                        isAssignmentOnly: ts.isVariableStatement(stmt),
                    });
                }
            }
            this.indentationLevel--;
            code += `${this.indent()}}\n`;
        } else if (ts.isDefaultClause(clause)) {
            // Default clause
            code +=
                `${this.indent()}if (!${fallthroughVar}) ${fallthroughVar} = true;\n`;
            if (firstIf) {
                code += `${this.indent()}if (true) {\n`; // Always execute if no prior cases match and it's the first clause
                firstIf = false;
            } else {
                code += `${this.indent()}if (${fallthroughVar}) {\n`; // Only execute if no prior case (or default) has matched and caused fallthrough
            }
            this.indentationLevel++;
            for (const stmt of clause.statements) {
                code += this.visit(stmt, {
                    ...context,
                    switchBreakLabel,
                    currentLabel: undefined, // Clear currentLabel for nested visits
                    topLevelScopeSymbols,
                    localScopeSymbols: hoistedSymbols,
                    derefBeforeAssignment: true,
                    isAssignmentOnly: ts.isVariableStatement(stmt),
                });
            }
            this.indentationLevel--;
            code += `${this.indent()}}\n`;
        }
    }

    this.indentationLevel--;
    code += `${this.indent()}}\n`; // End of the switch block
    code +=
        `${this.indent()}${switchBreakLabel}:; // break target for switch\n`;

    if (context.currentLabel) {
        this.indentationLevel--;
        code += `${this.indent()}}\n`;
        code +=
            `${this.indent()}${context.currentLabel}_break:; // break target for labeled switch\n`;
    }

    return code;
}

export function visitCaseClause(
    this: CodeGenerator,
    node: ts.CaseClause,
    context: VisitContext,
): string {
    // Case clauses are handled inline by visitSwitchStatement, not generated directly
    // This function will likely not be called, or can return an empty string
    return "";
}

export function visitDefaultClause(
    this: CodeGenerator,
    node: ts.DefaultClause,
    context: VisitContext,
): string {
    // Default clauses are handled inline by visitSwitchStatement, not generated directly
    // This function will likely not be called, or can return an empty string
    return "";
}
