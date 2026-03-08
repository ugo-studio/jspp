import ts from "typescript";

const COLORS = {
    reset: "\x1b[0m",
    red: "\x1b[31m",
    yellow: "\x1b[33m",
    cyan: "\x1b[36m",
    bold: "\x1b[1m",
    dim: "\x1b[2m",
};

export class CompilerError extends Error {
    public readonly node: ts.Node;
    public readonly type: string;

    constructor(message: string, node: ts.Node, type: string = "SyntaxError") {
        super(message);
        this.node = node;
        this.type = type;
        Object.setPrototypeOf(this, CompilerError.prototype);
    }

    public getFormattedError(): string {
        const sourceFile = this.node.getSourceFile();
        if (!sourceFile) {
            return `${COLORS.red}${this.type}: ${this.message}${COLORS.reset}`;
        }

        const start = this.node.getStart();
        const { line, character } = sourceFile.getLineAndCharacterOfPosition(start);
        
        // Get the full line content safely
        const lineStartPos = sourceFile.getPositionOfLineAndCharacter(line, 0);
        let lineEndPos: number;
        try {
            lineEndPos = sourceFile.getLineEndOfPosition(start);
        } catch {
            lineEndPos = sourceFile.text.length;
        }
        
        const lineContent = sourceFile.text.substring(lineStartPos, lineEndPos).replace(/\r/g, ''); // Remove CR

        const fileName = sourceFile.fileName;
        const lineNum = line + 1;
        const charNum = character + 1;

        let output = `\n${COLORS.red}${COLORS.bold}${this.type}:${COLORS.reset} ${this.message}\n`;
        output += `   ${COLORS.dim}at ${fileName}:${lineNum}:${charNum}${COLORS.reset}\n\n`;

        // Code frame
        const lineNumStr = `${lineNum} | `;
        output += `${COLORS.dim}${lineNumStr}${COLORS.reset}${lineContent}\n`;
        
        // Adjust pointer position if there are tabs
        let pointerPadding = "";
        for (let i = 0; i < character; i++) {
            if (lineContent[i] === '\t') {
                pointerPadding += "\t";
            } else {
                pointerPadding += " ";
            }
        }
        
        const padding = " ".repeat(lineNumStr.length) + pointerPadding;
        output += `${padding}${COLORS.red}^${COLORS.reset}\n`;

        return output;
    }
}
