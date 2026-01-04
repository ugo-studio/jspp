import { spawnSync } from "child_process";
import fs from "fs/promises";
import path from "path";

const PRELUDE_DIR = path.resolve(process.cwd(), "src", "prelude");
const PRECOMPILED_HEADER_BASE_DIR = path.resolve(process.cwd(), "prelude-build");

const MODES = [
    {
        name: "debug",
        flags: ["-O0"],
    },
    {
        name: "release",
        flags: ["-O3", "-DNDEBUG"],
    },
];

if (process.platform === "win32") {
    MODES[0].flags.push("-Wa,-mbig-obj");
    MODES[1].flags.push("-Wa,-mbig-obj");
}

async function getLatestMtime(dirPath: string): Promise<number> {
    let maxMtime = 0;
    const entries = await fs.readdir(dirPath, { withFileTypes: true });
    for (const entry of entries) {
        const fullPath = path.join(dirPath, entry.name);
        if (entry.isDirectory()) {
            const nestedMtime = await getLatestMtime(fullPath);
            if (nestedMtime > maxMtime) maxMtime = nestedMtime;
        } else {
            const stats = await fs.stat(fullPath);
            if (stats.mtimeMs > maxMtime) maxMtime = stats.mtimeMs;
        }
    }
    return maxMtime;
}

async function precompileHeaders() {
    const force = process.argv.includes("--force");
    try {
        await fs.mkdir(PRECOMPILED_HEADER_BASE_DIR, { recursive: true });
        const sourceMtime = await getLatestMtime(PRELUDE_DIR);

        for (const mode of MODES) {
            const modeDir = path.join(PRECOMPILED_HEADER_BASE_DIR, mode.name);
            const headerPath = path.join(modeDir, "index.hpp");
            const gchPath = path.join(modeDir, "index.hpp.gch");

            if (!force) {
                try {
                    const gchStats = await fs.stat(gchPath);
                    if (gchStats.mtimeMs >= sourceMtime) {
                        console.log(`[${mode.name.toUpperCase()}] Headers are up-to-date. Skipping.`);
                        continue;
                    }
                } catch (e) {
                    // PCH doesn't exist, proceed to compile
                }
            }

            console.log(`\n[${mode.name.toUpperCase()}] Setting up...`);
            await fs.mkdir(modeDir, { recursive: true });

            // Copy index.hpp
            await fs.copyFile(path.join(PRELUDE_DIR, "index.hpp"), headerPath);

            // Remove existing gch if it exists
            if (
                await fs.stat(gchPath).then(
                    () => true,
                    () => false,
                )
            ) {
                await fs.unlink(gchPath);
            }

            console.log(`[${mode.name.toUpperCase()}] Compiling header...`);
            const compile = spawnSync(
                "g++",
                [
                    "-x",
                    "c++-header",
                    "-std=c++23",
                    ...mode.flags,
                    headerPath,
                    "-o",
                    gchPath,
                    "-I",
                    PRELUDE_DIR,
                ],
                {
                    stdio: "inherit",
                }
            );

            if (compile.status !== 0) {
                console.error(
                    `[${mode.name.toUpperCase()}] Failed to precompile headers.`,
                );
                process.exit(1);
            }
            console.log(`[${mode.name.toUpperCase()}] Success.`);
        }
    } catch (error: any) {
        console.error(`Error: ${error.message}`);
        process.exit(1);
    }
}

precompileHeaders();
