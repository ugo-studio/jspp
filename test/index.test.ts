import os from "node:os";

import { describe, expect, test } from "bun:test";
import fs from "fs/promises";
import path from "path";

import { Interpreter } from "../src";

// --- Configuration ---
// Dynamically set concurrency based on the number of logical CPU cores
// Using Math.max to ensure at least 1 core is used even on small VMs
const CONCURRENCY = Math.max(1, Math.floor(os.cpus().length / 2));

// --- Helper: Concurrency Limiter ---
const pLimit = (concurrency: number) => {
    let active = 0;
    const queue: (() => void)[] = [];

    return <T>(fn: () => Promise<T>): Promise<T> => {
        return new Promise((resolve, reject) => {
            const run = async () => {
                active++;
                try {
                    resolve(await fn());
                } catch (e) {
                    reject(e);
                } finally {
                    active--;
                    if (queue.length > 0) {
                        queue.shift()!();
                    }
                }
            };

            if (active < concurrency) {
                run();
            } else {
                queue.push(run);
            }
        });
    };
};

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
            "undefined",
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
        expected: [
            "--- Log function ---",
            "[Function: myFunc]",
            "[Function]",
        ],
    },
    {
        name: "recursion",
        expected: [
            "--- Recursion ---",
            "Factorial of 50: 3.0414e+64",
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
    {
        name: "array-length",
        expected: [
            "--- Array Length ---",
            "3",
            "5",
            "undefined",
            "undefined",
            "2",
            "undefined",
            "Caught expected error: RangeError: Invalid array length",
        ],
    },
    {
        name: "template-strings",
        expected: [
            "--- Template Strings ---",
            "hello world",
            "a + b = 3",
            "this is a multi-line\ntemplate string.",
            "obj.val = nested",
            "12",
        ],
    },
    {
        name: "equality",
        expected: [
            "--- Equality ---",
            "5 == '5' true",
            "5 === '5' false",
            "5 == 5 true",
            "5 === 5 true",
            "null == undefined true",
            "null === undefined false",
            "0 == false true",
            "0 === false false",
            "'' == false true",
            "'' === false false",
            "{} == {} false",
            "{} === {} false",
            "obj1 == obj3 true",
            "obj1 === obj3 true",
        ],
    },
    {
        name: "errors",
        expected: [
            "--- Errors ---",
            "Error: my message",
            "a string error",
            "123",
            "CustomError: m",
        ],
    },
    {
        name: "for-loops",
        expected: [
            "--- For Loops ---",
            "- C-style for loop -",
            "0",
            "1",
            "2",
            "3",
            "4",
            "- for...in loop -",
            "a 1",
            "b 2",
            "- for...of loop -",
            "10",
            "20",
            "30",
        ],
    },
    {
        name: "unresolved-property-access",
        expected: [
            "--- Unresolved Property Access ---",
            "Caught expected error: ReferenceError: a is not defined",
        ],
    },
    {
        name: "string-methods",
        expected: [
            "--- String Methods ---",
            "Original: '  Hello, World!  '",
            "charAt: l",
            "concat:   Hello, World!   How are you?",
            "endsWith: true",
            "includes: true",
            "indexOf: 6",
            "lastIndexOf: 10",
            "padEnd:   Hello, World!  ........",
            "padStart: ........  Hello, World!  ",
            "repeat: abcabcabc",
            "replace:   Hello, JSPP!  ",
            "replaceAll: b b b",
            "slice: Hello",
            'split: [ "  Hello", " World!  " ]',
            "startsWith: true",
            "substring: Hello",
            "toLowerCase:   hello, world!  ",
            "toUpperCase:   HELLO, WORLD!  ",
            "trim: 'Hello, World!'",
            "trimEnd: '  Hello, World!'",
            "trimStart: 'Hello, World!  '",
        ],
    },
    {
        name: "operators",
        expected: [
            "--- Operators ---",
            "a: 10 b: 5",
            "a + b = 15",
            "a - b = 5",
            "a * b = 50",
            "a / b = 2",
            "a % b = 0",
            "a ** 2 = 100",
            "a++: 11",
            "b--: 4",
            "++a: 12",
            "--b: 3",
            "a += 5: 17",
            "a -= 5: 12",
            "a *= 2: 24",
            "a /= 2: 12",
            "a %= 3: 0",
            "a > b: false",
            "a < b: true",
            "a >= b: false",
            "a <= b: true",
            "a != b: true",
            "c & d: 1",
            "c | d: 7",
            "c ^ d: 6",
            "~c: -6",
            "c << 1: 10",
            "c >> 1: 2",
        ],
    },
    {
        name: "while-loops",
        expected: [
            "--- While Loops ---",
            "0",
            "1",
            "2",
            "3",
            "4",
            "--- While loop with break ---",
            "0",
            "1",
            "2",
        ],
    },
    {
        name: "do-while-loops",
        expected: [
            "--- Do-While Loops ---",
            "0",
            "1",
            "2",
            "3",
            "4",
            "--- Do-While loop that runs once ---",
            "5",
        ],
    },
    {
        name: "labeled-statements",
        expected: [
            "--- Labeled Statements ---",
            "--- Labeled break ---",
            "Outer loop: 0",
            "Inner loop: 0",
            "Inner loop: 1",
            "Inner loop: 2",
            "Outer loop: 1",
            "Inner loop: 0",
            "Inner loop: 1",
            "Breaking outer loop",
            "After labeled break. x is 2",
            "--- Labeled continue ---",
            "i=0, j=0",
            "i=0, j=1",
            "i=0, j=2",
            "i=1, j=0",
            "continuing outer loop at i=1, j=1",
            "i=2, j=0",
            "i=2, j=1",
            "i=2, j=2",
            "After labeled continue",
        ],
    },
    {
        name: "switch-statements",
        expected: [
            "--- Switch Statements ---",
            "Case 1",
            "Case 2",
            "Default case",
            "Case 'hello'",
            "Case true",
            "Default case",
            "Default case",
            "Default case",
            "--- Switch with Fallthrough ---",
            "Fallthrough 1",
            "Fallthrough 2",
            "Fallthrough 2",
            "Fallthrough 3",
            "Fallthrough Default",
            "Fallthrough Default",
            "--- Switch with no matches and no default ---",
            "--- Switch in a loop ---",
            "Loop Case 0",
            "After switch, loop i: 0",
            "Loop Case 1",
            "Loop Case 2",
            "After switch, loop i: 2",
            "--- Labeled Switch ---",
            "Outer Switch Case 1",
            "Inner Switch Case 'a'",
            "After labeled outer switch",
            "--- Switch with complex expressions ---",
            "Sum is 30",
            "Object property matched"
        ]
    },
    {
        name: "generators",
        expected: [
            "--- Generators ---",
            "- for...of on generator call -",
            "1",
            "2",
            "3",
            "- Iterator protocol -",
            "1",
            "2",
            "3",
            "true",
            "- Generator with return -",
            "a",
            "b",
            "true",
            "- Iterable Iterator -",
            "from iter: 1",
            "from iter: 2",
            "from iter: 3",
            "- Array Iterator -",
            "10",
            "20",
            "true",
            "- Generator with input -",
            "start",
            "1",
            "received a: value-for-a",
            "2",
            "received b: value-for-b",
            "3",
        ],
    },
    {
        name: "new-operator",
        expected: [
            "--- New Operator ---",
            "Hello, World",
            "World"
        ],
    },
    {
        name: "classes",
        expected: [
            "--- Classes ---",
            "Dog makes a noise.",
            "Rex barks.",
            "Rex is a German Shepherd",
            "--- Static Methods ---",
            "8"
        ],
    },
    {
        name: "methods-and-accessors",
        expected: [
            "--- Object Methods & Computed Keys ---",
            "a",
            "b",
            "c",
            "d",
            "--- Object Accessors & Computed Keys ---",
            "1 2",
            "20 30",
            "20 30",
            "--- Class Computed Methods ---",
            "static dynamic",
            "instance dynamic",
            "--- Class Accessors ---",
            "Class getter: 100",
            "Class getter after set: 200",
            "Static getter: static",
        ],
    }
];

const stripAnsi = (str: string) =>
    str.replace(
        /[\u001b\u009b][[()#;?]*(?:[0-9]{1,4}(?:;[0-9]{0,4})*)?[0-9A-ORZcf-nqry=><]/g,
        "",
    );

describe("Interpreter tests", () => {
    const limit = pLimit(CONCURRENCY);

    const executions = cases.map(({ name: caseName, expected }) =>
        limit(async () => {
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
                    throw new Error(
                        `C++ compilation failed for ${caseName}: ${stderr}`,
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

                return { output, expected };
            } catch (e: any) {
                // Return the error rather than throwing it inside the limit() wrapper.
                // This ensures the promise rejects cleanly for the specific test case
                // instead of crashing the process or leaking.
                throw new Error(
                    `Execution failed for ${caseName}: ${e.message}`,
                );
            } finally {
                // Ignore errors during cleanup (e.g., if file doesn't exist)
                try {
                    // await fs.unlink(outputFile);
                    await fs.unlink(exeFile);
                } catch {
                    // no-op
                }
            }
        })
    );

    cases.forEach((caseItem, index) => {
        test(
            `should correctly interpret and run ${caseItem.name}.js`,
            async () => {
                // If the execution promise rejected, this await will throw the Error
                // created in the catch block above, failing ONLY this test.
                const { output, expected } = await executions[index];

                for (const expectedString of expected) {
                    expect(output).toInclude(expectedString);
                }
            },
            25000,
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
            "SyntaxError: Unlabeled break must be inside an iteration or switch statement"
        );
    });

    test("should throw for unlabeled continue outside a loop", () => {
        const interpreter = new Interpreter();
        const code = "continue;";
        expect(() => interpreter.interpret(code)).toThrow(
            "SyntaxError: Unlabeled continue must be inside an iteration statement"
        );
    });

    test("should throw for break to an undefined label", () => {
        const interpreter = new Interpreter();
        const code = "outer: while(true) { break inner; }";
        expect(() => interpreter.interpret(code)).toThrow(
            "SyntaxError: Undefined label 'inner'"
        );
    });

    test("should throw for continue to an undefined label", () => {
        const interpreter = new Interpreter();
        const code = "outer: while(true) { continue inner; }";
        expect(() => interpreter.interpret(code)).toThrow(
            "SyntaxError: Undefined label 'inner'"
        );
    });

    // This case is valid in JS, so we should ensure it does NOT throw.
    test("should allow break from a labeled non-loop block", () => {
        const interpreter = new Interpreter();
        const code = "myLabel: { break myLabel; }";
        expect(() => interpreter.interpret(code)).not.toThrow();
    });
});
