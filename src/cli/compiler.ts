import { spawn } from "child_process";
import fs from "fs/promises";
import path from "path";
import { COLORS } from "./colors.js";
import { Spinner } from "./spinner.js";
import { msToHumanReadable } from "./utils.js";

export async function compileCpp(
    cppFilePath: string,
    exeFilePath: string,
    pchDir: string,
    preludePath: string,
    isWasm: boolean,
    flags: string[],
    emsdkEnv: NodeJS.ProcessEnv,
    spinner: Spinner,
) {
    spinner.text = `Compiling binary...`;
    spinner.start();

    // Ensure output directory exists
    await fs.mkdir(path.dirname(exeFilePath), { recursive: true });

    const runtimeLibPath = path.join(pchDir, "libjspp.a");
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
}
