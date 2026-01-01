import os from "node:os";

import { describe, expect, test } from "bun:test";
import fs from "fs/promises";
import path from "path";

import { Interpreter } from "../src";
import cases from "./expected-results.json";

// --- Configuration ---
// Dynamically set concurrency based on the number of logical CPU cores
// Using Math.max to ensure at least 1 core is used even on small VMs
const CONCURRENCY = Math.max(1, Math.floor(os.cpus().length / 2));

// --- Helper: Strip ANSI Codes ---
const stripAnsi = (str: string) =>
    str.replace(
        /[\u001b\u009b][[()#;?]*(?:[0-9]{1,4}(?:;[0-9]{0,4})*)?[0-9A-ORZcf-nqry=><]/g,
        "",
    );

describe("Interpreter tests", () => {
    const caseQueue = cases.map((c, i) => ({ c, i }));
    const casePromises: {
        case: typeof cases[number];
        promise: Promise<
            {
                output: string;
                expected: string[];
                error?: undefined;
            } | {
                error: any;
                output?: undefined;
                expected?: undefined;
            }
        >;
        resolve: (value: any) => void;
    }[] = cases.map((c) => {
        let resolve: (value: any) => void = undefined as any;
        const promise = new Promise<any>((res) => (resolve = res));
        return {
            case: c,
            promise,
            resolve,
        };
    });

    const worker = async () => {
        while (caseQueue.length > 0) {
            const chunk = caseQueue.shift();
            if (!chunk) continue;

            const caseName = chunk.c.name;
            const expected = chunk.c.expected;
            const resolve = casePromises[chunk.i].resolve;

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

            try {
                const jsCode = await fs.readFile(inputFile, "utf-8");
                const interpreter = new Interpreter();
                const { cppCode, preludePath } = interpreter.interpret(jsCode);

                await fs.mkdir(path.dirname(outputFile), {
                    recursive: true,
                });
                await fs.writeFile(outputFile, cppCode);

                const compile = Bun.spawn(
                    [
                        "g++",
                        "-g",
                        "-Og",
                        "-std=c++23",
                        outputFile,
                        "-o",
                        exeFile,
                        "-I",
                        preludePath,
                    ],
                    {
                        stdout: "pipe",
                        stderr: "pipe",
                    },
                );

                const compileExitCode = await compile.exited;

                if (compileExitCode !== 0) {
                    const stderr = await new Response(compile.stderr).text();
                    const stdout = await new Response(compile.stdout).text();
                    throw new Error(
                        `C++ compilation failed for ${caseName}: ${stderr}\nSTDOUT: ${stdout}`,
                    );
                }

                const run = Bun.spawn([exeFile], {
                    stdout: "pipe",
                    stderr: "pipe",
                });

                await run.exited;

                const stdoutText = await new Response(run.stdout).text();
                const stderrText = await new Response(run.stderr).text();

                const stdout = stdoutText.trim().replace(/\r\n/g, "\n");
                const stderr = stderrText.trim().replace(/\r\n/g, "\n");
                const output = stripAnsi(`${stdout}\n${stderr}`.trim());

                resolve({ output, expected });
            } catch (e: any) {
                // Return the error rather than throwing it inside the limit() wrapper.
                // This ensures the promise rejects cleanly for the specific test case
                // instead of crashing the process or leaking.
                resolve({ error: e });
            } finally {
                // Ignore errors during cleanup (e.g., if file doesn't exist)
                try {
                    // await fs.unlink(outputFile);
                    await fs.unlink(exeFile);
                } catch {
                    // no-op
                }
            }
        }
    };

    // Start test promises with concurrency
    Promise.all(Array(CONCURRENCY).fill(0).map(worker));

    // Wait for promises one by one
    casePromises.forEach(({ case: caseItem, promise }) => {
        test(
            `should correctly interpret and run ${caseItem.name}.js`,
            async () => {
                // If the execution promise rejected, this await will throw the Error
                // created in the catch block above, failing ONLY this test.
                const { output, expected, error } = await promise;

                if (output && expected) {
                    for (const expectedString of expected!) {
                        expect(output).toInclude(expectedString);
                    }
                }

                if (error) {
                    error.message =
                        `Execution failed for ${caseItem.name}: ${error.message}`;
                    throw error;
                }
            },
            60000,
        );
    });

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

describe("Syntax Error tests", () => {
    test("should throw for unlabeled break outside a loop", () => {
        const interpreter = new Interpreter();
        const code = "break;";
        expect(() => interpreter.interpret(code)).toThrow(
            "SyntaxError: Unlabeled break must be inside an iteration or switch statement",
        );
    });

    test("should throw for unlabeled continue outside a loop", () => {
        const interpreter = new Interpreter();
        const code = "continue;";
        expect(() => interpreter.interpret(code)).toThrow(
            "SyntaxError: Unlabeled continue must be inside an iteration statement",
        );
    });

    test("should throw for break to an undefined label", () => {
        const interpreter = new Interpreter();
        const code = "outer: while(true) { break inner; }";
        expect(() => interpreter.interpret(code)).toThrow(
            "SyntaxError: Undefined label 'inner'",
        );
    });

    test("should throw for continue to an undefined label", () => {
        const interpreter = new Interpreter();
        const code = "outer: while(true) { continue inner; }";
        expect(() => interpreter.interpret(code)).toThrow(
            "SyntaxError: Undefined label 'inner'",
        );
    });

    // This case is valid in JS, so we should ensure it does NOT throw.
    test("should allow break from a labeled non-loop block", () => {
        const interpreter = new Interpreter();
        const code = "myLabel: { break myLabel; }";
        expect(() => interpreter.interpret(code)).not.toThrow();
    });
});
