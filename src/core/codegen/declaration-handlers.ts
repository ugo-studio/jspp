import ts from "typescript";

import { CodeGenerator } from "./index.js";
import type { VisitContext } from "./visitor.js";

export function visitVariableDeclarationList(
    this: CodeGenerator,
    node: ts.VariableDeclarationList,
    context: VisitContext,
): string {
    return node.declarations
        .map((d) => this.visit(d, context))
        .filter(Boolean)
        .join(", ");
}

export function visitVariableDeclaration(
    this: CodeGenerator,
    node: ts.VariableDeclaration,
    context: VisitContext,
): string {
    const varDecl = node as ts.VariableDeclaration;
    const name = varDecl.name.getText();
    const scope = this.getScopeForNode(varDecl);
    const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
        name,
        scope,
    )!;

    // Mark the symbol as checked
    this.markSymbolAsChecked(
        name,
        context.topLevelScopeSymbols,
        context.localScopeSymbols,
    );

    let initializer = "";
    if (varDecl.initializer) {
        const initExpr = varDecl.initializer;
        const initContext: VisitContext = {
            ...context,
            lambdaName: ts.isArrowFunction(initExpr) ? name : undefined, // Pass the variable name for arrow functions
        };
        let initText = ts.isNumericLiteral(initExpr)
            ? initExpr.getText()
            : this.visit(initExpr, initContext);
        if (ts.isIdentifier(initExpr)) {
            const initScope = this.getScopeForNode(initExpr);
            const initTypeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                initExpr.text,
                initScope,
            )!;
            const varName = this.getJsVarName(initExpr);
            if (
                initTypeInfo &&
                !initTypeInfo.isParameter &&
                !initTypeInfo.isBuiltin
            ) {
                initText = this.getDerefCode(
                    initText,
                    varName,
                    initContext,
                    initTypeInfo,
                );
            }
        }
        initializer = " = " + initText;
    }

    const isLetOrConst =
        (varDecl.parent.flags & (ts.NodeFlags.Let | ts.NodeFlags.Const)) !== 0;
    const shouldDeref = context.derefBeforeAssignment &&
        (!context.localScopeSymbols.has(name));

    const assignmentTarget = shouldDeref
        ? this.getDerefCode(name, name, context, typeInfo)
        : (typeInfo.needsHeapAllocation ? `*${name}` : name);

    if (isLetOrConst) {
        // If there's no initializer, it should be assigned undefined.
        if (!initializer) {
            return `${assignmentTarget} = jspp::Constants::UNDEFINED`;
        }
        return `${assignmentTarget}${initializer}`;
    }

    // For 'var', it's a bit more complex.
    if (context.isAssignmentOnly) {
        if (!initializer) return "";
        return `${assignmentTarget}${initializer}`;
    } else {
        // This case should not be hit with the new hoisting logic,
        // but is kept for safety.
        const initValue = initializer
            ? initializer.substring(3)
            : "jspp::Constants::UNDEFINED";
        if (typeInfo.needsHeapAllocation) {
            return `auto ${name} = std::make_shared<jspp::AnyValue>(${initValue})`;
        } else {
            return `jspp::AnyValue ${name} = ${initValue}`;
        }
    }
}
