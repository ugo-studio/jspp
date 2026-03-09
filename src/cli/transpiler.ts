import fs from "fs/promises";
import path from "path";
import { Interpreter } from "../index.js";
import { COLORS } from "./colors.js";
import { Spinner } from "./spinner.js";
import { msToHumanReadable } from "./utils.js";

export async function transpile(
    jsFilePath: string,
    cppFilePath: string,
    target: "native" | "wasm",
    spinner: Spinner,
) {
    spinner.update(`Reading ${path.basename(jsFilePath)}...`);
    const jsCode = await fs.readFile(jsFilePath, "utf-8");

    spinner.update("Transpiling to C++...");
    const transpileStartTime = performance.now();
    const interpreter = new Interpreter();
    const { cppCode, preludePath, wasmExports } = interpreter.interpret(
        jsCode,
        jsFilePath,
        target,
    );
    const transpileTime = msToHumanReadable(
        performance.now() - transpileStartTime,
    );

    // Ensure directory for cpp file exists
    await fs.mkdir(path.dirname(cppFilePath), { recursive: true });
    await fs.writeFile(cppFilePath, cppCode);
    spinner.succeed(
        `Generated cpp ${COLORS.dim}[${transpileTime}]${COLORS.reset}`,
    );

    return { preludePath, wasmExports };
}
