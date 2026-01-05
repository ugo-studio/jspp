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
    this.markSymbolAsInitialized(
        name,
        context.globalScopeSymbols,
        context.localScopeSymbols,
    );

    let nativeLambdaCode = "";
    let initializer = "";
    let shouldSkipDeref = false;

    if (varDecl.initializer) {
        const initExpr = varDecl.initializer;
        let initText = ts.isNumericLiteral(initExpr)
            ? initExpr.getText()
            : this.visit(initExpr, context);
        if (ts.isIdentifier(initExpr)) {
            const initScope = this.getScopeForNode(initExpr);
            const initTypeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                initExpr.text,
                initScope,
            )!;
            const varName = this.getJsVarName(initExpr);

            // Check if both target and initializer are heap allocated
            if (
                typeInfo.needsHeapAllocation &&
                initTypeInfo?.needsHeapAllocation
            ) {
                shouldSkipDeref = true;
            }

            if (
                initTypeInfo &&
                !initTypeInfo.isParameter &&
                !initTypeInfo.isBuiltin &&
                !shouldSkipDeref
            ) {
                initText = this.getDerefCode(
                    initText,
                    varName,
                    context,
                    initTypeInfo,
                );
            }
        } else if (ts.isArrowFunction(initExpr)) {
            const initContext: VisitContext = {
                ...context,
                lambdaName: name, // Use the variable name as function name
            };

            // Generate and update self name
            const nativeName = this.generateUniqueName(
                `__${name}_native_`,
                context.localScopeSymbols,
                context.globalScopeSymbols,
            );
            context.localScopeSymbols.update(name, { func: { nativeName } });

            // Generate lambda
            const lambda = this.generateLambda(
                initExpr,
                initContext,
                {
                    isAssignment: true,
                    generateOnlyLambda: true,
                    nativeName,
                },
            );
            nativeLambdaCode =
                `auto ${nativeName} = ${lambda};\n${this.indent()}`;

            // Generate AnyValue wrapper
            initText = this.generateFullLambdaExpression(
                initExpr,
                initContext,
                nativeName,
                { isAssignment: true, noTypeSignature: true },
            );
        }
        initializer = " = " + initText;
    }

    const isLetOrConst =
        (varDecl.parent.flags & (ts.NodeFlags.Let | ts.NodeFlags.Const)) !== 0;
    const shouldDeref = context.derefBeforeAssignment &&
        (!context.localScopeSymbols.has(name));

    const assignmentTarget = shouldDeref
        ? this.getDerefCode(name, name, context, typeInfo)
        : (typeInfo.needsHeapAllocation && !shouldSkipDeref
            ? `*${name}`
            : name);

    if (isLetOrConst) {
        // If there's no initializer, it should be assigned undefined.
        if (!initializer) {
            return `${nativeLambdaCode}${assignmentTarget} = jspp::Constants::UNDEFINED`;
        }
        return `${nativeLambdaCode}${assignmentTarget}${initializer}`;
    }

    // For 'var', it's a bit more complex.
    if (context.isAssignmentOnly) {
        if (!initializer) return "";
        return `${nativeLambdaCode}${assignmentTarget}${initializer}`;
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
