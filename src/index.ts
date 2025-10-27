import { CodeGenerator } from "./core/generator";
import { Parser } from "./core/parser";

class Interpreter {
    private parser = new Parser();
    private generator = new CodeGenerator();

    public interpret(jsCode: string): string {
        const ast = this.parser.parse(jsCode);
        return this.generator.generate(ast);
    }
}

// Example Usage
const jsCode = `
let message = "Hello from JavaScript!";
console.log(message);

function greet(name) {
    console.log("Hello, " + name);
    return 0;
}

greet("C++");

let x = 10.1+'';

for (let i = 0; i < 5; i++) {
    if (i % 2 === 0) {
        console.log(i, "is even");
    } else {
        console.log(i, "is odd");
    }
}
`;

const interpreter = new Interpreter();
const cppCode = interpreter.interpret(jsCode);

console.log("--- Generated C++ Code ---");
console.log(cppCode);
