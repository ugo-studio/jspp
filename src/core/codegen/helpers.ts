import ts from "typescript";

import { BUILTIN_OBJECTS, Scope } from "../../analysis/scope.js";
import type { TypeAnalyzer, TypeInfo } from "../../analysis/typeAnalyzer.js";
import { DeclarationType, DeclaredSymbols } from "../../ast/symbols.js";
import { CompilerError } from "../error.js";
import { CodeGenerator } from "./index.js";
import type { VisitContext } from "./visitor.js";

/**
 * Checks if an identifier refers to a built-in JavaScript object (e.g., console, Math).
 *
 * @param node The identifier node to check.
 * @returns True if it's a built-in object.
 */
export function isBuiltinObject(
    this: CodeGenerator | TypeAnalyzer,
    node: ts.Identifier,
): boolean {
    return BUILTIN_OBJECTS.values().some((obj) => obj.name === node.text);
}

/**
 * Collects all symbols declared within a node's subtree.
 *
 * @param node The node to scan for declarations.
 * @returns A set of declared symbol names.
 */
export function getDeclaredSymbols(
    this: CodeGenerator,
    node: ts.Node,
): Set<string> {
    const symbols = new Set<string>();

    const collectNames = (name: ts.BindingName) => {
        if (ts.isIdentifier(name)) {
            symbols.add(name.text);
        } else {
            name.elements.forEach((element) => {
                if (ts.isBindingElement(element)) {
                    collectNames(element.name);
                }
            });
        }
    };

    const visitor = (child: ts.Node) => {
        if (ts.isVariableDeclaration(child)) {
            // Handles let, const, var
            collectNames(child.name);
        } else if (ts.isFunctionDeclaration(child) && child.name) {
            // Handles function declarations
            symbols.add(child.name.getText());
        } else if (ts.isClassDeclaration(child) && child.name) {
            // Handles class declarations
            symbols.add(child.name.getText());
        } else if (ts.isEnumDeclaration(child) && child.name) {
            // Handles enum declarations
            symbols.add(child.name.getText());
        } else if (ts.isParameter(child)) {
            // Handles function parameters
            collectNames(child.name);
        } else if (ts.isCatchClause(child) && child.variableDeclaration) {
            // Handles catch clause variable
            collectNames(child.variableDeclaration.name);
        }
        ts.forEachChild(child, visitor);
    };
    visitor(node);
    return symbols;
}

/**
 * Generates a unique name by appending a counter to a prefix, avoiding existing symbols.
 *
 * @param prefix The base name for the unique identifier.
 * @param namesToAvoid Sets or maps of names that should not be used.
 * @returns A unique identifier string.
 */
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

/**
 * Generates a unique name for a caught exception variable.
 *
 * @param exceptionNameToAvoid The name of the exception variable to potentially avoid shadowing.
 * @param otherNamesToAvoid Additional names to avoid.
 * @returns A unique identifier string for the exception.
 */
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

/**
 * Retrieves the scope associated with a given node.
 *
 * Walks up the parent chain until a node with an associated scope is found.
 *
 * @param node The node to find the scope for.
 * @returns The associated Scope.
 * @throws CompilerError if no scope is found.
 */
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
        throw new CompilerError(
            "Could not find a root scope.",
            node,
            "CompilerBug",
        );
    }
    return rootScope;
}

/**
 * Returns a string of spaces representing the current indentation level.
 *
 * @returns An indentation string.
 */
export function indent(this: CodeGenerator) {
    return "  ".repeat(this.indentationLevel);
}

/**
 * Escapes special characters in a string for safe use in C++ string literals.
 *
 * @param str The string to escape.
 * @returns The escaped string.
 */
export function escapeString(this: CodeGenerator, str: string): string {
    return str
        .replace(/\\/g, "\\\\")
        .replace(/"/g, '\\"')
        .replace(/\n/g, "\\n")
        .replace(/\r/g, "\\r")
        .replace(/\t/g, "\\t")
        .replace(/\?/g, "\\?");
}

/**
 * Formats a JavaScript variable name as a C++ string literal for error reporting or property access.
 *
 * @param node The identifier node.
 * @returns The variable name wrapped in quotes.
 */
export function getJsVarName(this: CodeGenerator, node: ts.Identifier): string {
    return `"${node.text}"`;
}

/**
 * Generates C++ code to dereference a variable, handling TDZ and heap allocation.
 *
 * If the symbol is not yet initialized, it generates a call to a deref helper
 * that performs a runtime check.
 *
 * @param nodeText The C++ expression for the variable.
 * @param varName The JavaScript name of the variable.
 * @param context The current visit context.
 * @param typeInfo Type information for the variable.
 * @returns The C++ code for accessing the variable's value.
 */
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
    const isInitialized: boolean = symbol?.checks.initialized ||
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

/**
 * Marks a symbol as initialized in the provided symbol tables.
 *
 * @param name The name of the symbol.
 * @param topLevel The global symbol table.
 * @param local The local symbol table.
 */
export function markSymbolAsInitialized(
    name: string,
    topLevel: DeclaredSymbols,
    local: DeclaredSymbols,
) {
    if (topLevel.has(name)) {
        topLevel.update(name, {
            checks: { initialized: true },
        });
    } else if (local.has(name)) {
        local.update(name, {
            checks: { initialized: true },
        });
    }
}

/**
 * Determines the appropriate C++ return command (return or co_return) based on context.
 *
 * @param context Partial visit context.
 * @returns "co_return" for generators/async, "return" otherwise.
 */
export function getReturnCommand(
    this: CodeGenerator,
    context: Partial<VisitContext>,
) {
    return (context.isInsideGeneratorFunction || context.isInsideAsyncFunction)
        ? "co_return"
        : "return";
}

/**
 * Generates C++ code to hoist a declaration (var, let, const, function, class, enum).
 *
 * It registers the symbol and generates the variable declaration, initializing
 * block-scoped variables to UNINITIALIZED for TDZ support.
 *
 * @param decl The declaration node.
 * @param hoistedSymbols The symbol table to register with.
 * @param scopeNode The node representing the current scope.
 * @returns The C++ declaration code.
 * @throws CompilerError if a duplicate declaration is found.
 */
export function hoistDeclaration(
    this: CodeGenerator,
    decl:
        | ts.VariableDeclaration
        | ts.FunctionDeclaration
        | ts.ClassDeclaration
        | ts.EnumDeclaration,
    hoistedSymbols: DeclaredSymbols,
    scopeNode: ts.Node,
) {
    const isLet = ts.isVariableDeclaration(decl) &&
        (decl.parent.flags & (ts.NodeFlags.Let)) !== 0;
    const isConst = ts.isVariableDeclaration(decl) &&
        (decl.parent.flags & (ts.NodeFlags.Const)) !== 0;
    const declType = isLet
        ? DeclarationType.let
        : isConst
        ? DeclarationType.const
        : ts.isFunctionDeclaration(decl)
        ? DeclarationType.function
        : ts.isClassDeclaration(decl)
        ? DeclarationType.class
        : ts.isEnumDeclaration(decl)
        ? DeclarationType.enum
        : DeclarationType.var;

    const hoistName = (nameNode: ts.BindingName): string => {
        if (ts.isIdentifier(nameNode)) {
            const name = nameNode.text;
            if (hoistedSymbols.has(name)) {
                const existingSymbol = hoistedSymbols.get(name);
                // Don't allow multiple declaration of `let`,`const`,`function`, `class` or `enum` variables
                if (
                    existingSymbol?.type === DeclarationType.let ||
                    existingSymbol?.type === DeclarationType.const ||
                    existingSymbol?.type === DeclarationType.function ||
                    existingSymbol?.type === DeclarationType.class ||
                    existingSymbol?.type === DeclarationType.enum ||
                    existingSymbol?.type !== declType
                ) {
                    throw new CompilerError(
                        `Identifier '${name}' has already been declared.`,
                        decl,
                        "SyntaxError",
                    );
                }
                // `var` variables can be declared multiple times
                return "";
            } else {
                // Add the symbol to the hoisted symbols
                if (
                    ts.isFunctionDeclaration(decl) ||
                    (ts.isVariableDeclaration(decl) && decl.initializer &&
                        nameNode === decl.name &&
                        (ts.isArrowFunction(decl.initializer) ||
                            ts.isFunctionExpression(decl.initializer)))
                ) {
                    const funcExpr = ts.isVariableDeclaration(decl)
                        ? decl.initializer as
                            | ts.ArrowFunction
                            | ts.FunctionExpression
                        : decl;
                    const isAsync = this.isAsyncFunction(funcExpr);
                    const isGenerator = this.isGeneratorFunction(funcExpr);
                    hoistedSymbols.add(name, {
                        type: declType,
                        features: { isAsync, isGenerator },
                    });
                    // Don't hoist declarations not used as a variable
                    // They will be called with their native lambda/value
                    if (
                        !this.isDeclarationUsedAsValue(
                            decl as
                                | ts.FunctionDeclaration
                                | ts.VariableDeclaration,
                            scopeNode,
                        ) &&
                        !this.isDeclarationUsedBeforeInitialization(
                            name,
                            scopeNode,
                        )
                    ) {
                        return "";
                    }
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
        } else {
            let code = "";
            nameNode.elements.forEach((element) => {
                if (ts.isBindingElement(element)) {
                    code += hoistName(element.name);
                }
            });
            return code;
        }
    };

    if (ts.isVariableDeclaration(decl)) {
        return hoistName(decl.name);
    } else {
        const nameNode = decl.name;
        if (!nameNode) {
            return `/* Unknown declaration name: ${
                ts.SyntaxKind[decl.kind]
            } */`;
        }
        return hoistName(nameNode);
    }
}

/**
 * Checks if a function node is a generator function.
 *
 * @param node The node to check.
 * @returns True if it's a generator.
 */
export function isGeneratorFunction(node: ts.Node): boolean {
    return (
        (ts.isFunctionDeclaration(node) ||
            ts.isFunctionExpression(node) ||
            ts.isMethodDeclaration(node)) &&
        !!node.asteriskToken // generator indicator
    );
}

/**
 * Checks if a function node is an async function.
 *
 * @param node The node to check.
 * @returns True if it's async.
 */
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

/**
 * Combines top-level and local symbol tables for a nested visit.
 *
 * @param topLevel Global/outer symbol table.
 * @param local Local/inner symbol table.
 * @returns A new DeclaredSymbols instance representing the merged scope.
 */
export function prepareScopeSymbolsForVisit(
    topLevel: DeclaredSymbols,
    local: DeclaredSymbols,
): DeclaredSymbols {
    // Join the top and local scopes
    return new DeclaredSymbols(topLevel, local);
}

/**
 * Determines if a statement should be ignored (e.g., ambient declarations).
 *
 * @param stmt The statement to check.
 * @returns True if the statement should be ignored.
 */
export function shouldIgnoreStatement(
    stmt: ts.Statement,
): boolean {
    // Ignore variable statements with 'declare' modifier
    if (ts.isVariableStatement(stmt)) {
        if (
            stmt.modifiers &&
            stmt.modifiers.some((m) => m.kind === ts.SyntaxKind.DeclareKeyword)
        ) {
            return true;
        }
    }

    return false;
}

/**
 * Collects all function-scoped (var) declarations within a node,
 * respecting function boundaries.
 *
 * @param node The root node to scan.
 * @returns An array of variable declarations.
 */
export function collectFunctionScopedDeclarations(
    node: ts.Node,
): ts.VariableDeclaration[] {
    const decls: ts.VariableDeclaration[] = [];

    function visit(n: ts.Node) {
        if (ts.isVariableStatement(n)) {
            // Ignore Declare modifier
            if (shouldIgnoreStatement(n)) return;
            // Only collect let/const declarations
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

/**
 * Collects block-scoped (let/const) declarations directly within a list of statements.
 *
 * @param statements The statements to scan.
 * @returns An array of variable declarations.
 */
export function collectBlockScopedDeclarations(
    statements: ts.NodeArray<ts.Statement> | ts.Statement[],
): ts.VariableDeclaration[] {
    const decls: ts.VariableDeclaration[] = [];
    for (const stmt of statements) {
        if (ts.isVariableStatement(stmt)) {
            // Ignore Declare modifier
            if (shouldIgnoreStatement(stmt)) continue;
            // Only collect let/const declarations
            const isLetOrConst = (stmt.declarationList.flags &
                (ts.NodeFlags.Let | ts.NodeFlags.Const)) !== 0;
            if (isLetOrConst) {
                decls.push(...stmt.declarationList.declarations);
            }
        }
    }
    return decls;
}

/**
 * Checks if a declaration is used as a value within a given node.
 *
 * This is used to determine if a hoisted function declaration needs to be
 * wrapped in an AnyValue and assigned to a variable, or if it can be
 * optimized to only use its native lambda.
 *
 * @param decl The declaration to check.
 * @param root The root node to search for usages within.
 * @returns True if the declaration is used as a value.
 */
export function isDeclarationUsedAsValue(
    this: CodeGenerator,
    decl: ts.FunctionDeclaration | ts.ClassDeclaration | ts.VariableDeclaration,
    root: ts.Node,
): boolean {
    const nameNode = decl.name;
    if (!nameNode || !ts.isIdentifier(nameNode)) return false;
    const name = nameNode.text;

    let isUsed = false;

    const visitor = (node: ts.Node) => {
        if (isUsed) return;

        if (ts.isIdentifier(node) && node.text === name) {
            const scope = this.getScopeForNode(node);
            const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                node.text,
                scope,
            );

            if (typeInfo?.declaration === decl) {
                const parent = node.parent;

                // Ignore declarations where the identifier is the name being declared
                if (
                    (ts.isFunctionDeclaration(parent) ||
                        ts.isVariableDeclaration(parent) ||
                        ts.isClassDeclaration(parent) ||
                        ts.isMethodDeclaration(parent) ||
                        ts.isParameter(parent) ||
                        ts.isImportSpecifier(parent)) &&
                    parent.name === node
                ) {
                    // Declaration, do nothing
                } // Ignore property names (e.g. obj.funcName)
                else if (
                    (ts.isPropertyAccessExpression(parent) &&
                        parent.name === node) ||
                    (ts.isPropertyAssignment(parent) && parent.name === node)
                ) {
                    // Property name, do nothing
                } // Ignore direct calls (e.g. funcName())
                else if (
                    ts.isCallExpression(parent) && parent.expression === node
                ) {
                    // Call, do nothing
                } else {
                    // Used as a value
                    isUsed = true;
                }
            }
        }

        if (!isUsed) {
            ts.forEachChild(node, visitor);
        }
    };

    ts.forEachChild(root, visitor);

    return isUsed;
}

/**
 * Checks if a declaration is called as a function (e.g. decl()) within a given node.
 *
 * @param decl The declaration to check.
 * @param root The root node to search for usages within.
 * @returns True if the declaration is called as a function.
 */
export function isDeclarationCalledAsFunction(
    this: CodeGenerator,
    decl: ts.FunctionDeclaration | ts.ClassDeclaration | ts.VariableDeclaration,
    root: ts.Node,
): boolean {
    const nameNode = decl.name;
    if (!nameNode || !ts.isIdentifier(nameNode)) return false;
    const name = nameNode.text;

    let isCalled = false;

    const visitor = (node: ts.Node) => {
        if (isCalled) return;

        if (ts.isIdentifier(node) && node.text === name) {
            const scope = this.getScopeForNode(node);
            const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                node.text,
                scope,
            );

            if (typeInfo?.declaration === decl) {
                const parent = node.parent;

                if (
                    ts.isCallExpression(parent) && parent.expression === node
                ) {
                    isCalled = true;
                }
            }
        }

        if (!isCalled) {
            ts.forEachChild(node, visitor);
        }
    };

    ts.forEachChild(root, visitor);

    return isCalled;
}

/**
 * Checks if a declaration is used before it is initialized within a given node.
 *
 * This helps determine if a hoisted declaration (like a function or class)
 * might be accessed before its assignment logic is executed, necessitating
 * an AnyValue wrapper for TDZ or forward-reference support.
 *
 * @param name The name of the declaration to check.
 * @param root The root node to search for usages within.
 * @returns True if the declaration is used before initialization.
 */
export function isDeclarationUsedBeforeInitialization(
    this: CodeGenerator,
    name: string,
    root: ts.Node,
): boolean {
    let declPos = -1;
    let foundDecl = false;

    // Helper to find the declaration position
    function findDecl(node: ts.Node) {
        if (foundDecl) return;
        if (
            (ts.isFunctionDeclaration(node) ||
                ts.isClassDeclaration(node) ||
                ts.isVariableDeclaration(node) ||
                ts.isEnumDeclaration(node)) &&
            node.name && ts.isIdentifier(node.name) && node.name.text === name
        ) {
            declPos = node.getStart();
            foundDecl = true;
        } else {
            ts.forEachChild(node, findDecl);
        }
    }
    findDecl(root);

    // If not declared in this scope (or at least not found), assume it's not a local forward-ref issue
    if (!foundDecl) return false;

    let isUsedBefore = false;

    function visit(node: ts.Node) {
        if (isUsedBefore) return;

        if (ts.isIdentifier(node) && node.text === name) {
            const parent = node.parent;

            // Ignore declarations where the identifier is the name being declared
            if (
                (ts.isFunctionDeclaration(parent) ||
                    ts.isVariableDeclaration(parent) ||
                    ts.isClassDeclaration(parent) ||
                    ts.isMethodDeclaration(parent) ||
                    ts.isParameter(parent) ||
                    ts.isImportSpecifier(parent)) &&
                parent.name === node
            ) {
                // Declaration (or shadowing), do nothing
            } // Ignore property names (e.g. obj.funcName)
            else if (
                (ts.isPropertyAccessExpression(parent) &&
                    parent.name === node) ||
                (ts.isPropertyAssignment(parent) && parent.name === node)
            ) {
                // Property name, do nothing
            } else {
                // It is a usage (call or value usage)
                const usagePos = node.getStart();

                // Check 1: Lexically before
                if (usagePos < declPos) {
                    isUsedBefore = true;
                    return;
                }

                // Check 2: Inside a function declared before
                let current = parent;
                while (current && current !== root) {
                    if (
                        ts.isFunctionDeclaration(current) ||
                        ts.isMethodDeclaration(current) ||
                        ts.isArrowFunction(current) ||
                        ts.isFunctionExpression(current) ||
                        ts.isGetAccessor(current) ||
                        ts.isSetAccessor(current) ||
                        ts.isConstructorDeclaration(current)
                    ) {
                        if (current.getStart() < declPos) {
                            isUsedBefore = true;
                            return;
                        }
                        // Once we hit a function boundary, we stop bubbling up for this usage.
                        break;
                    }
                    current = current.parent;
                }
            }
        }

        ts.forEachChild(node, visit);
    }

    visit(root);
    return isUsedBefore;
}

/**
 * Validates and filters function parameters, checking for illegal "this"
 * and correctly positioned rest parameters.
 *
 * @param parameters The parameters to validate.
 * @returns A filtered array of valid parameters.
 * @throws CompilerError for invalid parameter usage.
 */
export function validateFunctionParams(
    this: CodeGenerator,
    parameters: ts.NodeArray<ts.ParameterDeclaration>,
) {
    return parameters.filter((p) => {
        if (p.name.getText() === "this") {
            if (!this.isTypescript) {
                // Throw error for "this" parameters in javascript files
                throw new CompilerError(
                    'Cannot use "this" as a parameter name.',
                    p,
                    "SyntaxError",
                );
            } else return false; // Ignore "this" parameters in typescript files
        } else return true;
    }).map((p, i, parameters) => {
        if (!!p.dotDotDotToken) {
            if (parameters.length - 1 !== i) {
                throw new CompilerError(
                    "Rest parameter must be last formal parameter.",
                    p,
                    "SyntaxError",
                );
            }
        }
        return p;
    }) || [];
}
