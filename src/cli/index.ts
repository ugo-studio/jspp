#!/usr/bin/env node
import { spawn } from "child_process";
import fs from "fs/promises";
import path from "path";

import pkg from "../../package.json" with { type: "json" };
import { Interpreter } from "../index.js";
import { CompilerError } from "../interpreter/core/error.js";
import { parseArgs } from "./args.js";
import { COLORS } from "./colors.js";
import { Spinner } from "./spinner.js";
import { getLatestMtime, msToHumanReadable } from "./utils.js";

const pkgDir = path.dirname(path.dirname(import.meta.dirname));

async function main() {
    const { jsFilePath, isRelease, keepCpp, outputExePath, scriptArgs, target } =
        parseArgs(
            process.argv.slice(2),
        );

    const isWasm = target === "wasm";
    const ext = path.extname(jsFilePath);
    const jsFileName = path.basename(jsFilePath, ext);
    const sourceDir = path.dirname(jsFilePath);

    // Intermediate C++ file goes alongside the source JS file
    const cppFilePath = path.join(sourceDir, `${jsFileName}.cpp`);

    // Determine output executable path
    let exeFilePath: string;
    if (outputExePath) {
        exeFilePath = outputExePath;
    } else {
        if (isWasm) {
            exeFilePath = path.join(sourceDir, `${jsFileName}.js`);
        } else {
            const ext = process.platform === "win32" ? ".exe" : "";
            exeFilePath = path.join(sourceDir, `${jsFileName}${ext}`);
        }
    }

    // Mode Configuration
    const mode = isWasm ? "wasm" : (isRelease ? "release" : "debug");
    console.log(
        `${COLORS.bold}JSPP Compiler${COLORS.reset} ${COLORS.dim}v${pkg.version}${COLORS.reset}`,
    );
    console.log(
        `Target: ${isWasm ? COLORS.cyan : COLORS.green}${target.toUpperCase()}${COLORS.reset} | Mode: ${
            (isRelease || isWasm) ? COLORS.green : COLORS.yellow
        }${mode.toUpperCase()}${COLORS.reset}\n`,
    );

    const flags = (isRelease || isWasm) ? ["-O3", "-DNDEBUG"] : ["-Og", "-ftime-report"];

    if (isWasm) {
        flags.push("-sASYNCIFY", "-sALLOW_MEMORY_GROWTH=1", "-sWASM=1");
    } else if (process.platform === "win32") {
        flags.push("-Wa,-mbig-obj");
    }

    const pchDir = path.resolve(pkgDir, "prelude-build", mode);

    const spinner = new Spinner("Initializing...");

    try {
        if (isWasm) {
            spinner.text = "Checking Emscripten SDK...";
            spinner.start();
            // Ensure emsdk is set up
            const setupEmsdk = spawn("bun", ["run", "scripts/setup-emsdk.ts"], {
                cwd: pkgDir,
                stdio: ["ignore", "pipe", "pipe"],
            });
            const setupExitCode = await new Promise<number>((resolve) => {
                setupEmsdk.on("close", (code) => resolve(code ?? 1));
            });
            if (setupExitCode !== 0) {
                spinner.fail("Emscripten SDK setup failed");
                process.exit(1);
            }
            spinner.stop();
        }

        spinner.start();

        // 1. Interpreter Phase
        spinner.update(`Reading ${path.basename(jsFilePath)}...`);
        const jsCode = await fs.readFile(jsFilePath, "utf-8");

        spinner.update("Transpiling to C++...");
        const transpileStartTime = performance.now();
        const interpreter = new Interpreter();
        const { cppCode, preludePath } = interpreter.interpret(
            jsCode,
            jsFilePath,
        );
        const transpileTime = msToHumanReadable(
            performance.now() - transpileStartTime,
        );

        // Ensure directory for cpp file exists (should exist as it's source dir, but for safety if we change logic)
        await fs.mkdir(path.dirname(cppFilePath), { recursive: true });
        await fs.writeFile(cppFilePath, cppCode);
        spinner.succeed(
            `Generated cpp ${COLORS.dim}[${transpileTime}]${COLORS.reset}`,
        );

        // 2. Precompiled Header Check
        spinner.text = "Checking precompiled headers...";
        spinner.start();

        const pchFile = isWasm ? path.join(pchDir, "jspp.hpp") : path.join(pchDir, "jspp.hpp.gch");
        const runtimeLibPath = path.join(pchDir, "libjspp.a");
        let shouldRebuildPCH = false;

        try {
            const pchStats = await fs.stat(pchFile);
            const libStats = await fs.stat(runtimeLibPath);

            // 1. Check if any header is newer than the PCH
            const latestHeaderMtime = await getLatestMtime(
                preludePath,
                (name) => name.endsWith(".hpp") || name.endsWith(".h"),
            );

            // 2. Check if any CPP file is newer than the library
            const latestCppMtime = await getLatestMtime(
                preludePath,
                (name) => name.endsWith(".cpp"),
            );

            if (
                latestHeaderMtime > pchStats.mtimeMs ||
                latestCppMtime > libStats.mtimeMs ||
                pchStats.mtimeMs > libStats.mtimeMs
            ) {
                shouldRebuildPCH = true;
            }
        } catch (e) {
            shouldRebuildPCH = true;
        }

        if (shouldRebuildPCH) {
            spinner.update(
                "Rebuilding precompiled headers (this may take a while)...",
            );
            const pchStartTime = performance.now();

            const emsdkEnv = isWasm ? { ...process.env, PATH: `${path.join(pkgDir, ".emsdk")}${path.delimiter}${path.join(pkgDir, ".emsdk", "upstream", "emscripten")}${path.delimiter}${process.env.PATH}` } : process.env;

            // Use spawn (async) instead of spawnSync to keep spinner alive
            const rebuild = spawn("bun", [
                "run",
                "scripts/precompile-headers.ts",
                "--jspp-cli-is-parent",
            ], {
                cwd: pkgDir,
                stdio: ["ignore", "pipe", "pipe"],
                env: emsdkEnv,
            });

            if (rebuild.stdout) {
                rebuild.stdout.on("data", (chunk) => {
                    spinner.pause();
                    process.stdout.write(chunk);
                });
            }

            const stderrChunks: Buffer[] = [];
            if (rebuild.stderr) {
                rebuild.stderr.on("data", (chunk) => stderrChunks.push(chunk));
            }

            const exitCode = await new Promise<number>((resolve) => {
                rebuild.on("close", (code) => {
                    spinner.resume();
                    resolve(code ?? 1);
                });
            });

            if (exitCode !== 0) {
                const stderr = Buffer.concat(stderrChunks).toString();
                spinner.fail("Failed to rebuild precompiled headers");
                console.error(stderr);
                process.exit(1);
            }
            const pchTime = msToHumanReadable(performance.now() - pchStartTime);
            spinner.succeed(
                `Precompiled headers updated ${COLORS.dim}[${pchTime}]${COLORS.reset}`,
            );
        } else {
            spinner.stop();
        }

        // 3. Compilation Phase
        spinner.text = `Compiling binary...`;
        spinner.start();

        // Ensure output directory exists
        await fs.mkdir(path.dirname(exeFilePath), { recursive: true });

        const compiler = isWasm ? "em++" : "g++";
        const compileArgs = [
            "-std=c++23",
            ...flags,
            "-include",
            "jspp.hpp",
            cppFilePath,
            runtimeLibPath,
            "-o",
            exeFilePath,
            "-I",
            pchDir,
            "-I",
            preludePath,
        ];

        if (!isWasm) {
            compileArgs.splice(1, 0, "-Winvalid-pch");
        }

        const emsdkEnv = isWasm ? { ...process.env, PATH: `${path.join(pkgDir, ".emsdk")}${path.delimiter}${path.join(pkgDir, ".emsdk", "upstream", "emscripten")}${path.delimiter}${process.env.PATH}` } : process.env;

        const compileStartTime = performance.now();
        const compile = spawn(
            compiler,
            compileArgs,
            {
                stdio: ["ignore", "pipe", "pipe"],
                env: emsdkEnv,
                shell: process.platform === "win32",
            },
        );

        const compileStderrChunks: Buffer[] = [];
        if (compile.stderr) {
            compile.stderr.on(
                "data",
                (chunk) => compileStderrChunks.push(chunk),
            );
        }

        const compileExitCode = await new Promise<number>((resolve) => {
            compile.on("close", (code) => resolve(code ?? 1));
        });

        if (compileExitCode !== 0) {
            const stderr = Buffer.concat(compileStderrChunks).toString();
            spinner.fail(`Compilation failed`);
            console.error(stderr);
            process.exit(1);
        }

        const compileEndTime = performance.now();
        const compileTime = msToHumanReadable(
            compileEndTime - compileStartTime,
        );

        spinner.succeed(
            `Compiled to ${COLORS.green}${COLORS.bold}${
                path.basename(exeFilePath)
            }${COLORS.reset} ${COLORS.dim}[${compileTime}]${COLORS.reset}`,
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
        if (isWasm) {
            console.log(`\n${COLORS.cyan}Compilation finished. To run the output, use node or a browser:${COLORS.reset}`);
            console.log(`${COLORS.bold}    node ${path.basename(exeFilePath)}${COLORS.reset}\n`);
        } else {
            console.log(`\n${COLORS.cyan}--- Running Output ---${COLORS.reset}`);
            const run = spawn(exeFilePath, scriptArgs, {
                stdio: "inherit",
            });

            const runExitCode = await new Promise<number>((resolve) => {
                run.on("close", (code) => resolve(code ?? 1));
            });
            console.log(`${COLORS.cyan}----------------------${COLORS.reset}\n`);

            if (runExitCode !== 0) {
                console.error(
                    `${COLORS.red}Execution failed with exit code ${runExitCode}${COLORS.reset}`,
                );
                process.exit(1);
            }
        }
    } catch (error: any) {
        if (error instanceof CompilerError) {
            spinner.fail("Compilation failed");
            console.error(error.getFormattedError());
            process.exit(1);
        }
        spinner.fail("An unexpected error occurred");
        console.error(error);
        process.exit(1);
    }
}

main();
