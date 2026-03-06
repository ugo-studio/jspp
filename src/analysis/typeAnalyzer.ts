import * as ts from "typescript";

import type { Node, Visitor } from "../ast/types.js";
import {
  isBuiltinObject,
  shouldIgnoreStatement,
} from "../core/codegen/helpers.js";
import { CompilerError } from "../core/error.js";
import { Traverser } from "../core/traverser.js";
import { Scope, ScopeManager } from "./scope.js";

function getParameterType(node: ts.ParameterDeclaration): string {
    if (node.type) {
        switch (node.type.kind) {
            case ts.SyntaxKind.StringKeyword:
                return "string";
            case ts.SyntaxKind.NumberKeyword:
                return "number";
            case ts.SyntaxKind.BooleanKeyword:
                return "boolean";
            case ts.SyntaxKind.AnyKeyword:
                return "any";
            case ts.SyntaxKind.VoidKeyword:
                return "void";
            case ts.SyntaxKind.ObjectKeyword:
                return "object";
            case ts.SyntaxKind.SymbolKeyword:
                return "symbol";
            case ts.SyntaxKind.UndefinedKeyword:
                return "undefined";
            case ts.SyntaxKind.UnknownKeyword:
                return "unknown";
            case ts.SyntaxKind.NeverKeyword:
                return "never";
            case ts.SyntaxKind.ArrayType:
                return "array";
            case ts.SyntaxKind.FunctionType:
                return "function";
        }
        if (ts.isTypeReferenceNode(node.type)) {
            return node.type.typeName.getText();
        }
        return node.type.getText();
    }
    return "auto";
}

export interface TypeInfo {
    type:
        | "auto"
        | "string"
        | "number"
        | "boolean"
        | "array"
        | "function"
        | "object"
        | "symbol"
        | "void"
        | "undefined"
        | "null"
        | "any"
        | string;
    isClosure?: boolean;
    isParameter?: boolean;
    isConst?: boolean;
    isBuiltin?: boolean;
    needsHeapAllocation?: boolean;
    captures?: Map<string, TypeInfo>; // <name, typeInfo>
    structName?: string;
    properties?: Map<string, string>;
    elementType?: string;
    declaration?: ts.Node;
    assignments?: ts.Expression[];
}

export class TypeAnalyzer {
    private traverser = new Traverser();
    public readonly scopeManager = new ScopeManager();
    public readonly functionTypeInfo = new Map<
        | ts.FunctionDeclaration
        | ts.ArrowFunction
        | ts.FunctionExpression
        | ts.ClassDeclaration
        | ts.MethodDeclaration
        | ts.ConstructorDeclaration,
        TypeInfo
    >();
    private functionStack: (
        | ts.FunctionDeclaration
        | ts.ArrowFunction
        | ts.FunctionExpression
        | ts.ClassDeclaration
        | ts.MethodDeclaration
        | ts.ConstructorDeclaration
    )[] = [];
    public readonly nodeToScope = new Map<ts.Node, Scope>();
    private labelStack: string[] = [];
    private loopDepth = 0;
    private switchDepth = 0;

    public inferFunctionReturnType(
        node: ts.Node,
        scope?: Scope,
        visited: Set<ts.Node> = new Set(),
    ): "boolean" | "number" | "string" | "object" | "function" | "any" {
        if (
            !ts.isFunctionDeclaration(node) && !ts.isFunctionExpression(node) &&
            !ts.isArrowFunction(node) && !ts.isMethodDeclaration(node) &&
            !ts.isConstructorDeclaration(node)
        ) {
            return "any";
        }

        const info = this.functionTypeInfo.get(node as any);
        if (info && info.assignments && info.assignments.length > 0) {
            const types = info.assignments.map((expr) =>
                this.inferNodeReturnType(expr, scope, visited)
            );
            const uniqueTypes = new Set(types.filter((t) => t !== "any"));
            if (uniqueTypes.size === 1) {
                return uniqueTypes.values().next().value as any;
            }
            if (uniqueTypes.size > 1) return "any";
        }

        // Type checking TS signature
        const signature = node as ts.SignatureDeclaration;
        if (signature.type) {
            switch (signature.type.kind) {
                case ts.SyntaxKind.StringKeyword:
                    return "string";
                case ts.SyntaxKind.NumberKeyword:
                    return "number";
                case ts.SyntaxKind.BooleanKeyword:
                    return "boolean";
                case ts.SyntaxKind.ObjectKeyword:
                    return "object";
                case ts.SyntaxKind.VoidKeyword:
                    return "any";
            }
        }

        return "any";
    }

    public inferNodeReturnType(
        node: ts.Node,
        scope?: Scope,
        visited: Set<ts.Node> = new Set(),
    ): "boolean" | "number" | "string" | "object" | "function" | "any" {
        if (visited.has(node)) return "any";
        visited.add(node);

        if (!scope) {
            scope = this.nodeToScope.get(node) || this.scopeManager.currentScope;
        }

        // 1. Literal types
        if (ts.isNumericLiteral(node)) return "number";
        if (
            ts.isStringLiteral(node) || ts.isNoSubstitutionTemplateLiteral(node)
        ) return "string";
        if (
            node.kind === ts.SyntaxKind.TrueKeyword ||
            node.kind === ts.SyntaxKind.FalseKeyword
        ) return "boolean";
        if (
            node.kind === ts.SyntaxKind.NullKeyword ||
            node.kind === ts.SyntaxKind.UndefinedKeyword
        ) return "any";

        // 2. Complex literals / expressions
        if (
            ts.isObjectLiteralExpression(node) ||
            ts.isArrayLiteralExpression(node)
        ) return "object";
        if (
            ts.isFunctionExpression(node) || ts.isArrowFunction(node) ||
            ts.isClassExpression(node)
        ) return "function";
        if (ts.isClassDeclaration(node) || ts.isFunctionDeclaration(node)) {
            return "function";
        }
        if (ts.isEnumDeclaration(node)) return "object";

        // 3. Parenthesized / Await / Yield
        if (ts.isParenthesizedExpression(node)) {
            return this.inferNodeReturnType(node.expression, scope, visited);
        }
        if (ts.isAwaitExpression(node)) {
            return this.inferNodeReturnType(node.expression, scope, visited);
        }
        if (ts.isYieldExpression(node)) {
            return node.expression
                ? this.inferNodeReturnType(node.expression, scope, visited)
                : "any";
        }

        // 4. Unary
        if (ts.isTypeOfExpression(node)) return "string";
        if (ts.isVoidExpression(node)) return "any";
        if (ts.isDeleteExpression(node)) return "boolean";
        if (ts.isPrefixUnaryExpression(node)) {
            switch (node.operator) {
                case ts.SyntaxKind.ExclamationToken:
                    return "boolean";
                case ts.SyntaxKind.PlusToken:
                case ts.SyntaxKind.MinusToken:
                case ts.SyntaxKind.TildeToken:
                case ts.SyntaxKind.PlusPlusToken:
                case ts.SyntaxKind.MinusMinusToken:
                    return "number";
            }
        }
        if (ts.isPostfixUnaryExpression(node)) return "number";

        // 5. Binary
        if (ts.isBinaryExpression(node)) {
            const op = node.operatorToken.kind;
            // Assignment
            if (
                op >= ts.SyntaxKind.FirstAssignment &&
                op <= ts.SyntaxKind.LastAssignment
            ) {
                return this.inferNodeReturnType(node.right, scope, visited);
            }
            // Comparison
            if (
                [
                    ts.SyntaxKind.EqualsEqualsToken,
                    ts.SyntaxKind.ExclamationEqualsToken,
                    ts.SyntaxKind.EqualsEqualsEqualsToken,
                    ts.SyntaxKind.ExclamationEqualsEqualsToken,
                    ts.SyntaxKind.LessThanToken,
                    ts.SyntaxKind.LessThanEqualsToken,
                    ts.SyntaxKind.GreaterThanToken,
                    ts.SyntaxKind.GreaterThanEqualsToken,
                    ts.SyntaxKind.InKeyword,
                    ts.SyntaxKind.InstanceOfKeyword,
                ].includes(op)
            ) {
                return "boolean";
            }
            // Arithmetic
            if (
                [
                    ts.SyntaxKind.MinusToken,
                    ts.SyntaxKind.AsteriskToken,
                    ts.SyntaxKind.SlashToken,
                    ts.SyntaxKind.PercentToken,
                    ts.SyntaxKind.AsteriskAsteriskToken,
                    ts.SyntaxKind.LessThanLessThanToken,
                    ts.SyntaxKind.GreaterThanGreaterThanToken,
                    ts.SyntaxKind.GreaterThanGreaterThanGreaterThanToken,
                    ts.SyntaxKind.AmpersandToken,
                    ts.SyntaxKind.BarToken,
                    ts.SyntaxKind.CaretToken,
                ].includes(op)
            ) {
                return "number";
            }
            // Plus
            if (op === ts.SyntaxKind.PlusToken) {
                const left = this.inferNodeReturnType(node.left, scope, visited);
                const right = this.inferNodeReturnType(
                    node.right,
                    scope,
                    visited,
                );
                if (left === "string" || right === "string") return "string";
                if (left === "number" && right === "number") return "number";
                return "any";
            }
            // Logical
            if (
                [
                    ts.SyntaxKind.AmpersandAmpersandToken,
                    ts.SyntaxKind.BarBarToken,
                    ts.SyntaxKind.QuestionQuestionToken,
                ].includes(op)
            ) {
                const left = this.inferNodeReturnType(node.left, scope, visited);
                const right = this.inferNodeReturnType(
                    node.right,
                    scope,
                    visited,
                );
                return left === right ? left : "any";
            }
        }

        // 6. Conditional
        if (ts.isConditionalExpression(node)) {
            const trueType = this.inferNodeReturnType(
                node.whenTrue,
                scope,
                visited,
            );
            const falseType = this.inferNodeReturnType(
                node.whenFalse,
                scope,
                visited,
            );
            return trueType === falseType ? trueType : "any";
        }

        // 7. Template
        if (ts.isTemplateExpression(node)) return "string";

        // 8. New
        if (ts.isNewExpression(node)) return "object";

        // 9. Call
        if (ts.isCallExpression(node)) {
            const callee = node.expression;
            if (ts.isIdentifier(callee)) {
                const name = callee.text;
                if (
                    name === "Number" || name === "parseInt" ||
                    name === "parseFloat"
                ) return "number";
                if (name === "String") return "string";
                if (
                    name === "Boolean" || name === "isNaN" || name === "isFinite"
                ) return "boolean";

                const typeInfo = this.scopeManager.lookupFromScope(name, scope);
                if (typeInfo && typeInfo.declaration) {
                    return this.inferFunctionReturnType(
                        typeInfo.declaration,
                        scope,
                        visited,
                    );
                }
            } else if (ts.isPropertyAccessExpression(callee)) {
                const obj = callee.expression.getText();
                const prop = callee.name.text;
                const full = `${obj}.${prop}`;
                if (full.startsWith("Math.") && prop !== "PI" && prop !== "E") {
                    return "number";
                }
                if (full === "Array.isArray") return "boolean";
                if (
                    full === "Object.keys" || full === "Object.values" ||
                    full === "Object.entries"
                ) return "object";
            }
            return "any";
        }

        // 10. Identifier
        if (ts.isIdentifier(node)) {
            if (node.text === "NaN" || node.text === "Infinity") return "number";
            if (node.text === "undefined") return "any";

            const typeInfo = this.scopeManager.lookupFromScope(node.text, scope);
            if (typeInfo) {
                // Check assignments
                if (typeInfo.assignments && typeInfo.assignments.length > 0) {
                    const types = typeInfo.assignments.map((expr) =>
                        this.inferNodeReturnType(expr, scope, visited)
                    );
                    const uniqueTypes = new Set(types.filter((t) => t !== "any"));
                    if (uniqueTypes.size === 1) {
                        return uniqueTypes.values().next().value as any;
                    }
                    if (uniqueTypes.size > 1) return "any";
                }

                // Check declaration for TS type
                if (
                    typeInfo.declaration &&
                    ts.isVariableDeclaration(typeInfo.declaration) &&
                    typeInfo.declaration.type
                ) {
                    const kind = typeInfo.declaration.type.kind;
                    if (kind === ts.SyntaxKind.StringKeyword) return "string";
                    if (kind === ts.SyntaxKind.NumberKeyword) return "number";
                    if (kind === ts.SyntaxKind.BooleanKeyword) return "boolean";
                    if (kind === ts.SyntaxKind.ObjectKeyword) return "object";
                }

                // Builtin check
                if (typeInfo.isBuiltin) {
                    if (
                        ["console", "Math", "process", "global", "globalThis"]
                            .includes(node.text)
                    ) return "object";
                }

                // Fallback to what we know
                if (
                    typeInfo.type === "string" || typeInfo.type === "number" ||
                    typeInfo.type === "boolean" || typeInfo.type === "object" ||
                    typeInfo.type === "function"
                ) {
                    return typeInfo.type as any;
                }
                if (typeInfo.type === "array") return "object";
            }
        }

        // 11. Property Access
        if (ts.isPropertyAccessExpression(node)) {
            if (node.name.text === "length") {
                const objType = this.inferNodeReturnType(
                    node.expression,
                    scope,
                    visited,
                );
                if (objType === "string" || objType === "object") return "number";
            }
            // Hard to know other properties without full type system
            return "any";
        }

        // 12. Function / Method Return type inference
        if (
            ts.isFunctionDeclaration(node) || ts.isFunctionExpression(node) ||
            ts.isArrowFunction(node) || ts.isMethodDeclaration(node) ||
            ts.isConstructorDeclaration(node)
        ) {
            return this.inferFunctionReturnType(node, scope, visited);
        }

        return "any";
    }

    private defineParameter(
        nameNode: ts.BindingName,
        p: ts.ParameterDeclaration,
        type?: string,
    ) {
        if (ts.isIdentifier(nameNode)) {
            this.scopeManager.define(nameNode.text, {
                type: type || getParameterType(p),
                isParameter: true,
                declaration: p,
            });
        } else {
            nameNode.elements.forEach((element) => {
                if (ts.isBindingElement(element)) {
                    this.defineParameter(element.name, p, type);
                }
            });
        }
    }

    public analyze(ast: Node) {
        this.nodeToScope.set(ast, this.scopeManager.currentScope);

        const crossScopeModificationVisitor = (node: ts.Expression) => {
            if (ts.isIdentifier(node)) {
                const name = node.getText();
                const definingScope = this.scopeManager.currentScope
                    .findScopeFor(name);
                if (definingScope) {
                    const currentFuncNode =
                        this.functionStack[this.functionStack.length - 1] ??
                            null;
                    const definingFunc = definingScope.ownerFunction;
                    if (definingFunc !== currentFuncNode) {
                        const typeInfo = this.scopeManager.lookup(name);
                        if (typeInfo) {
                            typeInfo.needsHeapAllocation = true;
                        }
                    }
                }
            }
        };

        const visitor: Visitor = {
            // Enter new scope for any block-like structure
            Block: {
                enter: (node, parent) => {
                    const currentFuncNode =
                        this.functionStack[this.functionStack.length - 1] ??
                            null;
                    this.scopeManager.enterScope(currentFuncNode);
                    this.nodeToScope.set(node, this.scopeManager.currentScope);
                    if (
                        parent && ts.isCatchClause(parent) &&
                        parent.variableDeclaration
                    ) {
                        if (ts.isIdentifier(parent.variableDeclaration.name)) {
                            const name = parent.variableDeclaration.name
                                .getText();
                            // The type is basically 'any' or 'string' from the .what()
                            const typeInfo: TypeInfo = {
                                type: "string",
                                declaration: parent.variableDeclaration,
                            };
                            this.scopeManager.define(name, typeInfo);
                        }
                        // TODO: handle binding patterns in catch clause
                    }
                },
                exit: () => this.scopeManager.exitScope(),
            },
            ForStatement: {
                enter: (node) => {
                    this.loopDepth++;
                    const currentFuncNode =
                        this.functionStack[this.functionStack.length - 1] ??
                            null;
                    this.scopeManager.enterScope(currentFuncNode);
                    this.nodeToScope.set(node, this.scopeManager.currentScope);
                },
                exit: () => {
                    this.loopDepth--;
                    this.scopeManager.exitScope();
                },
            },
            ForOfStatement: {
                enter: (node) => {
                    this.loopDepth++;
                    const currentFuncNode =
                        this.functionStack[this.functionStack.length - 1] ??
                            null;
                    this.scopeManager.enterScope(currentFuncNode);
                    this.nodeToScope.set(node, this.scopeManager.currentScope);
                },
                exit: () => {
                    this.loopDepth--;
                    this.scopeManager.exitScope();
                },
            },
            ForInStatement: {
                enter: (node) => {
                    this.loopDepth++;
                    const currentFuncNode =
                        this.functionStack[this.functionStack.length - 1] ??
                            null;
                    this.scopeManager.enterScope(currentFuncNode);
                    this.nodeToScope.set(node, this.scopeManager.currentScope);
                    const forIn = node as ts.ForInStatement;
                    if (ts.isVariableDeclarationList(forIn.initializer)) {
                        const varDecl = forIn.initializer.declarations[0];
                        if (varDecl) {
                            const name = varDecl.name.getText();
                            const isConst =
                                (varDecl.parent.flags & ts.NodeFlags.Const) !==
                                    0;
                            const typeInfo: TypeInfo = {
                                type: "string", // Keys are always strings
                                declaration: varDecl,
                                isConst,
                            };
                            this.scopeManager.define(name, typeInfo);
                        }
                    } else if (ts.isIdentifier(forIn.initializer)) {
                        // If it's an existing identifier, we don't redefine it, but ensure it's in scope.
                        // The generator will handle assigning to it.
                    }
                },
                exit: () => {
                    this.loopDepth--;
                    this.scopeManager.exitScope();
                },
            },
            WhileStatement: {
                enter: (node) => {
                    this.loopDepth++;
                    const currentFuncNode =
                        this.functionStack[this.functionStack.length - 1] ??
                            null;
                    this.scopeManager.enterScope(currentFuncNode);
                    this.nodeToScope.set(node, this.scopeManager.currentScope);
                },
                exit: () => {
                    this.loopDepth--;
                    this.scopeManager.exitScope();
                },
            },
            DoStatement: {
                enter: (node) => {
                    this.loopDepth++;
                    const currentFuncNode =
                        this.functionStack[this.functionStack.length - 1] ??
                            null;
                    this.scopeManager.enterScope(currentFuncNode);
                    this.nodeToScope.set(node, this.scopeManager.currentScope);
                },
                exit: () => {
                    this.loopDepth--;
                    this.scopeManager.exitScope();
                },
            },

            SwitchStatement: {
                enter: (node) => {
                    this.switchDepth++;
                    const currentFuncNode =
                        this.functionStack[this.functionStack.length - 1] ??
                            null;
                    this.scopeManager.enterScope(currentFuncNode);
                    this.nodeToScope.set(node, this.scopeManager.currentScope);
                },
                exit: () => {
                    this.switchDepth--;
                    this.scopeManager.exitScope();
                },
            },

            LabeledStatement: {
                enter: (node) => {
                    this.nodeToScope.set(node, this.scopeManager.currentScope);
                    this.labelStack.push(
                        (node as ts.LabeledStatement).label.text,
                    );
                },
                exit: () => {
                    this.labelStack.pop();
                },
            },

            BreakStatement: {
                enter: (node) => {
                    const breakNode = node as ts.BreakStatement;
                    if (breakNode.label) {
                        if (!this.labelStack.includes(breakNode.label.text)) {
                            throw new CompilerError(
                                `Undefined label '${breakNode.label.text}'`,
                                breakNode.label,
                                "SyntaxError",
                            );
                        }
                    } else {
                        if (this.loopDepth === 0 && this.switchDepth === 0) {
                            throw new CompilerError(
                                "Unlabeled break must be inside an iteration or switch statement",
                                node,
                                "SyntaxError",
                            );
                        }
                    }
                },
            },

            ContinueStatement: {
                enter: (node) => {
                    const continueNode = node as ts.ContinueStatement;
                    if (continueNode.label) {
                        if (
                            !this.labelStack.includes(continueNode.label.text)
                        ) {
                            throw new CompilerError(
                                `Undefined label '${continueNode.label.text}'`,
                                continueNode.label,
                                "SyntaxError",
                            );
                        }
                        // Also need to check if the label belongs to a loop, but that's harder here.
                        // The TS checker should handle this. We'll assume for now it does.
                    } else {
                        if (this.loopDepth === 0) {
                            throw new CompilerError(
                                "Unlabeled continue must be inside an iteration statement",
                                node,
                                "SyntaxError",
                            );
                        }
                    }
                },
            },

            ArrowFunction: {
                enter: (node) => {
                    if (ts.isArrowFunction(node)) {
                        const funcType: TypeInfo = {
                            type: "function",
                            isClosure: false,
                            captures: new Map(),
                            assignments: !ts.isBlock(node.body) ? [node.body] : [],
                        };
                        this.functionTypeInfo.set(node, funcType);

                        this.scopeManager.enterScope(node);
                        this.nodeToScope.set(
                            node,
                            this.scopeManager.currentScope,
                        );

                        // Define parameters in the new scope
                        node.parameters.forEach((p) => {
                            if (p.getText() == "this") { // Catch invalid parameters
                                throw new CompilerError(
                                    "Cannot use 'this' as a parameter name.",
                                    p,
                                    "SyntaxError",
                                );
                            }

                            this.defineParameter(p.name, p);
                        });
                        this.functionStack.push(node);
                    }
                },
                exit: (node) => {
                    if (ts.isArrowFunction(node)) {
                        this.functionStack.pop();
                    }
                    this.scopeManager.exitScope();
                },
            },

            FunctionExpression: {
                enter: (node) => {
                    if (ts.isFunctionExpression(node)) {
                        const funcType: TypeInfo = {
                            type: "function",
                            isClosure: false,
                            captures: new Map(),
                            declaration: node,
                            needsHeapAllocation: true,
                            assignments: [],
                        };
                        this.functionTypeInfo.set(node, funcType);

                        this.scopeManager.enterScope(node);
                        this.nodeToScope.set(
                            node,
                            this.scopeManager.currentScope,
                        );

                        // If the function expression is named, define the name within its own scope for recursion.
                        if (node.name) {
                            this.scopeManager.define(
                                node.name.getText(),
                                funcType,
                            );
                        }

                        // Define parameters in the new scope
                        node.parameters.forEach((p) => {
                            if (p.getText() == "this") { // Catch invalid parameters
                                throw new CompilerError(
                                    "Cannot use 'this' as a parameter name.",
                                    p,
                                    "SyntaxError",
                                );
                            }

                            this.defineParameter(p.name, p);
                        });
                        this.functionStack.push(node);
                    }
                },
                exit: (node) => {
                    if (ts.isFunctionExpression(node)) {
                        this.functionStack.pop();
                    }
                    this.scopeManager.exitScope();
                },
            },

            FunctionDeclaration: {
                enter: (node) => {
                    if (ts.isFunctionDeclaration(node)) {
                        // Define the function in the current scope.
                        if (node.name) {
                            const funcName = node.name.getText();
                            const funcType: TypeInfo = {
                                type: "function",
                                isClosure: false,
                                captures: new Map(),
                                declaration: node,
                                needsHeapAllocation: true,
                                assignments: [],
                            };
                            this.scopeManager.define(funcName, funcType);
                            this.functionTypeInfo.set(node, funcType);
                        }

                        this.scopeManager.enterScope(node);
                        this.nodeToScope.set(
                            node,
                            this.scopeManager.currentScope,
                        );

                        // Define parameters in the new scope
                        node.parameters.forEach((p) => {
                            if (p.getText() == "this") { // Catch invalid parameters
                                throw new CompilerError(
                                    "Cannot use 'this' as a parameter name.",
                                    p,
                                    "SyntaxError",
                                );
                            }

                            this.defineParameter(p.name, p);
                        });
                        this.functionStack.push(node);
                    }
                },
                exit: (node) => {
                    if (ts.isFunctionDeclaration(node)) {
                        this.functionStack.pop();
                    }
                    this.scopeManager.exitScope();
                },
            },

            ClassDeclaration: {
                enter: (node) => {
                    const classNode = node as ts.ClassDeclaration;
                    if (classNode.name) {
                        const name = classNode.name.getText();
                        const typeInfo: TypeInfo = {
                            type: "function", // Classes are functions
                            declaration: classNode,
                            needsHeapAllocation: true,
                            assignments: [],
                        };
                        this.scopeManager.define(name, typeInfo);
                    }
                    const currentFuncNode =
                        this.functionStack[this.functionStack.length - 1] ??
                            null;
                    this.scopeManager.enterScope(currentFuncNode);
                    this.nodeToScope.set(node, this.scopeManager.currentScope);
                },
                exit: () => {
                    this.scopeManager.exitScope();
                },
            },

            EnumDeclaration: {
                enter: (node) => {
                    const enumNode = node as ts.EnumDeclaration;
                    if (enumNode.name) {
                        const name = enumNode.name.getText();
                        const typeInfo: TypeInfo = {
                            type: "object", // Enums are objects
                            declaration: enumNode,
                            needsHeapAllocation: true,
                        };
                        this.scopeManager.define(name, typeInfo);
                    }
                    // Enums don't create a new scope for variables, but they are objects.
                    // We don't strictly need to enterScope unless we want to track something inside.
                    // But standard JS/TS behavior doesn't really have block scope inside enum definition that leaks out or captures differently.
                    // However, we map nodes to scopes.
                    this.nodeToScope.set(node, this.scopeManager.currentScope);
                },
            },

            MethodDeclaration: {
                enter: (node) => {
                    if (ts.isMethodDeclaration(node)) {
                        const funcType: TypeInfo = {
                            type: "function",
                            isClosure: false,
                            captures: new Map(),
                            declaration: node,
                            needsHeapAllocation: true,
                            assignments: [],
                        };                        // Methods don't need to be defined in scope by name generally,
                        // but we need to track them for captures.
                        this.functionTypeInfo.set(node, funcType);

                        this.scopeManager.enterScope(node);
                        this.nodeToScope.set(
                            node,
                            this.scopeManager.currentScope,
                        );
                        node.parameters.forEach((p) =>
                            this.defineParameter(p.name, p, "auto")
                        );
                        this.functionStack.push(node);
                    }
                },
                exit: (node) => {
                    if (ts.isMethodDeclaration(node)) {
                        this.functionStack.pop();
                    }
                    this.scopeManager.exitScope();
                },
            },

            Constructor: {
                enter: (node) => {
                    if (ts.isConstructorDeclaration(node)) {
                        const funcType: TypeInfo = {
                            type: "function",
                            isClosure: false,
                            captures: new Map(),
                            declaration: node,
                            needsHeapAllocation: true,
                            assignments: [],
                        };                        this.functionTypeInfo.set(node, funcType);

                        this.scopeManager.enterScope(node);
                        this.nodeToScope.set(
                            node,
                            this.scopeManager.currentScope,
                        );
                        node.parameters.forEach((p) =>
                            this.defineParameter(p.name, p, "auto")
                        );
                        this.functionStack.push(node); // Constructor acts like a function
                    }
                },
                exit: (node) => {
                    if (ts.isConstructorDeclaration(node)) {
                        this.functionStack.pop();
                    }
                    this.scopeManager.exitScope();
                },
            },

            VariableDeclaration: {
                enter: (node) => {
                    if (ts.isVariableDeclaration(node)) {
                        // Check if it is an ambient declaration (declare var/let/const ...)
                        let isAmbient = false;
                        if (
                            ts.isVariableDeclarationList(node.parent) &&
                            ts.isVariableStatement(node.parent.parent)
                        ) {
                            if (shouldIgnoreStatement(node.parent.parent)) {
                                isAmbient = true;
                            }
                        }

                        if (isAmbient) return;

                        const isBlockScoped = (node.parent.flags &
                            (ts.NodeFlags.Let | ts.NodeFlags.Const)) !== 0;
                        const isConst =
                            (node.parent.flags & ts.NodeFlags.Const) !== 0;

                        const defineName = (nameNode: ts.BindingName) => {
                            if (ts.isIdentifier(nameNode)) {
                                const name = nameNode.text;
                                let type = "auto";
                                let needsHeap = false;

                                // Type inference only for simple identifiers
                                if (
                                    nameNode === node.name && node.initializer
                                ) {
                                    if (
                                        ts.isArrayLiteralExpression(
                                            node.initializer,
                                        )
                                    ) {
                                        type = "array";
                                    } else if (
                                        ts.isArrowFunction(node.initializer) ||
                                        ts.isFunctionExpression(
                                            node.initializer,
                                        )
                                    ) {
                                        type = "function";
                                        needsHeap = true;
                                    } else if (
                                        ts.isIdentifier(node.initializer)
                                    ) {
                                        const typeInfo = this.scopeManager
                                            .lookup(
                                                node.initializer.text,
                                            );
                                        needsHeap =
                                            typeInfo?.needsHeapAllocation ??
                                                false;
                                    }
                                }

                                const typeInfo: TypeInfo = {
                                    type,
                                    declaration: node,
                                    isConst,
                                    needsHeapAllocation: needsHeap,
                                    assignments: node.initializer ? [node.initializer] : [],
                                };

                                if (isBlockScoped) {
                                    this.scopeManager.define(name, typeInfo);
                                } else {
                                    this.scopeManager.defineVar(name, typeInfo);
                                }
                            } else {
                                nameNode.elements.forEach((element) => {
                                    if (ts.isBindingElement(element)) {
                                        defineName(element.name);
                                    }
                                });
                            }
                        };

                        defineName(node.name);
                    }
                },
            },

            Identifier: {
                enter: (node, parent) => {
                    if (ts.isIdentifier(node)) {
                        if (isBuiltinObject.call(this, node)) return;

                        const currentFuncNode =
                            this.functionStack[this.functionStack.length - 1] ??
                                null;
                        if (
                            currentFuncNode &&
                            (ts.isFunctionDeclaration(currentFuncNode) ||
                                ts.isFunctionExpression(currentFuncNode)) &&
                            node.text === currentFuncNode.name?.getText()
                        ) {
                            return; // Don't treat recursive call as capture
                        }

                        // Check if this identifier is being used as a variable (not a function name, etc.)
                        // And see if it belongs to an outer scope
                        const definingScope = this.scopeManager.currentScope
                            .findScopeFor(node.text);
                        if (definingScope) {
                            const definingFunc = definingScope.ownerFunction;
                            if (definingFunc !== currentFuncNode) {
                                // This is a capture!
                                const type = this.scopeManager.lookup(
                                    node.text,
                                );
                                if (type) {
                                    type.needsHeapAllocation = true;
                                    if (currentFuncNode) {
                                        const info = this.functionTypeInfo.get(
                                            currentFuncNode,
                                        );
                                        if (info) {
                                            info.isClosure = true;
                                            info.captures?.set(node.text, type);
                                        }
                                    }
                                }
                            }
                        }
                    }
                },
            },

            BinaryExpression: {
                enter: (node) => {
                    if (ts.isBinaryExpression(node)) {
                        const isAssignment = node.operatorToken.kind >=
                                ts.SyntaxKind.FirstAssignment &&
                            node.operatorToken.kind <=
                                ts.SyntaxKind.LastAssignment;
                        if (isAssignment) {
                            crossScopeModificationVisitor(node.left);

                            if (ts.isIdentifier(node.left)) {
                                const name = node.left.text;
                                const typeInfo = this.scopeManager.lookup(name);
                                if (typeInfo) {
                                    if (!typeInfo.assignments) {
                                        typeInfo.assignments = [];
                                    }
                                    typeInfo.assignments.push(node.right);
                                }
                            }
                        }
                    }
                },
            },
            ReturnStatement: {
                enter: (node) => {
                    if (ts.isReturnStatement(node) && node.expression) {
                        const currentFuncNode =
                            this.functionStack[this.functionStack.length - 1];
                        if (currentFuncNode) {
                            const info = this.functionTypeInfo.get(
                                currentFuncNode,
                            );
                            if (info) {
                                if (!info.assignments) {
                                    info.assignments = [];
                                }
                                info.assignments.push(node.expression);
                            }
                        }
                    }
                },
            },
            PostfixUnaryExpression: {
                enter: (node) => {
                    if (ts.isPostfixUnaryExpression(node)) {
                        crossScopeModificationVisitor(node.operand);
                    }
                },
            },
            PrefixUnaryExpression: {
                enter: (node) => {
                    if (ts.isPrefixUnaryExpression(node)) {
                        const op = node.operator;
                        if (
                            op === ts.SyntaxKind.PlusPlusToken ||
                            op === ts.SyntaxKind.MinusMinusToken
                        ) {
                            crossScopeModificationVisitor(node.operand);
                        }
                    }
                },
            },
        };

        this.traverser.traverse(ast, visitor);
    }
}
