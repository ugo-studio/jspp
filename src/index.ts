import { TypeAnalyzer } from "./analysis/typeAnalyzer";
import { CodeGenerator } from "./core/generator";
import { Parser } from "./core/parser";

export class Interpreter {
    private parser = new Parser();
    private analyzer = new TypeAnalyzer();
    private generator = new CodeGenerator();

    public interpret(jsCode: string): string {
        const ast = this.parser.parse(jsCode);
        this.analyzer.analyze(ast);
        return this.generator.generate(ast, this.analyzer);
    }
}
