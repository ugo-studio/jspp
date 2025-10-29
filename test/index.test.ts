import fs from "fs/promises";
import path from "path";

import { Interpreter } from "../src";

const runTest = async (caseName: string, expectFail = false) => {
    const inputFile = path.join(
        process.cwd(),
        "test",
        "cases",
        `${caseName}.js`,
    );
    const outputFile = path.join(
        process.cwd(),
        "test",
        "output",
        `${caseName}.cpp`,
    );

    const jsCode = await fs.readFile(inputFile, "utf-8");
    const interpreter = new Interpreter();

    if (expectFail) {
        try {
            interpreter.interpret(jsCode);
            console.error(
                `ERROR: Test case ${caseName} was expected to fail but succeeded.`,
            );
            process.exit(1);
        } catch (e: any) {
            console.log(
                `--- Caught expected error for ${caseName}: ${e.message} ---`,
            );
        }
        return;
    }

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
        "hoisting",
        "mutability",
        "console",
        "void",
        "log-function",
        "recursion",
        "if-else",
        "default-params",
        "declarations",
        "try-catch",
        "global-return",
        "iife",
    ];

    for (const caseName of cases) {
        await runTest(caseName);
    }

    await runTest("reserved-keyword", true);
};

main();
