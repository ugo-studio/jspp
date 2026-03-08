import * as ts from "typescript";

import type { Node } from "../ast/types.js";

export class Parser {
    public parse(sourceCode: string, fileName?: string): Node {
        return ts.createSourceFile(
            fileName || "temp.js",
            sourceCode,
            ts.ScriptTarget.Latest,
            true,
        );
    }
}
