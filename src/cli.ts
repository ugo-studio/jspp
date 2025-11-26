#!/usr/bin/env bun
import fs from "fs/promises";
import path from "path";

import { Interpreter } from "./index";

async function main() {
    const args = process.argv.slice(2);
    if (args.length === 0) {
        console.log("Usage: jspp <path-to-js-file>");
        process.exit(1);
    }

    const jsFilePath = path.resolve(process.cwd(), args[0]!);
    const jsFileName = path.basename(jsFilePath, ".js");
    const outputDir = path.dirname(jsFilePath);
    const cppFilePath = path.join(outputDir, `${jsFileName}.cpp`);
    const exeFilePath = path.join(outputDir, `${jsFileName}.exe`);

    try {
        const jsCode = await fs.readFile(jsFilePath, "utf-8");
        const interpreter = new Interpreter();

        console.time(`Generated C++ code ${cppFilePath}...`);
        const { cppCode, preludePath } = interpreter.interpret(jsCode);
        console.timeEnd(`Generated C++ code ${cppFilePath}...`);

        await fs.mkdir(outputDir, { recursive: true });
        await fs.writeFile(cppFilePath, cppCode);

        console.log(`Compiling ${cppFilePath}...`);
        const compile = Bun.spawnSync({
            cmd: [
                "g++",
                "-std=c++23",
                cppFilePath,
                "-o",
                exeFilePath,
                "-I",
                preludePath,
                "-O3",
                "-DNDEBUG",
                // "-include",
                // path.join(process.cwd(), "prelude-build", "index.hpp"),
            ],
            stdout: "inherit",
            stderr: "inherit",
        });

        if (compile.exitCode !== 0) {
            console.error(`Compilation failed for ${cppFilePath}`);
            process.exit(1);
        }

        console.log(`Running ${exeFilePath}...`);
        console.log("\x1b[32m\n------start------\x1b[0m");
        const run = Bun.spawnSync({
            cmd: [exeFilePath],
            stdout: "inherit",
            stderr: "inherit",
        });
        console.log("\x1b[32m\n------end--------\x1b[0m");

        if (run.exitCode !== 0) {
            console.error(`Execution failed for ${exeFilePath}`);
            process.exit(1);
        }

        console.log(`Successfully ran ${exeFilePath}`);
    } catch (error: any) {
        console.error(`Error: ${error.message}`);
        process.exit(1);
    }
}

main();
