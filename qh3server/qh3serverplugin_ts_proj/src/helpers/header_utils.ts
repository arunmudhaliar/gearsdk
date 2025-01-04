import { qh3serversdk } from "./qh3serversdk";

export type header = {
    name: string;
    value: string;
};

export class header_utils {
    // Convert a JSON string to a Map<number, header>
    public static json_to_map(json_string: string): Map<number, header> {
        const map = new Map<number, header>();
        try {
            // Parse the JSON string
            const json_object = JSON.parse(json_string);

            // Iterate over the object properties
            for (const key in json_object) {
                if (Object.prototype.hasOwnProperty.call(json_object, key)) {
                    const crc = Number(key);
                    const header_data: header = {
                        name: json_object[key].name,
                        value: json_object[key].value,
                    };
                    map.set(crc, header_data);
                }
            }
        } catch (error) {
            console.error("Invalid JSON string:", error);
        }
        return map;
    }

    // Convert a Map<number, header> to a JSON string
    public static map_to_json(headers: Map<number, header>): string {
        const json_object: { [key: string]: header } = {};

        headers.forEach((header_data, crc) => {
            json_object[crc] = {
                name: header_data.name,
                value: header_data.value,
            };
        });

        return JSON.stringify(json_object);
    }

    public static get_header(name: string, headers: Map<number, header>) : header | undefined | null {
        let crc: number = qh3serversdk.qh3serverplugin.mod_crc32(0, name, name.length);
        if (headers.has(crc)) {
            return headers.get(crc);
        }
        return null;
    }
}
