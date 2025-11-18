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

    let initializer = "";
    if (varDecl.initializer) {
        const initExpr = varDecl.initializer;
        let initText = this.visit(initExpr, context);
        if (ts.isIdentifier(initExpr)) {
            const scope = this.getScopeForNode(initExpr);
            const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                initExpr.text,
                scope,
            );
            const varName = this.getJsVarName(initExpr);
            if (
                typeInfo && !typeInfo.isParameter && !typeInfo.isBuiltin
            ) {
                initText = `jspp::Access::deref(${initText}, ${varName})`;
            }
        }
        initializer = " = " + initText;
    }

    const isLetOrConst =
        (varDecl.parent.flags & (ts.NodeFlags.Let | ts.NodeFlags.Const)) !== 0;

    if (isLetOrConst) {
        // If there's no initializer, it should be assigned undefined.
        if (!initializer) return `*${name} = jspp::JsValue::make_undefined()`;
        return `*${name}${initializer}`;
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
            : "jspp::JsValue::make_undefined()";
        return `auto ${name} = std::make_shared<jspp::JsValue>(${initValue})`;
    }
}
