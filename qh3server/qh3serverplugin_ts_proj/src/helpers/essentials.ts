export class essentials {
    public static get_time_utc_readable(): string {
        const utcTime = new Date();
        return utcTime.toISOString().replace("T", " ").split(".")[0]; // Trim milliseconds and adjust format
    }

    public static extract_ip_and_port(input: string): [string, number] | null {
        const regex = /^([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+):(\d+)$/;
        const match = input.match(regex);
        
        if (match) {
            const ip = match[1];
            const port = parseInt(match[2], 10);
            return [ip, port];
        }
        
        // Return null if the input doesn't match the expected format
        return null;
    }
}