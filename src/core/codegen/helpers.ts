import ts from "typescript";

import { BUILTIN_OBJECTS, Scope } from "../../analysis/scope.js";
import type { TypeAnalyzer, TypeInfo } from "../../analysis/typeAnalyzer.js";
import { DeclarationType, DeclaredSymbols } from "../../ast/symbols.js";
import { CodeGenerator } from "./index.js";
import type { VisitContext } from "./visitor.js";

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
    let name = `${prefix}${this.uniqueNameCounter}`;
    while (namesToAvoid.some((names) => names.has(name))) {
        this.uniqueNameCounter++;
        name = `${prefix}${this.uniqueNameCounter}`;
    }
    this.uniqueNameCounter++;
    return name;
}

export function generateUniqueExceptionName(
    this: CodeGenerator,
    exceptionNameToAvoid: string | undefined,
    ...otherNamesToAvoid: (Set<string> | DeclaredSymbols)[]
): string {
    let prefix = `__caught_exception_`;
    if (exceptionNameToAvoid) {
        prefix += exceptionNameToAvoid;
    }
    return this.generateUniqueName(prefix, ...otherNamesToAvoid);
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
        .replace(/\t/g, "\\t")
        .replace(/\?/g, "\\?");
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
        context.globalScopeSymbols.get(symbolName);
    const isInitialized: boolean = symbol?.checked.initialized ||
        false;

    // Mark the symbol as checked
    this.markSymbolAsInitialized(
        symbolName,
        context.globalScopeSymbols,
        context.localScopeSymbols,
    );

    // Apply deref code
    if (isInitialized) {
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

export function markSymbolAsInitialized(
    name: string,
    topLevel: DeclaredSymbols,
    local: DeclaredSymbols,
) {
    if (topLevel.has(name)) {
        topLevel.update(name, {
            checked: { initialized: true },
        });
    } else if (local.has(name)) {
        local.update(name, {
            checked: { initialized: true },
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

    const isLet = (decl.parent.flags & (ts.NodeFlags.Let)) !== 0;
    const isConst = (decl.parent.flags & (ts.NodeFlags.Const)) !== 0;
    const declType = isLet
        ? DeclarationType.let
        : isConst
        ? DeclarationType.const
        : ts.isFunctionDeclaration(decl)
        ? DeclarationType.function
        : ts.isClassDeclaration(decl)
        ? DeclarationType.class
        : DeclarationType.var;

    if (hoistedSymbols.has(name)) {
        const existingSymbol = hoistedSymbols.get(name);
        // Don't allow multiple declaration of `let`,`const`,`function` or `class` variables
        if (
            existingSymbol?.type === DeclarationType.let ||
            existingSymbol?.type === DeclarationType.const ||
            existingSymbol?.type === DeclarationType.function ||
            existingSymbol?.type === DeclarationType.class ||
            existingSymbol?.type !== declType
        ) {
            throw new SyntaxError(
                `Identifier '${name}' has already been declared.\n\n${
                    " ".repeat(6) + decl.getText()
                }\n`,
            );
        }
        // `var` variables can be declared multiple times
        return "";
    } else {
        // Add the symbol to the hoisted symbols
        if (declType === DeclarationType.function) {
            const isAsync = this.isAsyncFunction(decl);
            const isGenerator = this.isGeneratorFunction(decl);
            hoistedSymbols.add(name, {
                type: declType,
                func: { isAsync, isGenerator },
            });
        } else {
            hoistedSymbols.add(name, { type: declType });
        }
    }

    const scope = this.getScopeForNode(decl);
    const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
        name,
        scope,
    )!;

    const initializer = isLet || isConst || ts.isClassDeclaration(decl)
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

export function collectFunctionScopedDeclarations(
    node: ts.Node,
): ts.VariableDeclaration[] {
    const decls: ts.VariableDeclaration[] = [];

    function visit(n: ts.Node) {
        if (ts.isVariableStatement(n)) {
            const isLetOrConst = (n.declarationList.flags &
                (ts.NodeFlags.Let | ts.NodeFlags.Const)) !== 0;
            if (!isLetOrConst) {
                decls.push(...n.declarationList.declarations);
            }
        } else if (ts.isForStatement(n)) {
            if (n.initializer && ts.isVariableDeclarationList(n.initializer)) {
                const isLetOrConst = (n.initializer.flags &
                    (ts.NodeFlags.Let | ts.NodeFlags.Const)) !== 0;
                if (!isLetOrConst) {
                    decls.push(...n.initializer.declarations);
                }
            }
        } else if (ts.isForInStatement(n)) {
            if (ts.isVariableDeclarationList(n.initializer)) {
                const isLetOrConst = (n.initializer.flags &
                    (ts.NodeFlags.Let | ts.NodeFlags.Const)) !== 0;
                if (!isLetOrConst) {
                    decls.push(...n.initializer.declarations);
                }
            }
        } else if (ts.isForOfStatement(n)) {
            if (ts.isVariableDeclarationList(n.initializer)) {
                const isLetOrConst = (n.initializer.flags &
                    (ts.NodeFlags.Let | ts.NodeFlags.Const)) !== 0;
                if (!isLetOrConst) {
                    decls.push(...n.initializer.declarations);
                }
            }
        }

        // Stop recursion at function boundaries (but not the root node if it is a function)
        if (
            n !== node &&
            (ts.isFunctionDeclaration(n) || ts.isFunctionExpression(n) ||
                ts.isArrowFunction(n) || ts.isMethodDeclaration(n) ||
                ts.isGetAccessor(n) || ts.isSetAccessor(n) ||
                ts.isClassDeclaration(n))
        ) {
            return;
        }

        ts.forEachChild(n, visit);
    }

    ts.forEachChild(node, visit);

    return decls;
}

export function collectBlockScopedDeclarations(
    statements: ts.NodeArray<ts.Statement> | ts.Statement[],
): ts.VariableDeclaration[] {
    const decls: ts.VariableDeclaration[] = [];
    for (const stmt of statements) {
        if (ts.isVariableStatement(stmt)) {
            const isLetOrConst = (stmt.declarationList.flags &
                (ts.NodeFlags.Let | ts.NodeFlags.Const)) !== 0;
            if (isLetOrConst) {
                decls.push(...stmt.declarationList.declarations);
            }
        }
    }
    return decls;
}
