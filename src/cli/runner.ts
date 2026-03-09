import { spawn } from "child_process";
import path from "path";
import { COLORS } from "./colors.js";

export async function runOutput(
    exeFilePath: string,
    scriptArgs: string[],
    isWasm: boolean,
) {
    if (isWasm) {
        console.log(
            `\n${COLORS.cyan}Compilation finished. To run the output, use node or a browser:${COLORS.reset}`,
        );
        console.log(
            `${COLORS.bold}    node ${
                path.basename(exeFilePath)
            }${COLORS.reset}\n`,
        );
    } else {
        console.log(
            `\n${COLORS.cyan}--- Running Output ---${COLORS.reset}`,
        );
        const run = spawn(exeFilePath, scriptArgs, {
            stdio: "inherit",
        });

        const runExitCode = await new Promise<number>((resolve) => {
            run.on("close", (code) => resolve(code ?? 1));
        });
        console.log(
            `${COLORS.cyan}----------------------${COLORS.reset}\n`,
        );

        if (runExitCode !== 0) {
            console.error(
                `${COLORS.red}Execution failed with exit code ${runExitCode}${COLORS.reset}`,
            );
            process.exit(1);
        }
    }
}
