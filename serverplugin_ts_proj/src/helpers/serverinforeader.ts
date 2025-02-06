import * as fs from 'fs';

export class server_inf_reader {
    private static instance: server_inf_reader | null = null;
    private config_map: Map<string, string>;

    private constructor() {
        this.config_map = new Map<string, string>();
    }

    // Static method to get the instance
    public static get_instance(): server_inf_reader {
        if (!server_inf_reader.instance) {
            server_inf_reader.instance = new server_inf_reader();
        }
        return server_inf_reader.instance;
    }

    // Method to load and parse the config file
    public load_config(file_path: string): void {
        const file_content = fs.readFileSync(file_path, 'utf-8');

        file_content.split(/\r?\n/).forEach((line) => {
            line = line.trim();

            // Skip empty lines and comments
            if (!line || line.startsWith('#')) return;

            // Handle inline comments (e.g., key = value #comment)
            const [key_value_part] = line.split('#');

            // Split key and value by '='
            const [key, raw_value] = key_value_part.split('=').map((part) => part.trim());

            if (key && raw_value !== undefined) {
                // Remove surrounding quotes (if any) from the value
                const value = this.strip_quotes(raw_value);
                this.config_map.set(key, value);
            }
        });
    }

    // Method to get all key-value pairs as an object
    public get_all(): Record<string, string> {
        return Object.fromEntries(this.config_map);
    }

    // Method to get a value by key
    public get_value(key: string): string {
        return this.config_map.get(key) ?? '';
    }

    public get_value_else_default(key: string, default_value: string): string {
        return this.config_map.get(key) ?? default_value;
    }

    // Method to get a value as a number, with a default fallback
    public get_value_as_number(key: string, default_value: number): number {
        const value = this.get_value(key);
        if (value !== undefined) {
            const parsed_number = Number(value);
            if (!isNaN(parsed_number)) {
                return parsed_number;
            }
        }
        return default_value;
    }

    // Helper method to strip quotes from a value
    private strip_quotes(value: string): string {
        return value.replace(/^["'`]|["'`]$/g, '');
    }
}
