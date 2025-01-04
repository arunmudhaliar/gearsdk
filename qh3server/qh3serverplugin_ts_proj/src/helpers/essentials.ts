import { createHash } from 'crypto';

export class essentials {
    public static get_time_utc_readable(): { utc_date_string: string, utc_date_number: number } {
        const utc_date = new Date();
        const utc_date_string = utc_date.toISOString().replace("T", " ").split(".")[0]; // Trim milliseconds and adjust format
        const utc_date_number : number = Math.floor(utc_date.getTime() / 1000);
        return { utc_date_string, utc_date_number };  // Return both formatted string and the original Date object
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

    public static sha256(input: string): string {
        const hash = createHash('sha256');
        hash.update(input);
        return hash.digest('hex');
    }
}