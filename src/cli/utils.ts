import fs from "fs/promises";
import path from "path";

export async function getLatestMtime(dirPath: string): Promise<number> {
    let maxMtime = 0;
    const entries = await fs.readdir(dirPath, { withFileTypes: true });
    for (const entry of entries) {
        const fullPath = path.join(dirPath, entry.name);
        if (entry.isDirectory()) {
            const nestedMtime = await getLatestMtime(fullPath);
            if (nestedMtime > maxMtime) maxMtime = nestedMtime;
        } else {
            const stats = await fs.stat(fullPath);
            if (stats.mtimeMs > maxMtime) maxMtime = stats.mtimeMs;
        }
    }
    return maxMtime;
}

/**
 * Converts milliseconds to a human-readable format.
 * @param ms - The time in milliseconds
 * @returns A formatted string (e.g., "2 hours, 1 minute, 4 seconds")
 */
export function msToHumanReadable(ms: number): string {
    if (ms < 0) {
        throw new Error("Time cannot be negative");
    }

    if (ms === 0) {
        return "0 seconds";
    }

    // Define time constants in milliseconds
    const MS_PER_SECOND = 1000;
    const MS_PER_MINUTE = 60 * MS_PER_SECOND;
    const MS_PER_HOUR = 60 * MS_PER_MINUTE;
    const MS_PER_DAY = 24 * MS_PER_HOUR;

    // Calculate each unit
    const days = Math.floor(ms / MS_PER_DAY);
    const hours = Math.floor((ms % MS_PER_DAY) / MS_PER_HOUR);
    const minutes = Math.floor((ms % MS_PER_HOUR) / MS_PER_MINUTE);
    const seconds = Math.floor((ms % MS_PER_MINUTE) / MS_PER_SECOND);

    // Array to hold the formatted parts
    const parts: string[] = [];

    // Helper function to handle pluralization
    const addPart = (value: number, unit: string) => {
        if (value > 0) {
            parts.push(`${value} ${unit}${value > 1 ? "s" : ""}`);
        }
    };

    addPart(days, "day");
    addPart(hours, "hour");
    addPart(minutes, "minute");
    addPart(seconds, "second");

    // If the time was less than 1 second, it will be empty
    if (parts.length === 0) {
        return `${ms} millisecond${ms > 1 ? "s" : ""}`;
    }

    // Join the parts with a comma and space
    return parts.join(", ");
}
