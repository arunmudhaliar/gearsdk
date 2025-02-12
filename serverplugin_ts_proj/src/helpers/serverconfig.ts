import qzookeeper from "./qzookeeper";
import interface_qzookeeper  from "./qzookeeper";
import * as fs from 'fs';
import { debug_print, LOG_LEVEL_0, LOG_LEVEL_4 } from "./sdktypes";


export abstract class observer_serverconfig {
    abstract configchanged(path: string, data: string): void;
}

export class serverconfig {
    private static __LOGTAG__: string = `serverconfig`;
    private zk_interface: interface_qzookeeper | null;
    private config_change_observer: observer_serverconfig | null;
    private configs: Map<string, string>;

    constructor(zk_interface: interface_qzookeeper | null, observer: observer_serverconfig | null) {
        this.zk_interface = zk_interface;
        this.config_change_observer = observer;
        this.configs = new Map();

        if (this.zk_interface) {
            this.zk_interface.register_value_change_callback(
                serverconfig.zk_value_change_listener,
                this
            );
        }
    }

    public clear(): void {
        this.configs.clear();
    }

    public async load(path: string, qzk: qzookeeper | null, zk_root_folder: string): Promise<boolean> {
        debug_print(LOG_LEVEL_0, serverconfig.__LOGTAG__, `reading from zk_root_node ${zk_root_folder}`);
        let result = false;
        try {
            let buffer = fs.readFileSync(path, 'utf8'); // Read file content
            result = await this.iterate_and_load_keys(buffer, qzk, zk_root_folder);
        } catch (error) {
            console.error(`Couldn't read zk config - ${path}:`, error);
        }
        return result;
    }

    public get_config(key: string, default_value: string): string {
        return this.configs.get(key) ?? default_value;
    }

    public get_int32(key: string, DEFAULT_VALUE: number): number {
        const value = this.configs.get(key);
        if (!value) {
            console.warn(
                `get_int32: zk config not found for key ${key}. Setting default value of ${DEFAULT_VALUE}!`
            );
            return DEFAULT_VALUE;
        }

        const parsedValue = parseInt(value, 10);
        if (isNaN(parsedValue)) {
            console.error(
                `Unable to parse ${key} value - ${value}. Setting default value of ${DEFAULT_VALUE}!`
            );
            return DEFAULT_VALUE;
        }

        return parsedValue;
    }

    public get_string(key: string, default_value: string): string {
        return this.configs.get(key) ?? default_value;
    }

    private async iterate_and_load_keys(buffer: string, qzk: qzookeeper | null, zk_root_folder: string): Promise<boolean> {
        const configKeys: Map<string, string[]> = new Map();
    
        // Parse the buffer content as JSON
        let parsedData: any;
        try {
            parsedData = JSON.parse(buffer);
        } catch (error : Error | any) {
            console.error(`JSON parse error: ${error.message}`);
            return false;
        }
    
        // Validate and process the parsed data
        for (const [key, value] of Object.entries(parsedData)) {
            debug_print(LOG_LEVEL_0, serverconfig.__LOGTAG__, `${key}`);
            if (!Array.isArray(value)) {
                console.warn(`Root key (${key}) must be an array!`);
                return false;
            }
    
            for (const n of value) {
                if (typeof n !== "string") {
                    console.warn(`Value (${n}) must be a string!`);
                    return false;
                }
                configKeys.get(key)?.push(n) ?? configKeys.set(key, [n]);
            }
        }
    
        // Load values from Zookeeper
        const loadPromises: Promise<void>[] = [];
    
        for (const [rootKey, values] of configKeys.entries()) {
            for (const key of values) {
                const zkKey = `${zk_root_folder}/${rootKey}/${key}`;
                loadPromises.push(
                    new Promise((resolve) => {
                        qzk?.get_data( zkKey, (error, data) => {
                                if (error) {
                                    console.warn(`Node does not exist or failed to fetch for key ${zkKey}: ${error.message}`);
                                } else {
                                    const modZkKey = `${rootKey}/${key}`;
                                    this.configs.set(modZkKey, data.toString());
                                }
                                resolve();
                            },
                            "{}" // Default value
                        );
                    })
                );
            }
        }
    
        // Wait for all Zookeeper calls to complete
        await Promise.all(loadPromises);
    
        debug_print(LOG_LEVEL_0, serverconfig.__LOGTAG__, `${this.configs.size} keys loaded`);
        return true;
    }
    
    public try_update_value(path: string, data: string): boolean {
        const array = path.split("/");
        let mod_zk_key = path;

        if (array.length > 1) {
            mod_zk_key = mod_zk_key.replace(`/${array[1]}/`, "");
        }

        if (this.configs.has(mod_zk_key)) {
            this.configs.set(mod_zk_key, data);
            return true;
        }

        return false;
    }

    public static zk_value_change_listener(path: string, data: string, context: any): void {
        const config = context as serverconfig;
        if (config.try_update_value(path, data)) {
            debug_print(LOG_LEVEL_0, serverconfig.__LOGTAG__, `Config updated: ${path}`);
            config.config_change_observer?.configchanged(path, data);
        } else {
            console.warn(`Config not found for: ${path}`);
        }
    }
}
