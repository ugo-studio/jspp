#!/usr/bin/env node
import fs from "fs/promises";
import path from "path";

import pkg from "../../package.json" with { type: "json" };
import { CompilerError } from "../interpreter/core/error.js";
import { parseArgs } from "./args.js";
import { COLORS } from "./colors.js";
import { compileCpp } from "./compiler.js";
import { checkAndRebuildPCH } from "./pch.js";
import { runOutput } from "./runner.js";
import { Spinner } from "./spinner.js";
import { transpile } from "./transpiler.js";
import { postProcessWasm, setupEmsdk } from "./wasm.js";

const pkgDir = path.dirname(path.dirname(import.meta.dirname));
const emsdkEnv = {
    ...process.env,
    PATH: `${path.join(pkgDir, ".emsdk")}${path.delimiter}${
        path.join(pkgDir, ".emsdk", "upstream", "emscripten")
    }${path.delimiter}${process.env.PATH}`,
};

async function main() {
    const {
        jsFilePath,
        isRelease,
        keepCpp,
        outputExePath,
        scriptArgs,
        target,
    } = parseArgs(
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
    const modeNote = isRelease
        ? `${COLORS.dim}(optimized)${COLORS.reset}`
        : `${COLORS.dim}(debug)${COLORS.reset}\n${COLORS.dim}NOTE: Use "--release" for an optimized output for production${COLORS.reset}`;
    
    console.log(
        `${COLORS.bold}JSPP Compiler${COLORS.reset} ${COLORS.dim}v${pkg.version}${COLORS.reset}`,
    );
    console.log(
        `Target: ${
            isWasm ? COLORS.cyan : COLORS.green
        }${target.toUpperCase()}${COLORS.reset} | Mode: ${
            isRelease ? COLORS.green : COLORS.yellow
        }${mode.toUpperCase()}${COLORS.reset} ${modeNote}\n`,
    );

    const flags = isRelease ? ["-O3", "-DNDEBUG"] : ["-Og"];

    if (isWasm) {
        flags.push(
            "-sASYNCIFY",
            "-sALLOW_MEMORY_GROWTH=1",
            "-sWASM=1",
            "-sEXPORT_ES6=1",
            "-sMODULARIZE=1",
            "-sEXPORT_NAME=jsppModule",
        );
    } else if (process.platform === "win32") {
        flags.push("-Wa,-mbig-obj");
    }

    const pchDir = path.resolve(pkgDir, "prelude-build", mode);
    const spinner = new Spinner("Initializing...");

    try {
        if (isWasm) {
            await setupEmsdk(pkgDir, spinner);
        }

        spinner.start();

        // 1. Transpilation Phase
        const { preludePath, wasmExports } = await transpile(
            jsFilePath,
            cppFilePath,
            target as "native" | "wasm",
            spinner,
        );

        // 2. Precompiled Header Check
        await checkAndRebuildPCH(
            pkgDir,
            pchDir,
            mode,
            preludePath,
            emsdkEnv,
            spinner,
        );

        // 3. Compilation Phase
        await compileCpp(
            cppFilePath,
            exeFilePath,
            pchDir,
            preludePath,
            isWasm,
            flags,
            emsdkEnv,
            spinner,
        );

        // 3.5 Post-processing for Wasm (Exports)
        if (isWasm) {
            await postProcessWasm(exeFilePath, wasmExports);
        }

        // Clean up C++ file if not requested to keep
        if (!keepCpp) {
            try {
                await fs.unlink(cppFilePath);
            } catch (e) {
                // Ignore error if file cannot be deleted
            }
        }

        // 4. Execution Phase
        await runOutput(exeFilePath, scriptArgs, isWasm);

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
