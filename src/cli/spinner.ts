import { COLORS } from "./colors.js";

export class Spinner {
    private frames = ["⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"];
    private interval: Timer | null = null;
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
