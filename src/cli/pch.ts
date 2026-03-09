import { spawn } from "child_process";
import fs from "fs/promises";
import path from "path";
import { COLORS } from "./colors.js";
import { Spinner } from "./spinner.js";
import { getLatestMtime, msToHumanReadable } from "./utils.js";

export async function checkAndRebuildPCH(
    pkgDir: string,
    pchDir: string,
    mode: string,
    preludePath: string,
    emsdkEnv: NodeJS.ProcessEnv,
    spinner: Spinner,
) {
    spinner.text = "Checking precompiled headers...";
    spinner.start();

    const pchFile = path.join(pchDir, "jspp.hpp.gch");
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

        const rebuild = spawn("bun", [
            "run",
            "scripts/precompile-headers.ts",
            "--jspp-cli-is-parent",
            "--mode",
            mode,
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
}
