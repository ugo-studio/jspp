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

async function getLatestMtime(
    dirPath: string,
    filter?: (name: string) => boolean,
): Promise<number> {
    let maxMtime = 0;
    const entries = await fs.readdir(dirPath, { withFileTypes: true });
    for (const entry of entries) {
        const fullPath = path.join(dirPath, entry.name);
        if (entry.isDirectory()) {
            const nestedMtime = await getLatestMtime(fullPath, filter);
            if (nestedMtime > maxMtime) maxMtime = nestedMtime;
        } else {
            if (filter && !filter(entry.name)) continue;
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
    return Array.prototype.concat(...files).filter((f) => f.endsWith(".cpp"));
}

async function precompileHeaders() {
    const force = process.argv.includes("--force");
    try {
        await fs.mkdir(PRECOMPILED_HEADER_BASE_DIR, { recursive: true });

        // Latest mtime of any header file
        const latestHeaderMtime = await getLatestMtime(
            PRELUDE_DIR,
            (name) => name.endsWith(".hpp") || name.endsWith(".h"),
        );

        for (const mode of MODES) {
            const modeDir = path.join(PRECOMPILED_HEADER_BASE_DIR, mode.name);
            const headerPath = path.join(modeDir, "jspp.hpp");
            const gchPath = path.join(modeDir, "jspp.hpp.gch");

            console.log(`\n[${mode.name.toUpperCase()}] Checking headers...`);
            await fs.mkdir(modeDir, { recursive: true });

            let gchRebuilt = false;
            let shouldBuildGch = force;

            if (!shouldBuildGch) {
                try {
                    const gchStats = await fs.stat(gchPath);
                    if (latestHeaderMtime > gchStats.mtimeMs) {
                        shouldBuildGch = true;
                    }
                } catch (e) {
                    shouldBuildGch = true;
                }
            }

            if (shouldBuildGch) {
                console.log(`[${mode.name.toUpperCase()}] Compiling header...`);
                // Copy jspp.hpp
                await fs.copyFile(
                    path.join(PRELUDE_DIR, "jspp.hpp"),
                    headerPath,
                );

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
                    { stdio: "inherit" },
                );

                if (compile.status !== 0) {
                    try {
                        await fs.unlink(tempGchPath);
                    } catch (e) {}
                    console.error(
                        `[${mode.name.toUpperCase()}] Failed to precompile headers.`,
                    );
                    process.exit(1);
                }

                await fs.rename(tempGchPath, gchPath);
                gchRebuilt = true;
                console.log(`[${mode.name.toUpperCase()}] PCH Success.`);
            } else {
                console.log(
                    `[${mode.name.toUpperCase()}] Headers are up-to-date.`,
                );
            }

            // --- Incremental Compilation of .cpp files ---
            const cppFiles = await findCppFiles(PRELUDE_DIR);
            const objFiles: string[] = [];
            let anyObjRebuilt = false;

            const gchMtime = (await fs.stat(gchPath)).mtimeMs;

            for (const cppFile of cppFiles) {
                const relativePath = path.relative(PRELUDE_DIR, cppFile);
                const objFile = path.join(
                    modeDir,
                    relativePath.replace(/\.cpp$/, ".o"),
                );
                await fs.mkdir(path.dirname(objFile), { recursive: true });
                objFiles.push(objFile);

                let shouldCompile = force || gchRebuilt;
                if (!shouldCompile) {
                    try {
                        const objStats = await fs.stat(objFile);
                        const cppStats = await fs.stat(cppFile);
                        if (
                            cppStats.mtimeMs > objStats.mtimeMs ||
                            gchMtime > objStats.mtimeMs
                        ) {
                            shouldCompile = true;
                        }
                    } catch (e) {
                        shouldCompile = true;
                    }
                }

                if (shouldCompile) {
                    console.log(
                        `[${mode.name.toUpperCase()}] Compiling ${relativePath}...`,
                    );
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
                        { stdio: "inherit" },
                    );

                    if (compile.status !== 0) {
                        console.error(
                            `[${mode.name.toUpperCase()}] Failed to compile ${relativePath}`,
                        );
                        process.exit(1);
                    }
                    anyObjRebuilt = true;
                }
            }

            const libPath = path.join(modeDir, "libjspp.a");
            let shouldArchive = anyObjRebuilt;
            if (!shouldArchive) {
                try {
                    await fs.access(libPath);
                } catch (e) {
                    shouldArchive = true;
                }
            }

            if (shouldArchive) {
                console.log(
                    `[${mode.name.toUpperCase()}] Updating runtime library...`,
                );
                const tempLibPath = `${libPath}.tmp`;
                const archive = spawnSync(
                    "ar",
                    ["rcs", tempLibPath, ...objFiles],
                    { stdio: "inherit" },
                );

                if (archive.status !== 0) {
                    try {
                        await fs.unlink(tempLibPath);
                    } catch (e) {}
                    console.error(
                        `[${mode.name.toUpperCase()}] Failed to create static library.`,
                    );
                    process.exit(1);
                }

                await fs.rename(tempLibPath, libPath);
                console.log(
                    `[${mode.name.toUpperCase()}] Runtime Library Success.`,
                );
            } else {
                console.log(
                    `[${mode.name.toUpperCase()}] Runtime library is up-to-date.`,
                );
            }
        }
    } catch (error: any) {
        console.error(`Error: ${error.message}`);
        process.exit(1);
    }
}

precompileHeaders();
