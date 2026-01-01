#!/usr/bin/env bun
import fs from "fs/promises";
import path from "path";

import { Interpreter } from "./index";

async function main() {
    const rawArgs = process.argv.slice(2);
    
    // Parse flags
    const isRelease = rawArgs.includes("--release");
    const args = rawArgs.filter(arg => arg !== "--release");

    if (args.length === 0) {
        console.log("Usage: jspp <path-to-js-file> [--release]");
        process.exit(1);
    }

    const jsFilePath = path.resolve(process.cwd(), args[0]!);
    const jsFileName = path.basename(jsFilePath, ".js");
    const outputDir = path.dirname(jsFilePath);
    const cppFilePath = path.join(outputDir, `${jsFileName}.cpp`);
    const exeFilePath = path.join(outputDir, `${jsFileName}.exe`);

    // Mode Configuration
    const mode = isRelease ? "release" : "debug";
    console.log(`Mode: ${mode.toUpperCase()}`);

    const flags = isRelease 
        ? ["-O3", "-DNDEBUG", "-Wa,-mbig-obj"]
        : ["-O0", "-Wa,-mbig-obj"];
    
    const pchDir = path.resolve(process.cwd(), "prelude-build", mode);

    try {
        const jsCode = await fs.readFile(jsFilePath, "utf-8");
        const interpreter = new Interpreter();

        const { cppCode, preludePath } = interpreter.interpret(jsCode);
        console.log(`Generated C++ code ${cppFilePath}...`);

        await fs.mkdir(outputDir, { recursive: true });
        await fs.writeFile(cppFilePath, cppCode);

        console.log(`Compiling ${cppFilePath}...`);
        const compile = Bun.spawnSync({
            cmd: [
                "g++",
                "-std=c++23",
                ...flags,
                cppFilePath,
                "-o",
                exeFilePath,
                "-I",
                pchDir,
                "-I",
                preludePath,
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
        console.error(error);
        process.exit(1);
    }
}

main();
