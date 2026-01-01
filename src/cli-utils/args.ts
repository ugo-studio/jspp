import path from "path";

import { COLORS } from "./colors";

export interface CliOptions {
    jsFilePath: string;
    isRelease: boolean;
    keepCpp: boolean;
    outputExePath: string | null;
    scriptArgs: string[];
}

export function parseArgs(rawArgs: string[]): CliOptions {
    let jsFilePathArg: string | null = null;
    let isRelease = false;
    let keepCpp = false;
    let outputExePath: string | null = null;
    let scriptArgs: string[] = [];

    for (let i = 0; i < rawArgs.length; i++) {
        const arg = rawArgs[i];
        if (!arg) continue;

        if (arg === "--") {
            scriptArgs = rawArgs.slice(i + 1);
            break;
        }

        if (arg === "--release") {
            isRelease = true;
        } else if (arg === "--keep-cpp") {
            keepCpp = true;
        } else if (arg === "-o" || arg === "--output") {
            if (i + 1 < rawArgs.length) {
                outputExePath = rawArgs[i + 1] ?? null;
                i++;
            } else {
                console.error(
                    `${COLORS.red}Error: --output requires a file path argument.${COLORS.reset}`,
                );
                process.exit(1);
            }
        } else if (arg.startsWith("-")) {
            console.warn(
                `${COLORS.yellow}Warning: Unknown argument '${arg}'${COLORS.reset}`,
            );
        } else {
            if (jsFilePathArg) {
                console.error(
                    `${COLORS.red}Error: Multiple input files specified.${COLORS.reset}`,
                );
                process.exit(1);
            }
            jsFilePathArg = arg;
        }
    }

    if (!jsFilePathArg) {
        console.log(
            `${COLORS.bold}Usage:${COLORS.reset} jspp <path-to-js-file> [--release] [--keep-cpp] [-o <output-path>] [-- <args...>]`,
        );
        process.exit(1);
    }

    return {
        jsFilePath: path.resolve(process.cwd(), jsFilePathArg),
        isRelease,
        keepCpp,
        outputExePath: outputExePath
            ? path.resolve(process.cwd(), outputExePath)
            : null,
        scriptArgs,
    };
}
