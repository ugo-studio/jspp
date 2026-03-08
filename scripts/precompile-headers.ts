import { spawn } from "child_process";
import fs from "fs/promises";
import path from "path";

const COLORS = {
    reset: "\x1b[0m",
    cyan: "\x1b[36m",
    green: "\x1b[32m",
    yellow: "\x1b[33m",
    red: "\x1b[31m",
    dim: "\x1b[2m",
    bold: "\x1b[1m",
};

export class Spinner {
    private frames = ["⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"];
    private interval: any = null;
    private frameIndex = 0;
    public text: string;

    constructor(text: string) {
        this.text = text;
    }

    start() {
        process.stdout.write("\x1b[?25l"); // Hide cursor
        this.frameIndex = 0;
        this.render();
        this.interval = setInterval(() => {
            this.frameIndex = (this.frameIndex + 1) % this.frames.length;
            this.render();
        }, 80);
    }

    update(text: string) {
        this.text = text;
        this.render();
    }

    stop(symbol: string = "", color: string = COLORS.reset) {
        if (this.interval) {
            clearInterval(this.interval);
            this.interval = null;
        }
        this.clearLine();
        process.stdout.write(
            `${color}${symbol} ${COLORS.reset} ${this.text}\n`,
        );
        process.stdout.write("\x1b[?25h"); // Show cursor
    }

    succeed(text?: string) {
        if (text) this.text = text;
        this.stop("✔", COLORS.green);
    }

    fail(text?: string) {
        if (text) this.text = text;
        this.stop("✖", COLORS.red);
    }

    info(text?: string) {
        if (text) this.text = text;
        this.stop("ℹ", COLORS.cyan);
    }

    private render() {
        this.clearLine();
        const frame = this.frames[this.frameIndex];
        process.stdout.write(
            `${COLORS.cyan}${frame} ${COLORS.reset} ${this.text}`,
        );
    }

    private clearLine() {
        process.stdout.write("\r\x1b[K");
    }
}

const PRELUDE_DIR = path.resolve(process.cwd(), "src", "prelude");
const PRECOMPILED_HEADER_BASE_DIR = path.resolve(
    process.cwd(),
    "prelude-build",
);

const MODES = [
    {
        name: "debug",
        flags: ["-Og"],
        compiler: "g++",
        archiver: "ar",
    },
    {
        name: "release",
        flags: ["-O3", "-DNDEBUG"],
        compiler: "g++",
        archiver: "ar",
    },
    {
        name: "wasm",
        flags: ["-O3", "-DNDEBUG", "-sASYNCIFY", "-sALLOW_MEMORY_GROWTH=1"],
        compiler: "em++",
        archiver: "emar",
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
        if (entry.isDirectory()) {
            return findCppFiles(res);
        } else {
            return res.endsWith(".cpp") ? [res] : [];
        }
    }));
    return Array.prototype.concat(...files);
}

async function runCommand(cmd: string, args: string[]): Promise<boolean> {
    console.log(`${COLORS.dim}> ${cmd} ${args.join(" ")}${COLORS.reset}`);
    return new Promise((resolve) => {
        const proc = spawn(cmd, args, { stdio: "inherit", shell: process.platform === "win32" });
        proc.on("close", (code) => resolve(code === 0));
    });
}

async function precompileHeaders() {
    const force = process.argv.includes("--force");
    const jsppCliIsParent = process.argv.includes("--jspp-cli-is-parent");

    if (!jsppCliIsParent) {
        console.log(
            `${COLORS.bold}${COLORS.cyan}JSPP: Precompiling headers and runtime...${COLORS.reset}\n`,
        );
    }

    try {
        await fs.mkdir(PRECOMPILED_HEADER_BASE_DIR, { recursive: true });

        const latestHeaderMtime = await getLatestMtime(
            PRELUDE_DIR,
            (name) => name.endsWith(".hpp") || name.endsWith(".h"),
        );

        for (const mode of MODES) {
            const modeDir = path.join(PRECOMPILED_HEADER_BASE_DIR, mode.name);
            const headerPath = path.join(modeDir, "jspp.hpp");
            const gchPath = path.join(modeDir, "jspp.hpp.gch");

            const modeLabel = `[${mode.name.toUpperCase()}]`;
            const spinner = new Spinner(`${modeLabel} Checking headers...`);
            spinner.start();

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
                spinner.update(`${modeLabel} Compiling header...`);
                await fs.copyFile(
                    path.join(PRELUDE_DIR, "jspp.hpp"),
                    headerPath,
                );

                const tempGchPath = `${gchPath}.tmp`;
                const success = await runCommand(mode.compiler, [
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
                ]);

                if (!success) {
                    spinner.fail(`${modeLabel} Failed to precompile headers.`);
                    process.exit(1);
                }

                await fs.rename(tempGchPath, gchPath);
                gchRebuilt = true;
                spinner.succeed(`${modeLabel} PCH Success.`);
            } else {
                spinner.succeed(`${modeLabel} Headers are up-to-date.`);
            }

            // --- Incremental Compilation of .cpp files ---
            const cppFiles = await findCppFiles(PRELUDE_DIR);
            const objFiles: string[] = [];
            let anyObjRebuilt = false;

            const gchMtime = (await fs.stat(gchPath)).mtimeMs;

            const libSpinner = new Spinner(
                `${modeLabel} Checking runtime library...`,
            );
            libSpinner.start();

            for (let idx = 0; idx < cppFiles.length; idx++) {
                const cppFile = cppFiles[idx];
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
                    libSpinner.update(
                        `${modeLabel} Compiling ${relativePath} ${COLORS.dim}[${
                            idx + 1
                        }/${cppFiles.length}]${COLORS.reset}`,
                    );
                    const success = await runCommand(mode.compiler, [
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
                    ]);

                    if (!success) {
                        libSpinner.fail(
                            `${modeLabel} Failed to compile ${relativePath}`,
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
                libSpinner.update(`${modeLabel} Updating runtime library...`);
                const tempLibPath = `${libPath}.tmp`;

                const success = await runCommand(mode.archiver, [
                    "rcs",
                    tempLibPath,
                    ...objFiles,
                ]);

                if (!success) {
                    libSpinner.fail(
                        `${modeLabel} Failed to create static library.`,
                    );
                    process.exit(1);
                }

                await fs.rename(tempLibPath, libPath);
                libSpinner.succeed(`${modeLabel} Runtime Library Success.`);
            } else {
                libSpinner.succeed(
                    `${modeLabel} Runtime library is up-to-date.`,
                );
            }
        }
        if (!jsppCliIsParent) {
            console.log(
                `\n${COLORS.bold}${COLORS.green}JSPP: Environment ready.${COLORS.reset}\n`,
            );
        }
    } catch (error: any) {
        console.error(`${COLORS.red}Error: ${error.message}${COLORS.reset}`);
        process.exit(1);
    }
}

precompileHeaders();
