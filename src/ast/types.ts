import * as ts from "typescript";

export type Node = ts.Node;

export interface Visitor {
    [key: string]: {
        enter?: (node: Node, parent: Node | null) => void;
        exit?: (node: Node, parent: Node | null) => void;
    };
}

export type DeclaredSymbols = Map<
    string,
    "letOrConst" | "function" | "var"
>;
