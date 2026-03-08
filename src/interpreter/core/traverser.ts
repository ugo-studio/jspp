import * as ts from "typescript";

import type { Node, Visitor } from "../ast/types.js";

export class Traverser {
    public traverse(node: Node, visitor: Visitor) {
        this.traverseNode(node, null, visitor);
    }

    private traverseNode(node: Node, parent: Node | null, visitor: Visitor) {
        const nodeKind = ts.SyntaxKind[node.kind];
        const visitorActions = visitor[nodeKind];

        if (visitorActions && visitorActions.enter) {
            visitorActions.enter(node, parent);
        }

        ts.forEachChild(node, (childNode) => {
            this.traverseNode(childNode, node, visitor);
        });

        if (visitorActions && visitorActions.exit) {
            visitorActions.exit(node, parent);
        }
    }
}
