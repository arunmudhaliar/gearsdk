import zookeeper, { Exception, Stat } from 'node-zookeeper-client';
import { debug_print, debug_warn, LOG_LEVEL_0, LOG_LEVEL_4 } from './sdktypes';

/**
 * Type definition for the callback function triggered on ZooKeeper value changes.
 * 
 * @param path The path of the changed node.
 * @param data The new data of the changed node.
 * @param context User-defined context data.
 */
type type_qzk_value_changed = (path: string, data: string, context: any) => void;

/**
 * Interface for a ZooKeeper client wrapper to handle value change callbacks.
 */
interface interface_qzookeeper {
    /**
     * Registers a callback function to be called when a value changes in the ZooKeeper client.
     * 
     * @param callback The callback function to be registered.
     * @param context A user-defined context object that will be passed to the callback function.
     */
    register_value_change_callback(callback: type_qzk_value_changed, context: any): void;

    /**
     * Unregisters a previously registered value change callback function.
     * 
     * @param callback The callback function to be unregistered.
     * @param context The user-defined context object that was passed to the callback function.
     */
    unregister_value_change_callback(callback: type_qzk_value_changed, context: any): void;
}


class qzookeeper implements interface_qzookeeper {
    private static __LOGTAG__: string = `qzookeeper`;
    private client: zookeeper.Client;
    private zk_connection_string: string;
    private retry_attempts: number;
    private retry_interval: number;
    private close_issued: boolean = false;
    // Store registered callbacks and their associated context in a Map.
    private value_change_callbacks: Map<type_qzk_value_changed, any> = new Map();
    // Store paths and their associated watchers
    private active_watchers: Map<string, (event: zookeeper.Event) => void> = new Map();


    constructor(zk_connection_string: string, retry_attempts: number = 3, retry_interval: number = 1000) {
        this.zk_connection_string = zk_connection_string;
        this.client = zookeeper.createClient(zk_connection_string);
        this.retry_attempts = retry_attempts;
        this.retry_interval = retry_interval;

        // Add event listeners for various Zookeeper events
        this.client.on('connected', () => {
            debug_print(LOG_LEVEL_0, qzookeeper.__LOGTAG__, 'qzookeeper client connected.');
        });

        this.client.on('disconnected', () => {
            debug_print(LOG_LEVEL_0, qzookeeper.__LOGTAG__, `qzookeeper client disconnected. ${this.close_issued}`);
            if (!this.close_issued) {
                this.reconnect();
            } else {
                this.close_issued = false;
            }
        });

        this.client.on('auth_failed', () => {
            debug_print(LOG_LEVEL_0, qzookeeper.__LOGTAG__, 'qzookeeper client authentication failed.');
        });

        this.client.on('session_expired', () => {
            debug_print(LOG_LEVEL_0, qzookeeper.__LOGTAG__, 'qzookeeper session expired.');
            this.reconnect();
        });

        // this.client.on('error', (err: Error | Exception) => {
        //     // debug_print(LOG_LEVEL_4, qzookeeper.__LOGTAG__, `Zookeeper client encountered an error: ${err.message}`);
        // });
    }
    
    register_value_change_callback(callback: type_qzk_value_changed, context: any): void {
        if (this.value_change_callbacks.has(callback)) {
            return; // Callback already registered
        }
        this.value_change_callbacks.set(callback, context);
    }
    unregister_value_change_callback(callback: type_qzk_value_changed, context: any): void {
        if (this.value_change_callbacks.has(callback)) {
            this.value_change_callbacks.delete(callback);
        }
    }

    /**
     * Connects to the ZooKeeper server.
     * 
     * @returns A promise that resolves when the client successfully connects.
     */
    public connect(): Promise<void> {
        return new Promise((resolve, reject) => {
            this.client.connect();
            debug_print(LOG_LEVEL_4, qzookeeper.__LOGTAG__, 'qzookeeper trying to connect');

            // Listen for the 'connected' event to confirm the connection is established
            this.client.once('connected', () => {
                resolve();
            });
        });
    }

    private reconnect(): void {
        debug_print(LOG_LEVEL_4, qzookeeper.__LOGTAG__, 'Closing current session...');
        // Store all the watchers before closing the client
        const watchers_to_restore = new Map(this.active_watchers);
        this.client.close();
        this.client = zookeeper.createClient(this.zk_connection_string);
        this.connect().then(() => {
            debug_print(LOG_LEVEL_0, qzookeeper.__LOGTAG__, 'Reconnected to ZooKeeper');
            // Reapply the stored watchers to the new client
            watchers_to_restore.forEach((watcher, path) => {
                this.set_watcher(path);
            });
        }).catch((err) => {
            console.error('Failed to reconnect:', err.message);
        });
    }

    public get_data(zk_path: string, get_data_callback: (error: Error | null, data: Buffer) => void, default_value: string = '{}') {
        const attempt_get_data = (retry_count: number) => {
            if (retry_count <= this.retry_attempts) {
                this.client.getData(zk_path, (error: Error | Exception, data: Buffer, stat: zookeeper.Stat) => {
                    if (error) {
                        // Ignore NO_NODE exception (typically means the node doesn't exist)
                        if (error instanceof zookeeper.Exception && error.code === zookeeper.Exception.NO_NODE) {
                            debug_warn(qzookeeper.__LOGTAG__, `Node does not exist at path ${zk_path}. Ignoring NO_NODE error.`);
                            get_data_callback(null, Buffer.from(default_value));  // Return default value or null data
                            return;
                        }
                        debug_warn(qzookeeper.__LOGTAG__, `Error fetching data (Attempt ${retry_count}):`, error);
                        setTimeout(() => attempt_get_data(retry_count + 1), this.retry_interval);
                    } else {
                        get_data_callback(null, data);
                        this.set_watcher(zk_path);
                    }
                });
            } else {
                debug_warn(qzookeeper.__LOGTAG__, 'Max retry attempts reached for get_data');
                get_data_callback(new Error('Max retry attempts reached'), Buffer.from(default_value));
            }
        };

        attempt_get_data(1);
    }

    /**
     * Sets a watcher on the specified ZooKeeper path to listen for changes.
     * When data changes, the registered callback will be invoked.
     * 
     * @param zk_path The path in ZooKeeper to watch.
     */
    public set_watcher(zk_path: string): void {
        const watcher = (event: zookeeper.Event) => {
            debug_print(LOG_LEVEL_4, qzookeeper.__LOGTAG__, `Watcher triggered for ${zk_path}. Event: ${event.type}`);
            // Fetch the updated data after the watch has been triggered
            this.client.getData(zk_path, (error: Error | Exception, data: Buffer) => {
                if (error) {
                    console.error(`Error fetching data after watcher triggered on ${zk_path}:`, error);
                } else {
                    // Trigger the callback for each registered watcher
                    this.value_change_callbacks.forEach((context, callback) => {
                        callback(zk_path, data.toString(), context);
                    });
                }
            });

            // Re-register the watch to continue watching for changes
            this.client.exists(zk_path, watcher, (err: Error | Exception | any, stat: Stat) => {
                if (err) {
                    console.error(`Error checking node existence: ${err.message}`);
                }/* else {
                    debug_print(LOG_LEVEL_4, qzookeeper.__LOGTAG__, `Node exists at ${zk_path}. Stat:`, stat);
                }*/
            });
        };

        // Store the watcher for the path
        this.active_watchers.set(zk_path, watcher);
        // Start the watch by checking if the node exists
        this.client.exists(zk_path, watcher, (err: Error | Exception | any, stat: Stat) => {
            if (err) {
                console.error(`Error checking node existence: ${err.message}`);
            } /*else {
                debug_print(LOG_LEVEL_4, qzookeeper.__LOGTAG__, `Node exists at ${zk_path}. Stat:`, stat);
            }*/
        });
    }

    public close(): void {
        this.close_issued = true;
        this.client.close();
        debug_print(LOG_LEVEL_0, qzookeeper.__LOGTAG__, 'qzookeeper close');
    }
}

export default qzookeeper;


// const zkClient = new ZooKeeperClient('localhost:2181');
// zkClient.connect();

// zkClient.get_data('/my/path', (err, data) => {
//     if (err) {
//         console.error('Error:', err);
//     } else {
//         debug_print(LOG_LEVEL_4, userserver.__LOGTAG__, 'Data:', data.toString());
//     }
// });
