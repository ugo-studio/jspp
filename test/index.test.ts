import fs from "fs/promises";
import path from "path";
import { Interpreter } from "../src";

const runTest = async (caseName: string) => {
    const inputFile = path.join(process.cwd(), "test", "cases", `${caseName}.js`);
    const outputFile = path.join(process.cwd(), "test", "output", `${caseName}.cpp`);

    const jsCode = await fs.readFile(inputFile, "utf-8");
    const interpreter = new Interpreter();
    const cppCode = interpreter.interpret(jsCode);

    await fs.mkdir(path.dirname(outputFile), { recursive: true });
    await fs.writeFile(outputFile, cppCode);

    console.log(`--- Generated C++ Code for ${caseName} (${outputFile}) ---`);
};

const main = async () => {
    const cases = [
        "dynamic",
        "values",
        "functions",
        "arrow-functions",
        "mutability",
        "console",
        "void",
        "log-function",
        "recursion",
        "if-else",
    ];

    for (const caseName of cases) {
        await runTest(caseName);
    }
};

main();
