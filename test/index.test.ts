import fs from "fs/promises";
import path from "path";

import { Interpreter } from "../src";

const inputFile = path.join(process.cwd(), "test", "input.js");
const outputFile = path.join(process.cwd(), "test", "output.cpp");

// Example Usage with a closure
const jsCode = await fs.readFile(inputFile, "utf-8");
const interpreter = new Interpreter();
const cppCode = interpreter.interpret(jsCode);

await fs.writeFile(outputFile, cppCode);
console.log(`--- Generated C++ Code (${outputFile}) ---`);
