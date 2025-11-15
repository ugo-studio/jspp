import path from "path";

import { TypeAnalyzer } from "./analysis/typeAnalyzer";
import { CodeGenerator } from "./core/codegen";
import { Parser } from "./core/parser";

export class Interpreter {
    private parser = new Parser();
    private analyzer = new TypeAnalyzer();
    private generator = new CodeGenerator();

    public interpret(
        jsCode: string,
    ): { cppCode: string; preludePath: string } {
        const ast = this.parser.parse(jsCode);
        this.analyzer.analyze(ast);
        const cppCode = this.generator.generate(ast, this.analyzer);
        const preludePath = path.resolve(
            __dirname,
            "..",
            "src",
            "prelude",
        );
        return { cppCode, preludePath };
    }
}
