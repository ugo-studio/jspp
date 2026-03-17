import * as ts from "typescript";

export type Node = ts.Node;

export type Visitor = Partial<
    Record<ts.SyntaxKind, {
        enter?: (node: Node, parent: Node | null) => void;
        exit?: (node: Node, parent: Node | null) => void;
    }>
>;
