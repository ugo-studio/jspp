import fs from "fs/promises";
import path from "path";

const PRELUDE_DIR = path.resolve(process.cwd(), "src", "prelude");
const PRECOMPILED_HEADER_BASE_DIR = path.resolve(process.cwd(), "prelude-build");

const MODES = [
    {
        name: "debug",
        flags: ["-O0", "-Wa,-mbig-obj"],
    },
    {
        name: "release",
        flags: ["-O3", "-DNDEBUG", "-Wa,-mbig-obj"],
    },
];

async function precompileHeaders() {
    try {
        await fs.mkdir(PRECOMPILED_HEADER_BASE_DIR, { recursive: true });

        for (const mode of MODES) {
            const modeDir = path.join(PRECOMPILED_HEADER_BASE_DIR, mode.name);
            const headerPath = path.join(modeDir, "index.hpp");
            const gchPath = path.join(modeDir, "index.hpp.gch");

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
            const compile = Bun.spawnSync({
                cmd: [
                    "g++",
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
                stdout: "inherit",
                stderr: "inherit",
            });

            if (compile.exitCode !== 0) {
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
