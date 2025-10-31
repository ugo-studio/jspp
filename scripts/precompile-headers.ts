import fs from "fs/promises";
import path from "path";

const PRELUDE_DIR = path.resolve(process.cwd(), "src", "prelude");
const PRECOMPILED_HEADER_DIR = path.resolve(process.cwd(), "prelude-build");
const PRECOMPILED_HEADER_PATH = path.join(
    PRECOMPILED_HEADER_DIR,
    "index.hpp.gch",
);

async function precompileHeaders() {
    try {
        console.log("Checking for existing precompiled header...");
        await fs.access(PRECOMPILED_HEADER_PATH);
        console.log("Precompiled header already exists. Skipping.");
        return;
    } catch (error) {
        // Precompiled header doesn't exist, so we'll create it.
    }

    try {
        console.log("Precompiling prelude headers...");

        await fs.mkdir(PRECOMPILED_HEADER_DIR, { recursive: true });

        const compile = Bun.spawnSync({
            cmd: [
                "g++",
                "-x",
                "c++-header",
                path.join(PRELUDE_DIR, "index.hpp"),
                "-o",
                PRECOMPILED_HEADER_PATH,
                "-I",
                PRELUDE_DIR,
                // "-std=c++23",
            ],
            stdout: "inherit",
            stderr: "inherit",
        });

        if (compile.exitCode !== 0) {
            console.error("Failed to precompile prelude headers.");
            process.exit(1);
        }

        console.log(
            `Successfully precompiled headers to ${PRECOMPILED_HEADER_PATH}`,
        );
    } catch (error: any) {
        console.error(`Error: ${error.message}`);
        process.exit(1);
    }
}

precompileHeaders();
