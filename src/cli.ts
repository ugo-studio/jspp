#!/usr/bin/env bun
import fs from "fs/promises";
import path from "path";

import pkg from "../package.json";
import { parseArgs } from "./cli-utils/args";
import { COLORS } from "./cli-utils/colors";
import { getLatestMtime } from "./cli-utils/file-utils";
import { Spinner } from "./cli-utils/spinner";
import { Interpreter } from "./index";

async function main() {
    const { jsFilePath, isRelease, keepCpp, outputExePath, scriptArgs } = parseArgs(
        process.argv.slice(2),
    );

    const jsFileName = path.basename(jsFilePath, ".js");
    const sourceDir = path.dirname(jsFilePath);

    // Intermediate C++ file goes alongside the source JS file
    const cppFilePath = path.join(sourceDir, `${jsFileName}.cpp`);

    // Determine output executable path
    let exeFilePath: string;
    if (outputExePath) {
        exeFilePath = outputExePath;
    } else {
        const ext = process.platform === "win32" ? ".exe" : "";
        exeFilePath = path.join(sourceDir, `${jsFileName}${ext}`);
    }

    // Mode Configuration
    const mode = isRelease ? "release" : "debug";
    console.log(
        `${COLORS.bold}JSPP Compiler${COLORS.reset} ${COLORS.dim}v${pkg.version}${COLORS.reset}`,
    );
    console.log(
        `Mode: ${
            isRelease ? COLORS.green : COLORS.yellow
        }${mode.toUpperCase()}${COLORS.reset}\n`,
    );

    const flags = isRelease
        ? ["-O3", "-DNDEBUG", "-Wa,-mbig-obj"]
        : ["-O0", "-Wa,-mbig-obj"];

    const pchDir = path.resolve(process.cwd(), "prelude-build", mode);

    const spinner = new Spinner("Initializing...");

    try {
        spinner.start();

        // 1. Interpreter Phase
        spinner.update(`Reading ${path.basename(jsFilePath)}...`);
        const jsCode = await fs.readFile(jsFilePath, "utf-8");

        spinner.update("Transpiling to C++...");
        const interpreter = new Interpreter();
        const { cppCode, preludePath } = interpreter.interpret(jsCode);

        // Ensure directory for cpp file exists (should exist as it's source dir, but for safety if we change logic)
        await fs.mkdir(path.dirname(cppFilePath), { recursive: true });
        await fs.writeFile(cppFilePath, cppCode);
        spinner.succeed(
            `Generated ${COLORS.dim}${
                path.basename(cppFilePath)
            }${COLORS.reset}`,
        );

        // 2. Precompiled Header Check
        spinner.text = "Checking precompiled headers...";
        spinner.start();

        const pchFile = path.join(pchDir, "index.hpp.gch");
        let shouldRebuild = false;
        try {
            const pchStats = await fs.stat(pchFile);
            const sourceMtime = await getLatestMtime(preludePath);
            if (sourceMtime > pchStats.mtimeMs) {
                shouldRebuild = true;
            }
        } catch (e) {
            shouldRebuild = true;
        }

        if (shouldRebuild) {
            spinner.update(
                "Rebuilding precompiled headers (this may take a while)...",
            );
            // Use spawn (async) instead of spawnSync to keep spinner alive
            const rebuild = Bun.spawn({
                cmd: ["bun", "run", "scripts/precompile-headers.ts"],
                stdout: "pipe", // pipe to hide output unless error, or handle differently
                stderr: "pipe",
            });

            const exitCode = await rebuild.exited;

            if (exitCode !== 0) {
                const stderr = await new Response(rebuild.stderr).text();
                spinner.fail("Failed to rebuild precompiled headers");
                console.error(stderr);
                process.exit(1);
            }
            spinner.succeed("Precompiled headers updated");
        } else {
            spinner.succeed("Precompiled headers up-to-date");
        }

        // 3. Compilation Phase
        spinner.text = `Compiling binary...`;
        spinner.start();

        // Ensure output directory exists
        await fs.mkdir(path.dirname(exeFilePath), { recursive: true });

        const compile = Bun.spawn({
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
            stdout: "pipe",
            stderr: "pipe",
        });

        const compileExitCode = await compile.exited;

        if (compileExitCode !== 0) {
            const stderr = await new Response(compile.stderr).text();
            spinner.fail(`Compilation failed`);
            console.error(stderr);
            process.exit(1);
        }
        spinner.succeed(
            `Compiled to ${COLORS.green}${COLORS.bold}${
                path.basename(exeFilePath)
            }${COLORS.reset}`,
        );

        // Clean up C++ file if not requested to keep
        if (!keepCpp) {
            try {
                await fs.unlink(cppFilePath);
            } catch (e) {
                // Ignore error if file cannot be deleted
            }
        }

        // 4. Execution Phase
        console.log(`\n${COLORS.cyan}--- Running Output ---${COLORS.reset}`);
        const run = Bun.spawn({
            cmd: [exeFilePath, ...scriptArgs],
            stdout: "inherit",
            stderr: "inherit",
        });

        const runExitCode = await run.exited;
        console.log(`${COLORS.cyan}----------------------${COLORS.reset}\n`);

        if (runExitCode !== 0) {
            console.error(
                `${COLORS.red}Execution failed with exit code ${runExitCode}${COLORS.reset}`,
            );
            process.exit(1);
        }
    } catch (error: any) {
        spinner.fail("An unexpected error occurred");
        console.error(error);
        process.exit(1);
    }
}

main();
