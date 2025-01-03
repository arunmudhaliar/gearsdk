export const LOG_LEVEL_4 = 3

export function debug_print(level: number, logtag: string, message: string, ...args: any[]) {
    console.log(`[${level}] ${logtag} - ${message}`, ...args);
}

export function debug_print_error(logtag: string, message: string, ...args: any[]) {
    console.error(`[ERROR] ${logtag} - ${message}`, ...args);
}