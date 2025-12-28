import * as ts from "typescript";
import type { TypeInfo } from "./typeAnalyzer";

export const RESERVED_KEYWORDS = new Set([
    "jspp",
    "std",
    "co_yield",
    "co_return",
    "co_await",
]);
export const BUILTIN_OBJECTS = new Set([
    { name: "undefined", isConst: true },
    { name: "null", isConst: true },
    { name: "Symbol", isConst: false },
    { name: "console", isConst: false },
    { name: "performance", isConst: false },
    { name: "global", isConst: false },
    { name: "globalThis", isConst: false },
    { name: "Error", isConst: false },
    { name: "Promise", isConst: false },
    { name: "setTimeout", isConst: false },
    { name: "clearTimeout", isConst: false },
    { name: "setInterval", isConst: false },
    { name: "clearInterval", isConst: false },
]);

// Represents a single scope (e.g., a function body or a block statement)
export class Scope {
    public readonly symbols = new Map<string, TypeInfo>();
    public readonly children: Scope[] = [];
    constructor(
        public readonly parent: Scope | null,
        public readonly ownerFunction: ts.Node | null,
    ) {}

    // Defines a variable in this scope.
    define(name: string, type: TypeInfo): boolean {
        if (this.symbols.has(name)) {
            return false; // Already defined in this scope
        }
        this.symbols.set(name, type);
        return true;
    }

    // Finds the scope where a variable is defined, walking up the chain.
    findScopeFor(name: string): Scope | null {
        if (this.symbols.has(name)) {
            return this;
        }
        return this.parent ? this.parent.findScopeFor(name) : null;
    }
}

// Manages the hierarchy of scopes during analysis.
export class ScopeManager {
    public currentScope: Scope;
    private readonly allScopes: Scope[] = []; // Array to store all created scopes
    private readonly reservedKeywords = new Set(RESERVED_KEYWORDS);

    constructor() {
        const rootScope = new Scope(null, null); // The global scope
        this.currentScope = rootScope;
        this.allScopes.push(rootScope); // Add the root scope to our list

        for (const { name, isConst } of BUILTIN_OBJECTS) {
            this.define(name, {
                type: name,
                isConst: isConst,
                isBuiltin: true,
            });
        }
    }

    // Enters a new, nested scope.
    enterScope(ownerFunction: ts.Node | null) {
        const newScope = new Scope(this.currentScope, ownerFunction);
        this.currentScope.children.push(newScope);
        this.currentScope = newScope;
        this.allScopes.push(newScope); // Add every new scope to the list
    }

    // Exits the current scope, returning to the parent.
    exitScope() {
        if (this.currentScope.parent) {
            this.currentScope = this.currentScope.parent;
        }
    }

    // Defines a variable in the current scope.
    define(name: string, type: TypeInfo) {
        if (this.reservedKeywords.has(name) && !type.isBuiltin) {
            throw new Error(`SyntaxError: Unexpected reserved word "${name}"`);
        }
        this.currentScope.define(name, type);
    }

    // Looks up a variable's type information from the current scope upwards.
    lookup(name: string): TypeInfo | null {
        const scope = this.currentScope.findScopeFor(name);
        return scope ? scope.symbols.get(name) ?? null : null;
    }

    lookupFromScope(name: string, scope: Scope): TypeInfo | null {
        const definingScope = scope.findScopeFor(name);
        return definingScope ? definingScope.symbols.get(name) ?? null : null;
    }

    // The missing method to retrieve all scopes for the generator.
    public getAllScopes(): Scope[] {
        return this.allScopes;
    }
}
