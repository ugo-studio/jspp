import { spawn, spawnSync } from "child_process";
import { describe, expect, test } from "bun:test";
import fs from "fs/promises";
import path from "path";

import { Interpreter } from "../src";
import cases from "./expected-results.json";

const pkgDir = path.dirname(__dirname);

// --- Helper: Strip ANSI Codes ---
const stripAnsi = (str: string) =>
    str.replace(
        /[\u001b\u009b][[()#;?]*(?:[0-9]{1,4}(?:;[0-9]{0,4})*)?[0-9A-ORZcf-nqry=><]/g,
        "",
    );

const ensureHeaders = () => {
    console.log("Ensuring precompiled headers are ready...");
    const precompile = spawnSync("bun", ["run", "scripts/precompile-headers.ts"], {
        cwd: pkgDir,
        stdio: "inherit",
    });
    if (precompile.status !== 0) {
        throw new Error("Failed to precompile headers for tests.");
    }
};

describe("Interpreter tests", async () => {
    let jsCode = "const caseName = process.argv[2];\n";
    let isFirstCase = true;

    for (const caseItem of cases) {
        const caseName = caseItem.name;
        const inputFile = path.join(
            pkgDir,
            "test",
            "cases",
            `${caseName}.js`,
        );

        if (isFirstCase) {
            isFirstCase = false;
        } else {
            jsCode += "else ";
        }
        jsCode += `if (caseName === "${caseName}") {{\n`;
        try {
            jsCode += `${await fs.readFile(inputFile, "utf-8")}\n`;
        } catch (e: any) {
            jsCode +=
                `throw new Error("Error reading test file: ${e.message}");\n`;
        }
        jsCode += "}}\n";
    }

    // Prepare output file paths
    const outputFile = path.join(
        pkgDir,
        "test",
        "output",
        `out.cpp`,
    );
    const exeFile = path.join(
        pkgDir,
        "test",
        "output",
        `out.exe`,
    );

    // Transpile to c++
    const interpreter = new Interpreter();
    const { cppCode, preludePath } = interpreter.interpret(jsCode);

    await fs.mkdir(path.dirname(outputFile), {
        recursive: true,
    });
    await fs.writeFile(outputFile, cppCode);
    console.log("C++ code written to", outputFile);

    // Ensure that the precompiled headers are ready
    ensureHeaders();

    // Compile c++ code
    console.log("Compiling C++ code...");
    const compileCmd = [
        "-O0",
        "-std=c++23",
        outputFile,
        "-o",
        exeFile,
        "-I",
        path.resolve(pkgDir, "prelude-build", "debug"),
        "-I",
        preludePath,
    ];

    if (process.platform === "win32") {
        compileCmd.splice(1, 0, "-Wa,-mbig-obj");
    }

    const compile = spawn(
        "g++",
        compileCmd,
        {
            cwd: pkgDir,
            stdio: ["ignore", "pipe", "pipe"],
        },
    );

    const compileStdoutChunks: Buffer[] = [];
    const compileStderrChunks: Buffer[] = [];
    if (compile.stdout) compile.stdout.on("data", c => compileStdoutChunks.push(c));
    if (compile.stderr) compile.stderr.on("data", c => compileStderrChunks.push(c));

    const compileExitCode = await new Promise<number>((resolve) => {
        compile.on("close", (code) => resolve(code ?? 1));
    });

    if (compileExitCode !== 0) {
        const stderr = Buffer.concat(compileStderrChunks).toString();
        const stdout = Buffer.concat(compileStdoutChunks).toString();
        throw new Error(
            `C++ compilation failed for testsuite: ${stderr}\nSTDOUT: ${stdout}`,
        );
    } else console.log("C++ code compiled to", exeFile);

    // Run tests
    for (const caseItem of cases) {
        const caseName = caseItem.name;
        const expected = caseItem.expected;

        test(
            `should correctly interpret and run ${caseName}.js`,
            async () => {
                const run = spawn(exeFile, [caseName], {
                    cwd: pkgDir,
                    stdio: ["ignore", "pipe", "pipe"],
                });

                const runStdoutChunks: Buffer[] = [];
                const runStderrChunks: Buffer[] = [];
                if (run.stdout) run.stdout.on("data", c => runStdoutChunks.push(c));
                if (run.stderr) run.stderr.on("data", c => runStderrChunks.push(c));

                await new Promise<number>((resolve) => {
                    run.on("close", (code) => resolve(code ?? 1));
                });

                const stdoutText = Buffer.concat(runStdoutChunks).toString();
                const stderrText = Buffer.concat(runStderrChunks).toString();

                const stdout = stdoutText.trim().replace(/\r\n/g, "\n");
                const stderr = stderrText.trim().replace(/\r\n/g, "\n");
                const output = stripAnsi(`${stdout}\n${stderr}`.trim());

                for (const expectedString of expected) {
                    expect(output).toInclude(expectedString);
                }
            },
            { timeout: 20000 },
        );
    }

    test("should throw an error for reserved-keyword.js", async () => {
        const inputFile = path.join(
            pkgDir,
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
