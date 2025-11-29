import ts from "typescript";

import { CodeGenerator } from "./";
import type { VisitContext } from "./visitor";

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

    let initializer = "";
    if (varDecl.initializer) {
        const initExpr = varDecl.initializer;
        let initText = this.visit(initExpr, context);
        if (ts.isIdentifier(initExpr)) {
            const initScope = this.getScopeForNode(initExpr);
            const initTypeInfo =
                this.typeAnalyzer.scopeManager.lookupFromScope(
                    initExpr.text,
                    initScope,
                )!;
            const varName = this.getJsVarName(initExpr);
            if (
                initTypeInfo &&
                !initTypeInfo.isParameter &&
                !initTypeInfo.isBuiltin
            ) {
                initText = this.getDerefCode(initText, varName, initTypeInfo);
            }
        }
        initializer = " = " + initText;
    }

    const isLetOrConst =
        (varDecl.parent.flags & (ts.NodeFlags.Let | ts.NodeFlags.Const)) !== 0;

    const assignmentTarget = typeInfo.needsHeapAllocation ? `*${name}` : name;

    if (isLetOrConst) {
        // If there's no initializer, it should be assigned undefined.
        if (!initializer)
            return `${assignmentTarget} = jspp::AnyValue::make_undefined()`;
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
            : "jspp::AnyValue::make_undefined()";
        if (typeInfo.needsHeapAllocation) {
            return `auto ${name} = std::make_shared<jspp::AnyValue>(${initValue})`;
        } else {
            return `jspp::AnyValue ${name} = ${initValue}`;
        }
    }
}
