import ts from "typescript";

import { CodeGenerator } from "./";
import type { VisitContext } from "./visitor";

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
                        : "jspp::AnyValue::make_undefined()";

                    const scope = this.getScopeForNode(decl);
                    const typeInfo = this.typeAnalyzer.scopeManager
                        .lookupFromScope(
                            name,
                            scope,
                        )!;

                    if (typeInfo.needsHeapAllocation) {
                        initializerCode =
                            `auto ${name} = std::make_shared<jspp::AnyValue>(${initValue})`;
                    } else {
                        initializerCode =
                            `jspp::AnyValue ${name} = ${initValue}`;
                    }
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
        code += `(${this.visit(forStmt.condition, context)}).is_truthy()`;
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

    let code = `${this.indent()}{\n`;
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
                    `${this.indent()}auto ${varName} = std::make_shared<jspp::AnyValue>(jspp::AnyValue::make_undefined());\n`;
                assignmentTarget = `*${varName}`;
            } else {
                code +=
                    `${this.indent()}jspp::AnyValue ${varName} = jspp::AnyValue::make_undefined();\n`;
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
        isFunctionBody: false,
    });
    this.indentationLevel--;
    code += `${this.indent()}}}\n`;
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
    let elemName = "";
    let assignmentTarget = "";

    if (ts.isVariableDeclarationList(forOf.initializer)) {
        const decl = forOf.initializer.declarations[0];
        if (decl) {
            elemName = decl.name.getText();
            code += `${this.indent()}{\n`;
            const scope = this.getScopeForNode(decl);
            const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                elemName,
                scope,
            )!;
            if (typeInfo.needsHeapAllocation) {
                code +=
                    `${this.indent()}auto ${elemName} = std::make_shared<jspp::AnyValue>(jspp::AnyValue::make_undefined());\n`;
                assignmentTarget = `*${elemName}`;
            } else {
                code +=
                    `${this.indent()}jspp::AnyValue ${elemName} = jspp::AnyValue::make_undefined();\n`;
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
        code += `${this.indent()}{\n`;
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
        derefIterable = this.getDerefCode(iterableExpr, varName, typeInfo);
    }

    const declaredSymbols = this.getDeclaredSymbols(forOf.statement);
    const iteratorPtr = this.generateUniqueName("__iter_ptr_", declaredSymbols);
    const nextRes = this.generateUniqueName("__res__", declaredSymbols);

    const varName = this.getJsVarName(forOf.expression as ts.Identifier);
    code +=
        `${this.indent()}auto ${iteratorPtr} = jspp::Access::get_object_value_iterator(${derefIterable}, ${varName}).as_iterator();\n`;
    code += `${this.indent()}auto ${nextRes} = ${iteratorPtr}->next();\n`;
    code += `${this.indent()}while (!${nextRes}.done) {\n`;
    this.indentationLevel++;
    code +=
        `${this.indent()}${assignmentTarget} = ${nextRes}.value.value_or(jspp::AnyValue::make_undefined());\n`;
    code += this.visit(forOf.statement, {
        ...context,
        isFunctionBody: false,
    });
    code += `${this.indent()}${nextRes} = ${iteratorPtr}->next();\n`;
    this.indentationLevel--;
    code += `${this.indent()}}}\n`;
    this.indentationLevel--; // Exit the scope for the for-of loop

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
        : `(${this.visit(condition, context)}).is_truthy()`;

    let code = "";

    code += `${this.indent()}while (${conditionText}) `;

    const statementCode = this.visit(node.statement, {
        ...context,
        isFunctionBody: false,
    });

    if (ts.isBlock(node.statement)) {
        code += statementCode;
    } else {
        code += `{\n`;
        this.indentationLevel++;
        code += statementCode;
        this.indentationLevel--;
        code += `${this.indent()}}\n`;
    }

    return code;
}

export function visitDoStatement(
    this: CodeGenerator,
    node: ts.DoStatement,
    context: VisitContext,
): string {
    const condition = node.expression;
    const conditionText = `(${this.visit(condition, context)}).is_truthy()`;

    let code = `${this.indent()}do `;

    const statementCode = this.visit(node.statement, {
        ...context,
        isFunctionBody: false,
    });

    if (ts.isBlock(node.statement)) {
        code += statementCode.trimEnd();
    } else {
        code += `{\n`;
        this.indentationLevel++;
        code += statementCode;
        this.indentationLevel--;
        code += `${this.indent()}}`;
    }

    code += ` while (${conditionText});\n`;

    return code;
}
