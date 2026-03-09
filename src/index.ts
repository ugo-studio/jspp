import path from "path";

import { TypeAnalyzer } from "./interpreter/analysis/typeAnalyzer.js";
import { CodeGenerator } from "./interpreter/core/codegen/index.js";
import { Parser } from "./interpreter/core/parser.js";

export class Interpreter {
    private parser = new Parser();
    private analyzer = new TypeAnalyzer();
    private generator = new CodeGenerator();

    public interpret(
        code: string,
        fileName?: string,
        target: "native" | "wasm" = "native",
    ): {
        cppCode: string;
        preludePath: string;
        wasmExports: {
            jsName: string;
            params: { name: string; type: string }[];
            returnType: string;
        }[];
    } {
        const ast = this.parser.parse(code, fileName);
        this.analyzer.analyze(ast);
        const isTypescript = fileName
            ? path.extname(fileName) === ".ts"
            : false;
        const cppCode = this.generator.generate(
            ast,
            this.analyzer,
            isTypescript,
            target === "wasm",
        );
        const preludePath = path.resolve(
            import.meta.dirname,
            "..",
            "src",
            "prelude",
        );
        return {
            cppCode,
            preludePath,
            wasmExports: this.generator.wasmExports.map((e) => ({
                jsName: e.jsName,
                params: e.params.map((p) => ({
                    name: p.name.getText(),
                    type: this.analyzer.inferNodeReturnType(p),
                })),
                returnType: "number", // Currently always number as wrappers use double
            })),
        };
    }
}
