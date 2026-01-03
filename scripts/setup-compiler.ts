import { spawnSync } from "child_process";
import { platform } from "os";

const COLORS = {
    reset: "\x1b[0m",
    green: "\x1b[32m",
    yellow: "\x1b[33m",
    red: "\x1b[31m",
    cyan: "\x1b[36m",
    bold: "\x1b[1m",
};

function checkGpp(): boolean {
    try {
        const result = spawnSync("g++", ["--version"], { encoding: "utf8" });
        return result.status === 0;
    } catch (e) {
        return false;
    }
}

function setup() {
    console.log(`${COLORS.bold}${COLORS.cyan}JSPP: Checking for C++ compiler...${COLORS.reset}`);

    if (checkGpp()) {
        console.log(`${COLORS.green}✔ g++ found.${COLORS.reset}`);
        return;
    }

    console.log(`${COLORS.yellow}⚠ g++ (GCC) not found in PATH. JSPP requires a C++23 compatible compiler.${COLORS.reset}`);

    const os = platform();

    if (os === "win32") {
        console.log(`\n${COLORS.bold}To install GCC on Windows:${COLORS.reset}`);
        console.log(`1. Install MinGW-w64 via MSYS2: ${COLORS.cyan}https://www.msys2.org/${COLORS.reset}`);
        console.log(`2. Or use winget: ${COLORS.cyan}winget install MSYS2.MSYS2${COLORS.reset}`);
        console.log(`   Then run: ${COLORS.bold}pacman -S mingw-w64-x86_64-gcc${COLORS.reset}`);
    } else if (os === "linux") {
        console.log(`\n${COLORS.bold}To install GCC on Linux (Ubuntu/Debian):${COLORS.reset}`);
        console.log(`${COLORS.cyan}sudo apt update && sudo apt install g++-14 -y${COLORS.reset}`);
        console.log(`\nAttempting to install now (may require password)...`);
        
        // Try to install automatically on Linux if apt is found
        try {
            const install = spawnSync("sudo", ["apt-get", "install", "-y", "g++-14"], { stdio: "inherit" });
            if (install.status === 0) {
                console.log(`${COLORS.green}✔ g++-14 installed successfully.${COLORS.reset}`);
                // Try to set up symlink if needed or just inform user
                return;
            }
        } catch (e) {
            console.error(`${COLORS.red}Automatic installation failed. Please install manually.${COLORS.reset}`);
        }
    } else if (os === "darwin") {
        console.log(`\n${COLORS.bold}To install GCC on macOS:${COLORS.reset}`);
        console.log(`${COLORS.cyan}brew install gcc${COLORS.reset}`);
    }

    console.log(`\n${COLORS.bold}After installation, please ensure 'g++' is in your PATH and restart your terminal.${COLORS.reset}\n`);
}

setup();
