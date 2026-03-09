import { spawn } from "child_process";
import fs from "fs/promises";
import path from "path";

import { Spinner } from "./spinner.js";

export async function setupEmsdk(pkgDir: string, spinner: Spinner) {
    spinner.text = "Checking Emscripten SDK...";
    spinner.start();
    // Ensure emsdk is set up
    const setupEmsdk = spawn("bun", ["run", "scripts/setup-emsdk.ts"], {
        cwd: pkgDir,
        stdio: ["ignore", "pipe", "pipe"],
    });
    const setupExitCode = await new Promise<number>((resolve) => {
        setupEmsdk.on("close", (code) => resolve(code ?? 1));
    });
    if (setupExitCode !== 0) {
        spinner.fail("Emscripten SDK setup failed");
        process.exit(1);
    }
    spinner.stop();
}

export async function postProcessWasm(
    exeFilePath: string,
    wasmExports: {
        jsName: string;
        params: { name: string; type: string }[];
        returnType: string;
    }[],
) {
    const outputDir = path.dirname(exeFilePath);
    const outputBaseName = path.basename(exeFilePath, ".js");

    // Generate .d.ts
    let dtsContent = "";
    for (const exp of wasmExports) {
        const params = exp.params.map((p) => `${p.name}: ${p.type}`)
            .join(", ");
        dtsContent +=
            `export declare function wasm_export_${exp.jsName}(${params}): ${exp.returnType};\n`;
    }
    dtsContent += `export declare const Module: Promise<any>;\n`;
    dtsContent += `export declare const load: () => typeof Module;\n`;
    dtsContent +=
        `declare const jsppModule: (options?: any) => Promise<any>;\n`;
    dtsContent += `export default jsppModule;\n`;

    const dtsPath = path.join(outputDir, `${outputBaseName}.d.ts`);
    await fs.writeFile(dtsPath, dtsContent);

    // Append exports to .js glue code
    let jsGlue = await fs.readFile(exeFilePath, "utf-8");

    // Emscripten with -sEXPORT_ES6=1 adds this at the end
    const defaultExportStr = "export default jsppModule;";
    if (jsGlue.trim().endsWith(defaultExportStr)) {
        jsGlue = jsGlue.trim().substring(
            0,
            jsGlue.trim().length - defaultExportStr.length,
        );
    }

    jsGlue += `\n\n// JSPP Named Exports\n`;
    jsGlue += `let _notLoaded = true;\n`;
    jsGlue += `let _resolve = () => {};\n`;
    jsGlue += `let _reject = () => {};\n`;
    jsGlue += `let _instance;\n`;
    jsGlue +=
        `const _getinstance=()=>{if(!_instance)throw new Error("Module not loaded");return _instance}\n`;
    jsGlue +=
        `export const Module = new Promise((res,rej)=>{_resolve=res;_reject=rej});\n`;
    jsGlue += `export const load = async () => {\n`;
    jsGlue += ` if (_notLoaded) {\n`;
    jsGlue += `  _notLoaded = false;\n`;
    jsGlue +=
        `  jsppModule().then(m=>{_instance=m;_resolve(m)}).catch(e=>{_notLoaded=true;_reject(e)});\n`;
    jsGlue += ` }\n`;
    jsGlue += ` return Module;\n`;
    jsGlue += `};\n`;
    for (const exp of wasmExports) {
        jsGlue +=
            `export const wasm_export_${exp.jsName} = (...args) => _getinstance()._wasm_export_${exp.jsName}(...args);\n`;
    }
    jsGlue += `export default jsppModule;\n`;

    await fs.writeFile(exeFilePath, jsGlue);
}
