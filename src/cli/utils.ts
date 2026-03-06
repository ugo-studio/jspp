import fs from "fs/promises";
import path from "path";

export async function getLatestMtime(
    dirPath: string,
    filter?: (name: string) => boolean,
): Promise<number> {
    let maxMtime = 0;
    const entries = await fs.readdir(dirPath, { withFileTypes: true });
    for (const entry of entries) {
        const fullPath = path.join(dirPath, entry.name);
        if (entry.isDirectory()) {
            const nestedMtime = await getLatestMtime(fullPath, filter);
            if (nestedMtime > maxMtime) maxMtime = nestedMtime;
        } else {
            if (filter && !filter(entry.name)) continue;
            const stats = await fs.stat(fullPath);
            if (stats.mtimeMs > maxMtime) maxMtime = stats.mtimeMs;
        }
    }
    return maxMtime;
}

/**
 * Converts milliseconds to a single-unit, human-readable decimal format.
 * @param ms - The time in milliseconds
 * @returns A formatted string (e.g., "2.52s", "1.5h")
 */
export function msToHumanReadable(ms: number): string {
    if (ms < 0) {
        throw new Error("Time cannot be negative");
    }

    if (ms === 0) {
        return "0ms";
    }

    const MS_PER_SECOND = 1000;
    const MS_PER_MINUTE = 60 * MS_PER_SECOND;
    const MS_PER_HOUR = 60 * MS_PER_MINUTE;
    const MS_PER_DAY = 24 * MS_PER_HOUR;

    // Helper to safely truncate to 2 decimal places without rounding up
    const formatValue = (val: number): string => {
        // toFixed(3) resolves float math errors, slice(0,-1) enforces truncation (e.g. 2.528 -> 2.52)
        let str = val.toFixed(3).slice(0, -1);
        // Remove trailing zeroes and lingering decimal points (e.g., "2.00" -> "2")
        return str.replace(/0+$/, "").replace(/\.$/, "");
    };

    if (ms >= MS_PER_DAY) {
        return formatValue(ms / MS_PER_DAY) + "d";
    }
    if (ms >= MS_PER_HOUR) {
        return formatValue(ms / MS_PER_HOUR) + "h";
    }
    if (ms >= MS_PER_MINUTE) {
        return formatValue(ms / MS_PER_MINUTE) + "m";
    }
    if (ms >= MS_PER_SECOND) {
        return formatValue(ms / MS_PER_SECOND) + "s";
    }

    return formatValue(ms) + "ms";
}
