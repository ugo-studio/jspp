import * as ts from "typescript";

import type { Node, Visitor } from "../ast/types";
import { Traverser } from "../core/traverser";
import { Scope, ScopeManager } from "./scope";

export interface TypeInfo {
    type: string;
    isClosure?: boolean;
    isParameter?: boolean;
    isConst?: boolean;
    isBuiltin?: boolean;
    captures?: Map<string, TypeInfo>; // <name, typeInfo>
    structName?: string;
    properties?: Map<string, string>;
    elementType?: string;
    declaration?: ts.Node;
}

export class TypeAnalyzer {
    private traverser = new Traverser();
    public readonly scopeManager = new ScopeManager();
    public readonly functionTypeInfo = new Map<
        ts.FunctionDeclaration | ts.ArrowFunction | ts.FunctionExpression,
        TypeInfo
    >();
    private functionStack: (
        | ts.FunctionDeclaration
        | ts.ArrowFunction
        | ts.FunctionExpression
    )[] = [];
    public readonly nodeToScope = new Map<ts.Node, Scope>();

    public analyze(ast: Node) {
        this.nodeToScope.set(ast, this.scopeManager.currentScope);
        const visitor: Visitor = {
            // Enter new scope for any block-like structure
            Block: {
                enter: (node, parent) => {
                    this.scopeManager.enterScope();
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
                    this.scopeManager.enterScope();
                    this.nodeToScope.set(node, this.scopeManager.currentScope);
                },
                exit: () => this.scopeManager.exitScope(),
            },
            ForOfStatement: {
                enter: (node) => {
                    this.scopeManager.enterScope();
                    this.nodeToScope.set(node, this.scopeManager.currentScope);
                },
                exit: () => this.scopeManager.exitScope(),
            },

            ArrowFunction: {
                enter: (node) => {
                    if (ts.isArrowFunction(node)) {
                        const funcType: TypeInfo = {
                            type: "function",
                            isClosure: false,
                            captures: new Map(),
                        };
                        this.functionTypeInfo.set(node, funcType);

                        this.scopeManager.enterScope();
                        this.nodeToScope.set(
                            node,
                            this.scopeManager.currentScope,
                        );
                        // Define parameters in the new scope
                        node.parameters.forEach((p) =>
                            this.scopeManager.define(p.name.getText(), {
                                type: "auto",
                                isParameter: true,
                                declaration: p,
                            })
                        );
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
                        };
                        this.functionTypeInfo.set(node, funcType);

                        this.scopeManager.enterScope();
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
                        node.parameters.forEach((p) =>
                            this.scopeManager.define(p.name.getText(), {
                                type: "auto",
                                isParameter: true,
                                declaration: p,
                            })
                        );
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
                            };
                            this.scopeManager.define(funcName, funcType);
                            this.functionTypeInfo.set(node, funcType);
                        }

                        this.scopeManager.enterScope();
                        this.nodeToScope.set(
                            node,
                            this.scopeManager.currentScope,
                        );
                        // Define parameters in the new scope
                        node.parameters.forEach((p) =>
                            this.scopeManager.define(p.name.getText(), {
                                type: "auto",
                                isParameter: true,
                                declaration: p,
                            })
                        );
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

            VariableDeclaration: {
                enter: (node) => {
                    if (ts.isVariableDeclaration(node)) {
                        const name = node.name.getText();
                        const isConst =
                            (node.parent.flags & ts.NodeFlags.Const) !== 0;

                        let type = "auto";
                        if (node.initializer) {
                            if (
                                ts.isArrayLiteralExpression(node.initializer)
                            ) {
                                type = "array";
                            }
                        }

                        const typeInfo: TypeInfo = {
                            type,
                            declaration: node,
                            isConst,
                        };
                        this.scopeManager.define(name, typeInfo);
                    }
                },
            },

            Identifier: {
                enter: (node, parent) => {
                    if (ts.isIdentifier(node)) {
                        if (node.text === "console") {
                            return;
                        }

                        const currentFuncNode =
                            this.functionStack[this.functionStack.length - 1];
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
                        if (
                            definingScope &&
                            definingScope !== this.scopeManager.currentScope
                        ) {
                            // This is a potential capture!
                            // Find which function we are currently in and mark the capture.
                            if (currentFuncNode) {
                                const type = this.scopeManager.lookup(
                                    node.text,
                                );
                                if (type) {
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
                },
            },
        };

        this.traverser.traverse(ast, visitor);
    }
}
