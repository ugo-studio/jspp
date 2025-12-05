import ts from "typescript";

import { BUILTIN_OBJECTS, Scope } from "../../analysis/scope";
import type { TypeAnalyzer, TypeInfo } from "../../analysis/typeAnalyzer";
import { CodeGenerator } from "./";
import type { VisitContext } from "./visitor";

export function isBuiltinObject(
    this: CodeGenerator | TypeAnalyzer,
    node: ts.Identifier,
): boolean {
    return BUILTIN_OBJECTS.values().some((obj) => obj.name === node.text);
}

export function getDeclaredSymbols(
    this: CodeGenerator,
    node: ts.Node,
): Set<string> {
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

export function generateUniqueName(
    this: CodeGenerator,
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

export function generateUniqueExceptionName(
    this: CodeGenerator,
    nameToAvoid?: string,
): string {
    let exceptionName = `__caught_exception_${this.exceptionCounter}`;
    while (exceptionName === nameToAvoid) {
        this.exceptionCounter++;
        exceptionName = `__caught_exception_${this.exceptionCounter}`;
    }
    this.exceptionCounter++;
    return exceptionName;
}

export function getScopeForNode(this: CodeGenerator, node: ts.Node): Scope {
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

export function indent(this: CodeGenerator) {
    return "  ".repeat(this.indentationLevel);
}

export function escapeString(this: CodeGenerator, str: string): string {
    return str
        .replace(/\\/g, "\\\\")
        .replace(/"/g, '\\"')
        .replace(/\n/g, "\\n")
        .replace(/\r/g, "\\r")
        .replace(/\t/g, "\\t");
}

export function getJsVarName(this: CodeGenerator, node: ts.Identifier): string {
    return `"${node.text}"`;
}

export function getDerefCode(
    this: CodeGenerator,
    nodeText: string,
    varName: string,
    typeInfo: TypeInfo,
): string {
    if (typeInfo && typeInfo.needsHeapAllocation) {
        return `jspp::Access::deref_ptr(${nodeText}, ${varName})`;
    }
    return `jspp::Access::deref_stack(${nodeText}, ${varName})`;
}

export function getReturnCommand(
    this: CodeGenerator,
    context: Partial<VisitContext>,
) {
    return context.isInsideGeneratorFunction ? "co_return" : "return";
}

export function hoistVariableDeclaration(
    this: CodeGenerator,
    decl: ts.VariableDeclaration,
    hoistedSymbols: Set<string>,
) {
    const name = decl.name.getText();
    if (hoistedSymbols.has(name)) {
        throw new SyntaxError(
            `Identifier '${name}' has already been declared`,
        );
    }
    hoistedSymbols.add(name);

    const scope = this.getScopeForNode(decl);
    const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
        name,
        scope,
    )!;

    const isLetOrConst =
        (decl.parent.flags & (ts.NodeFlags.Let | ts.NodeFlags.Const)) !==
            0;
    const initializer = isLetOrConst
        ? "jspp::AnyValue::make_uninitialized()"
        : "jspp::AnyValue::make_undefined()";

    if (typeInfo.needsHeapAllocation) {
        return `${this.indent()}auto ${name} = std::make_shared<jspp::AnyValue>(${initializer});\n`;
    } else {
        return `${this.indent()}jspp::AnyValue ${name} = ${initializer};\n`;
    }
}

export function isGeneratorFunction(node: ts.Node): boolean {
    return (
        (ts.isFunctionDeclaration(node) ||
            ts.isFunctionExpression(node) ||
            ts.isMethodDeclaration(node)) &&
        !!node.asteriskToken // generator indicator
    );
}
