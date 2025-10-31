import ts from "typescript";
import { Scope } from "../../analysis/scope";
import { CodeGenerator } from ".";

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
        .replace(/"/g, '\"')
        .replace(/\n/g, "\\n")
        .replace(/\r/g, "\\r")
        .replace(/\t/g, "\\t");
}

export function getJsVarName(this: CodeGenerator, node: ts.Identifier): string {
    return `"${node.text}"`;
}
