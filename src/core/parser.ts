import * as ts from "typescript";

import type { Node } from "../ast/types";

export class Parser {
    public parse(sourceCode: string): Node {
        return ts.createSourceFile(
            "temp.js",
            sourceCode,
            ts.ScriptTarget.Latest,
            true,
        );
    }
}
