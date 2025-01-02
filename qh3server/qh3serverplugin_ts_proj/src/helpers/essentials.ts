export class essentials {
    public static get_time_utc_readable(): string {
        const utcTime = new Date();
        return utcTime.toISOString().replace("T", " ").split(".")[0]; // Trim milliseconds and adjust format
    }
}