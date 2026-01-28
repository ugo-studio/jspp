import path from "path";

import { TypeAnalyzer } from "./analysis/typeAnalyzer.js";
import { CodeGenerator } from "./core/codegen/index.js";
import { Parser } from "./core/parser.js";

export class Interpreter {
    private parser = new Parser();
    private analyzer = new TypeAnalyzer();
    private generator = new CodeGenerator();

    public interpret(
        code: string,
        fileName?: string,
    ): { cppCode: string; preludePath: string } {
        const ast = this.parser.parse(code, fileName);
        this.analyzer.analyze(ast);
        const isTypescript = fileName
            ? path.extname(fileName) === ".ts"
            : false;
        const cppCode = this.generator.generate(
            ast,
            this.analyzer,
            isTypescript,
        );
        const preludePath = path.resolve(
            import.meta.dirname,
            "..",
            "src",
            "prelude",
        );
        return { cppCode, preludePath };
    }
}
