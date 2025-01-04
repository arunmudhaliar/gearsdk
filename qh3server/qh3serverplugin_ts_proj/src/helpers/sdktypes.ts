export const LOG_LEVEL = 2
export const LOG_LEVEL_0 = 0
export const LOG_LEVEL_1 = 1
export const LOG_LEVEL_2 = 2
export const LOG_LEVEL_3 = 3
export const LOG_LEVEL_4 = 4
export const LOG_LEVEL_5 = 5

export const EXIT_SUCCESS = 0
export const EXIT_FAILURE = 1

export function debug_print(level: number, logtag: string, message: string, ...args: any[]) {
    if (level > LOG_LEVEL) {
		return;
	}
    console.log(`[${logtag}] - ${message}`, ...args);
}

export function debug_error(logtag: string, message: string, ...args: any[]) {
    console.error(`[ERROR] [${logtag}] - ${message}`, ...args);
}

export function debug_warn(logtag: string, message: string, ...args: any[]) {
    console.error(`[WARN] [${logtag}] - ${message}`, ...args);
}