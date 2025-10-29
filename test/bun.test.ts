import { describe, expect, test } from "bun:test";
import fs from "fs/promises";
import path from "path";

import { Interpreter } from "../src";

const cases: { name: string; expected: string[] }[] = [
    {
        name: "dynamic",
        expected: [
            "--- Dynamic ---",
            "undefined",
            "hello",
            "1",
            "true",
            "null",
        ],
    },
    {
        name: "values",
        expected: [
            "--- Values ---",
            "undefined",
            "null",
            "true",
            "false",
            "1",
            "3.14",
            "string",
        ],
    },
    {
        name: "functions",
        expected: [
            "--- Functions ---",
            "local1",
            "local0",
            "hoisted",
            "1",
            "2",
            "undefined",
        ],
    },
    {
        name: "arrow-functions",
        expected: ["--- Arrow functions ---", "func1", "func2"],
    },
    {
        name: "hoisting",
        expected: [
            "This should be undefined: undefined",
            "Caught expected error for let:",
            "Caught expected error for const:",
            "This should be 5: 5",
        ],
    },
    { name: "mutability", expected: ["--- Mutability ---", "hello", "world"] },
    {
        name: "console",
        expected: [
            "--- Console ---",
            "hello world",
            "this is a warning",
            "this is an error",
        ],
    },
    { name: "void", expected: ["--- Void ---", "hello", "undefined"] },
    {
        name: "log-function",
        expected: ["--- Log function ---", "function(){}", "function(){}"],
    },
    {
        name: "recursion",
        expected: [
            "--- Recursion ---",
            "Factorial of 5: 120",
            "Is 10 even? true",
            "Is 10 odd? false",
            "Is 7 even? false",
            "Is 7 odd? true",
        ],
    },
    {
        name: "if-else",
        expected: [
            "--- If-ElseIf-Else ---",
            "Positive",
            "Negative",
            "Zero",
            "20",
            "GreaterThan5",
        ],
    },
    {
        name: "default-params",
        expected: [
            "--- Default-parameters ---",
            "John undefined",
            "value defaultValue1",
        ],
    },
    {
        name: "declarations",
        expected: [
            "--- Declarations ---",
            "- var -",
            "1",
            "2",
            "- let -",
            "3",
            "4",
            "- const -",
            "5",
            "Caught expected error",
            "6",
        ],
    },
    {
        name: "try-catch",
        expected: [
            "--- Try-Catch-Finally ---",
            "Caught error: This is an error",
            "Inner variable: This is a new variable",
            "Assigned leaked variable: This is another new variable",
            "Caught error without exception variable",
            "Caught error before finally",
            "This is the finally block",
            "Caught propagated error: This is an error",
            "This is the finally block after a return statement",
            "This is the return value",
        ],
    },
    {
        name: "global-return",
        expected: [
            "--- Global Return ---",
            "SyntaxError: Return statements are only valid inside functions.",
        ],
    },
    {
        name: "iife",
        expected: [
            "--- IIFE ---",
            "Arrow IIFE",
            "Anon function IIFE",
            "Named function IIFE",
            "IIFE with args and return: 3",
            "3",
            "2",
            "1",
            "Blast off!",
            "Caught expected error for named IIFE leak",
        ],
    },
    {
        name: "objects",
        expected: [
            "--- Objects ---",
            "1",
            "hello",
            "true",
            "2",
            "nested",
            "nested",
            "new",
            "new",
        ],
    },
    {
        name: "arrays",
        expected: [
            "--- Arrays ---",
            "1",
            "hello",
            "true",
            "3",
            "world",
            "2",
            "0",
            "first",
            "1",
        ],
    },
];

describe("Interpreter tests", () => {
    for (const { name: caseName, expected } of cases) {
        test(`should correctly interpret and run ${caseName}.js`, async () => {
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
            const exeFile = path.join(
                process.cwd(),
                "test",
                "output",
                `${caseName}.exe`,
            );

            const jsCode = await fs.readFile(inputFile, "utf-8");
            const interpreter = new Interpreter();
            const cppCode = interpreter.interpret(jsCode);

            await fs.mkdir(path.dirname(outputFile), { recursive: true });
            await fs.writeFile(outputFile, cppCode);

            const compile = Bun.spawnSync({
                cmd: ["g++", outputFile, "-o", exeFile, "-std=c++23"],
                stdout: "pipe",
                stderr: "pipe",
            });

            if (compile.exitCode !== 0) {
                throw new Error(
                    `C++ compilation failed for ${caseName}: ${compile.stderr.toString()}`,
                );
            }

            try {
                const run = Bun.spawnSync({
                    cmd: [exeFile],
                    stdout: "pipe",
                    stderr: "pipe",
                });

                const stdout = run.stdout.toString().trim().replace(
                    /\r\n/g,
                    "\n",
                );
                const stderr = run.stderr.toString().trim().replace(
                    /\r\n/g,
                    "\n",
                );
                const output = `${stdout}\n${stderr}`.trim();

                for (const expectedString of expected) {
                    expect(output).toInclude(expectedString);
                }
            } catch (e: any) {
                throw new Error(
                    `Execution failed for ${caseName}: ${e.message}`,
                );
            } finally {
                await fs.unlink(exeFile);
            }
        }, 10000);
    }

    test("should throw an error for reserved-keyword.js", async () => {
        const inputFile = path.join(
            process.cwd(),
            "test",
            "cases",
            "reserved-keyword.js",
        );
        const jsCode = await fs.readFile(inputFile, "utf-8");
        const interpreter = new Interpreter();

        expect(() => interpreter.interpret(jsCode)).toThrow(
            'SyntaxError: Unexpected reserved word "std"',
        );
    });
});
