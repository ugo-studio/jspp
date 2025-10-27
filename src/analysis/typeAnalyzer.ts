import * as ts from "typescript";

import type { Node, Visitor } from "../ast/types";
import { Traverser } from "../core/traverser";
import { ScopeManager } from "./scope";

export interface TypeInfo {
    type: string;
    isClosure?: boolean;
    captures?: Map<string, TypeInfo>; // <name, typeInfo>
    structName?: string;
    properties?: Map<string, string>;
    elementType?: string;
}

export class TypeAnalyzer {
    private traverser = new Traverser();
    public readonly scopeManager = new ScopeManager();
    public readonly functionTypeInfo = new Map<
        ts.FunctionDeclaration,
        TypeInfo
    >();

    public analyze(ast: Node) {
        const visitor: Visitor = {
            // Enter new scope for any block-like structure
            Block: {
                enter: () => this.scopeManager.enterScope(),
                exit: () => this.scopeManager.exitScope(),
            },
            ForStatement: {
                enter: () => this.scopeManager.enterScope(),
                exit: () => this.scopeManager.exitScope(),
            },
            ForOfStatement: {
                enter: () => this.scopeManager.enterScope(),
                exit: () => this.scopeManager.exitScope(),
            },

            FunctionDeclaration: {
                enter: (node) => {
                    if (ts.isFunctionDeclaration(node)) {
                        const funcName = node.name?.getText();
                        // Pre-define the function in the parent scope
                        if (funcName) {
                            const funcType: TypeInfo = {
                                type: "function",
                                isClosure: false,
                                captures: new Map(),
                            };
                            this.scopeManager.define(funcName, funcType);
                            this.functionTypeInfo.set(node, funcType);
                        }
                        this.scopeManager.enterScope();
                        // Define parameters in the new scope
                        node.parameters.forEach((p) =>
                            this.scopeManager.define(p.name.getText(), {
                                type: "auto",
                            })
                        );
                    }
                },
                exit: () => this.scopeManager.exitScope(),
            },

            VariableDeclaration: {
                enter: (node) => {
                    if (ts.isVariableDeclaration(node) && node.initializer) {
                        const name = node.name.getText();
                        // We can add more detailed inference here if needed
                        const typeInfo = { type: "auto" };
                        this.scopeManager.define(name, typeInfo);
                    }
                },
            },

            Identifier: {
                enter: (node, parent) => {
                    if (ts.isIdentifier(node)) {
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
                            // (A more robust implementation would traverse up the parent nodes to find the enclosing function)
                            this.functionTypeInfo.forEach((info, funcNode) => {
                                // A simplified check: does this function contain the current node?
                                if (
                                    node.getStart() > funcNode.getStart() &&
                                    node.getEnd() < funcNode.getEnd()
                                ) {
                                    const type = this.scopeManager.lookup(
                                        node.text,
                                    );
                                    if (type) {
                                        info.isClosure = true;
                                        info.captures?.set(node.text, type);
                                    }
                                }
                            });
                        }
                    }
                },
            },
        };

        this.traverser.traverse(ast, visitor);
    }
}
