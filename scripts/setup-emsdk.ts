import { spawnSync } from "child_process";
import fs from "fs";
import { platform } from "os";
import path from "path";

const COLORS = {
    reset: "\x1b[0m",
    green: "\x1b[32m",
    yellow: "\x1b[33m",
    red: "\x1b[31m",
    cyan: "\x1b[36m",
    bold: "\x1b[1m",
};

const EMSDK_DIR = path.resolve(process.cwd(), ".emsdk");
const isWin = platform() === "win32";
const emsdkCmd = isWin ? "emsdk.bat" : "./emsdk";

function checkEmcc(): boolean {
    try {
        const result = spawnSync("emcc", ["--version"], { encoding: "utf8" });
        return result.status === 0;
    } catch (e) {
        return false;
    }
}

function runCommand(
    cmd: string,
    args: string[],
    cwd: string = process.cwd(),
): boolean {
    console.log(`${COLORS.cyan}> ${cmd} ${args.join(" ")}${COLORS.reset}`);
    const result = spawnSync(cmd, args, {
        cwd,
        stdio: "inherit",
        shell: isWin,
    });
    return result.status === 0;
}

async function setup() {
    console.log(
        `${COLORS.bold}${COLORS.cyan}JSPP: Setting up Emscripten SDK for Wasm support...${COLORS.reset}`,
    );

    if (checkEmcc()) {
        console.log(
            `${COLORS.green}✔ Emscripten (emcc) found in PATH.${COLORS.reset}`,
        );
        return;
    }

    if (fs.existsSync(path.join(EMSDK_DIR, emsdkCmd))) {
        console.log(
            `${COLORS.yellow}ℹ Local EMSDK found in .emsdk directory.${COLORS.reset}`,
        );
    } else {
        console.log(
            `${COLORS.yellow}ℹ EMSDK not found. Cloning from GitHub...${COLORS.reset}`,
        );
        if (
            !runCommand("git", [
                "clone",
                "https://github.com/emscripten-core/emsdk.git",
                ".emsdk",
            ])
        ) {
            console.error(
                `${COLORS.red}Error: Failed to clone EMSDK repository. Make sure 'git' is installed.${COLORS.reset}`,
            );
            return;
        }
    }

    console.log(
        `${COLORS.cyan}Installing latest Emscripten SDK...${COLORS.reset}`,
    );
    if (!runCommand(emsdkCmd, ["install", "latest"], EMSDK_DIR)) {
        console.error(
            `${COLORS.red}Error: Failed to install Emscripten SDK.${COLORS.reset}`,
        );
        return;
    }

    console.log(
        `${COLORS.cyan}Activating latest Emscripten SDK...${COLORS.reset}`,
    );
    if (
        !runCommand(emsdkCmd, ["activate", "latest", "--permanent"], EMSDK_DIR)
    ) {
        console.error(
            `${COLORS.red}Error: Failed to activate Emscripten SDK.${COLORS.reset}`,
        );
        return;
    }

    console.log(
        `\n${COLORS.green}✔ Emscripten SDK setup complete.${COLORS.reset}`,
    );
    console.log(
        `${COLORS.yellow}Note: To use emcc in your current terminal, you may need to run:${COLORS.reset}`,
    );
    if (isWin) {
        console.log(`${COLORS.bold}    .emsdk\\emsdk_env.bat${COLORS.reset}`);
    } else {
        console.log(
            `${COLORS.bold}    source .emsdk/emsdk_env.sh${COLORS.reset}`,
        );
    }
    console.log("");
}

setup();
