import ts from "typescript";

import { BUILTIN_OBJECTS, Scope } from "../../analysis/scope";
import type { TypeAnalyzer, TypeInfo } from "../../analysis/typeAnalyzer";
import { DeclaredSymbols, DeclaredSymbolType } from "../../ast/symbols";
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
        } else if (ts.isClassDeclaration(child) && child.name) {
            // Handles class declarations
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
    ...namesToAvoid: (Set<string> | DeclaredSymbols)[]
): string {
    let name = `${prefix}${this.exceptionCounter}`;
    while (namesToAvoid.some((names) => names.has(name))) {
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
    context: VisitContext,
    typeInfo: TypeInfo,
): string {
    // Make sure varName is incased in quotes
    if (!varName.startsWith('"')) varName = '"' + varName;
    if (!varName.endsWith('"')) varName = varName + '"';

    const symbolName = varName.slice(1).slice(0, -1);
    const symbol = context.localScopeSymbols.get(symbolName) ??
        context.topLevelScopeSymbols.get(symbolName);
    const checkedIfUninitialized: boolean = symbol?.checkedIfUninitialized ||
        false;

    // Mark the symbol as checked
    this.markSymbolAsChecked(
        symbolName,
        context.topLevelScopeSymbols,
        context.localScopeSymbols,
    );

    // Apply deref code
    if (checkedIfUninitialized) {
        if (typeInfo && typeInfo.needsHeapAllocation) {
            return `(*${nodeText})`;
        }
        return `${nodeText}`;
    } else {
        if (typeInfo && typeInfo.needsHeapAllocation) {
            return `jspp::Access::deref_ptr(${nodeText}, ${varName})`;
        }
        return `jspp::Access::deref_stack(${nodeText}, ${varName})`;
    }
}

export function markSymbolAsChecked(
    name: string,
    topLevel: DeclaredSymbols,
    local: DeclaredSymbols,
) {
    if (topLevel.has(name)) {
        topLevel.update(name, {
            checkedIfUninitialized: true,
        });
    } else if (local.has(name)) {
        local.update(name, {
            checkedIfUninitialized: true,
        });
    }
}

export function getReturnCommand(
    this: CodeGenerator,
    context: Partial<VisitContext>,
) {
    return (context.isInsideGeneratorFunction || context.isInsideAsyncFunction)
        ? "co_return"
        : "return";
}

export function hoistDeclaration(
    this: CodeGenerator,
    decl: ts.VariableDeclaration | ts.FunctionDeclaration | ts.ClassDeclaration,
    hoistedSymbols: DeclaredSymbols,
) {
    const name = decl.name?.getText();
    if (!name) {
        return `/* Unknown declaration name: ${ts.SyntaxKind[decl.kind]} */`;
    }

    const isLetOrConst =
        (decl.parent.flags & (ts.NodeFlags.Let | ts.NodeFlags.Const)) !==
            0;
    const symbolType = isLetOrConst
        ? DeclaredSymbolType.letOrConst
        : (ts.isFunctionDeclaration(decl)
            ? DeclaredSymbolType.function
            : (ts.isClassDeclaration(decl)
                ? DeclaredSymbolType.letOrConst
                : DeclaredSymbolType.var));

    if (hoistedSymbols.has(name)) {
        const existingType = hoistedSymbols.get(name)?.type;
        // Don't allow multiple declaration of `letOrConst` or `function` or `class` variables
        if (
            existingType === DeclaredSymbolType.letOrConst ||
            existingType === DeclaredSymbolType.function ||
            existingType !== symbolType
        ) {
            throw new SyntaxError(
                `Identifier '${name}' has already been declared`,
            );
        }
        // `var` variables can be declared multiple times
        return "";
    }
    hoistedSymbols.set(name, {
        type: symbolType,
        checkedIfUninitialized: false,
    });

    const scope = this.getScopeForNode(decl);
    const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
        name,
        scope,
    )!;

    const initializer = isLetOrConst || ts.isClassDeclaration(decl)
        ? "jspp::Constants::UNINITIALIZED"
        : "jspp::Constants::UNDEFINED";

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

export function isAsyncFunction(node: ts.Node): boolean {
    return (
        (ts.isFunctionDeclaration(node) ||
            ts.isFunctionExpression(node) ||
            ts.isMethodDeclaration(node) ||
            ts.isArrowFunction(node)) &&
        (ts.getCombinedModifierFlags(node as ts.Declaration) &
                ts.ModifierFlags.Async) !== 0
    );
}

export function prepareScopeSymbolsForVisit(
    topLevel: DeclaredSymbols,
    local: DeclaredSymbols,
): DeclaredSymbols {
    // Join the top and local scopes
    return new DeclaredSymbols(topLevel, local);
}
