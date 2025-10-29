import fs from "fs/promises";
import path from "path";

import { Interpreter } from "../src";

async function main() {
    const args = process.argv.slice(2);
    if (args.length === 0) {
        console.error("Usage: bun run scripts/run-js.ts <path-to-js-file>");
        process.exit(1);
    }

    const jsFilePath = path.resolve(process.cwd(), args[0]!);
    const jsFileName = path.basename(jsFilePath, ".js");
    const outputDir = path.join(process.cwd(), "temp-run");
    const cppFilePath = path.join(outputDir, `${jsFileName}.cpp`);
    const exeFilePath = path.join(outputDir, `${jsFileName}.exe`);

    try {
        const jsCode = await fs.readFile(jsFilePath, "utf-8");
        const interpreter = new Interpreter();
        const cppCode = interpreter.interpret(jsCode);

        await fs.mkdir(outputDir, { recursive: true });
        await fs.writeFile(cppFilePath, cppCode);

        console.log(`Compiling ${cppFilePath}...`);
        const compile = Bun.spawnSync({
            cmd: ["g++", cppFilePath, "-o", exeFilePath, "-std=c++23"],
            stdout: "inherit",
            stderr: "inherit",
        });

        if (compile.exitCode !== 0) {
            console.error(`Compilation failed for ${jsFileName}.js`);
            process.exit(1);
        }

        console.log(`Running ${exeFilePath}...`);
        console.log("\x1b[32m\n------start------\x1b[0m");
        const run = Bun.spawnSync({
            cmd: [exeFilePath],
            stdout: "inherit",
            stderr: "inherit",
        });
        console.log("\x1b[32m\n------end--------\x1b[0m");

        if (run.exitCode !== 0) {
            console.error(`Execution failed for ${jsFileName}.js`);
            process.exit(1);
        }

        console.log(`Successfully ran ${jsFileName}.js`);
    } catch (error: any) {
        console.error(`Error: ${error.message}`);
        process.exit(1);
    } finally {
        // Clean up generated files
        try {
            // await fs.unlink(cppFilePath);
            await fs.unlink(exeFilePath);
            // await fs.rmdir(outputDir);
        } catch (cleanupError: any) {
            console.warn(`Cleanup warning: ${cleanupError.message}`);
        }
    }
}

main();
