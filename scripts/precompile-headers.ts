import { spawnSync } from "child_process";
import fs from "fs/promises";
import path from "path";

const PRELUDE_DIR = path.resolve(process.cwd(), "src", "prelude");
const PRECOMPILED_HEADER_BASE_DIR = path.resolve(
    process.cwd(),
    "prelude-build",
);

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

async function findCppFiles(dir: string): Promise<string[]> {
    const entries = await fs.readdir(dir, { withFileTypes: true });
    const files = await Promise.all(entries.map((entry) => {
        const res = path.resolve(dir, entry.name);
        return entry.isDirectory() ? findCppFiles(res) : res;
    }));
    return Array.prototype.concat(...files).filter(f => f.endsWith('.cpp'));
}

async function precompileHeaders() {
    const force = process.argv.includes("--force");
    try {
        await fs.mkdir(PRECOMPILED_HEADER_BASE_DIR, { recursive: true });
        const sourceMtime = await getLatestMtime(PRELUDE_DIR);

        for (const mode of MODES) {
            const modeDir = path.join(PRECOMPILED_HEADER_BASE_DIR, mode.name);
            const headerPath = path.join(modeDir, "jspp.hpp");
            const gchPath = path.join(modeDir, "jspp.hpp.gch");

            if (!force) {
                try {
                    const gchStats = await fs.stat(gchPath);
                    if (gchStats.mtimeMs >= sourceMtime) {
                        console.log(
                            `[${mode.name.toUpperCase()}] Headers are up-to-date. Skipping.`,
                        );
                        continue;
                    }
                } catch (e) {
                    // PCH doesn't exist, proceed to compile
                }
            }

            console.log(`\n[${mode.name.toUpperCase()}] Setting up...`);
            await fs.mkdir(modeDir, { recursive: true });

            // Copy jspp.hpp
            await fs.copyFile(path.join(PRELUDE_DIR, "jspp.hpp"), headerPath);

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
            const tempGchPath = `${gchPath}.tmp`;

            const compile = spawnSync(
                "g++",
                [
                    "-x",
                    "c++-header",
                    "-std=c++23",
                    ...mode.flags,
                    headerPath,
                    "-o",
                    tempGchPath,
                    "-I",
                    modeDir,
                    "-I",
                    PRELUDE_DIR,
                ],
                {
                    stdio: "inherit",
                },
            );

            if (compile.status !== 0) {
                try {
                    await fs.unlink(tempGchPath);
                } catch (e) {
                    // Ignore if temp file doesn't exist
                }
                console.error(
                    `[${mode.name.toUpperCase()}] Failed to precompile headers.`,
                );
                process.exit(1);
            }

            // Atomically replace the old GCH with the new one
            await fs.rename(tempGchPath, gchPath);
            console.log(`[${mode.name.toUpperCase()}] PCH Success.`);

            // --- NEW: Compile all .cpp files into libjspp.a ---
            console.log(
                `[${mode.name.toUpperCase()}] Compiling runtime library...`,
            );
            
            const cppFiles = await findCppFiles(PRELUDE_DIR);
            const objFiles: string[] = [];

            for (const cppFile of cppFiles) {
                const relativePath = path.relative(PRELUDE_DIR, cppFile);
                const objFile = path.join(modeDir, relativePath.replace(/\.cpp$/, '.o'));
                await fs.mkdir(path.dirname(objFile), { recursive: true });
                
                const compile = spawnSync(
                    "g++",
                    [
                        "-c",
                        "-std=c++23",
                        ...mode.flags,
                        cppFile,
                        "-o",
                        objFile,
                        "-I",
                        modeDir,
                        "-I",
                        PRELUDE_DIR,
                    ],
                    { stdio: "inherit" }
                );

                if (compile.status !== 0) {
                    console.error(`[${mode.name.toUpperCase()}] Failed to compile ${relativePath}`);
                    process.exit(1);
                }
                objFiles.push(objFile);
            }

            const libPath = path.join(modeDir, "libjspp.a");
            const archive = spawnSync(
                "ar",
                ["rcs", libPath, ...objFiles],
                { stdio: "inherit" }
            );

            if (archive.status !== 0) {
                console.error(`[${mode.name.toUpperCase()}] Failed to create static library.`);
                process.exit(1);
            }

            console.log(`[${mode.name.toUpperCase()}] Runtime Library Success.`);
        }
    } catch (error: any) {
        console.error(`Error: ${error.message}`);
        process.exit(1);
    }
}

precompileHeaders();
