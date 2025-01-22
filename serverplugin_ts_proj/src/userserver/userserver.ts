import * as ffi from 'ffi-napi';
import { serversdk } from '../helpers/serversdk';
import * as ref from 'ref-napi';
import { essentials } from '../helpers/essentials';
import { qmongo } from '../helpers/qmongo';
import qhiredis from '../helpers/qhiredis';
import qzookeeper from '../helpers/qzookeeper';
import { debug_print, debug_error, LOG_LEVEL_0, LOG_LEVEL_4 } from '../helpers/sdktypes';
import { serverconfig } from '../helpers/serverconfig';
import * as path from 'path';
import * as filelogger from '../helpers/filelogger';
import { server_config_reader } from '../helpers/serverconfig-reader';

export namespace server {
    export type type_api_callback = (native_server: serversdk.qh3server_ptr, cid: Buffer, cid_len: number, user_server_interface: interface_userserver, api_instance: interface_api, path: string, buffer: string, len: number, headers: string, header_buffer_size: number) => Promise<string | null | any>;

    export interface interface_userserver {
        get_mongo_driver(): qmongo | null;
        get_hiredis_driver(): qhiredis | null;
        get_qzookeeper_driver(): qzookeeper | null;
        get_zkconfig(): serverconfig | null;
    }

    export interface interface_api {
        get_path(): string;
        get_post_cb(): type_api_callback;
    }

    export class userserver implements interface_userserver {
        private static __LOGTAG__: string = `userserver`;
        private router_config: serversdk.qh3_router_input_config = {
            router_address: server_config_reader.get_instance().get_value('router_address'),
            mongodb_uri: server_config_reader.get_instance().get_value('router_mongodb_uri'),
            redis_address: server_config_reader.get_instance().get_value('router_redis_uri'),
            zk_uri: server_config_reader.get_instance().get_value('router_zk_uri'),
            root_dir: process.cwd(),
            command_port: server_config_reader.get_instance().get_value_as_number('command_port', 4010),
            router_port_return: server_config_reader.get_instance().get_value_as_number('router_port_return', 4005),
            app_id: server_config_reader.get_instance().get_value('app_id')
        };

        private mongo: qmongo | null = null;
        private hiredis: qhiredis | null = null;
        private qzk: qzookeeper | null = null;
        private zkconfig: serverconfig | null = null;
        private api_callbacks: Map<string, interface_api> = new Map();

        private start_time: number;
        private request_counter: number;
        private total_execution_time: number;

        constructor() {
            this.start_time = Date.now(); // Initialize the start time
            this.request_counter = 0;     // Initialize the request counter
            this.total_execution_time = 0; // Initialize the total execution time
        }

        public get_mongo_driver(): qmongo | null {
            return this.mongo;
        }
        public get_hiredis_driver(): qhiredis | null {
            return this.hiredis;
        }
        public get_qzookeeper_driver(): qzookeeper | null {
            return this.qzk;
        }
        public get_zkconfig(): serverconfig | null {
            return this.zkconfig;
        }

        protected on_server_pre_start = ffi.Callback('void', ['pointer', 'pointer'], (native_server: serversdk.qh3server_ptr, user_arg: Buffer) => {
            debug_print(LOG_LEVEL_4, userserver.__LOGTAG__, `on_server_pre_start`);
        }) as unknown as serversdk.type_on_server_pre_start;
        protected on_server_start = ffi.Callback('void', ['pointer', 'pointer', 'string', serversdk.uint16], async (native_server: serversdk.qh3server_ptr, user_arg: Buffer, ip: string, port: number) => {
            debug_print(LOG_LEVEL_4, userserver.__LOGTAG__, `on_server_start`);
            setImmediate(async () => {
                await this.hiredis?.set_hash_value(`servers:${serversdk.serverplugin.get_device_public_ip()}`, `server-${port}`, `${ip}:${port}`);
            });
        }) as unknown as serversdk.type_on_server_start;
        protected on_server_stop = ffi.Callback('void', ['pointer', 'pointer'], async (native_server: serversdk.qh3server_ptr, user_arg: Buffer) => {
            debug_print(LOG_LEVEL_4, userserver.__LOGTAG__, `on_server_stop:`);
        }) as unknown as serversdk.type_on_server_stop;
        protected on_server_error = ffi.Callback('void', ['pointer', 'pointer'], (native_server: serversdk.qh3server_ptr, user_arg: Buffer, error_code: number) => {
            debug_error(userserver.__LOGTAG__, `on_server_error: ${error_code}`);
        }) as unknown as serversdk.type_on_server_error;
        protected on_server_parse = ffi.Callback('void', ['pointer', 'pointer', serversdk.uint8_p, serversdk.uint16, 'string', 'string', serversdk.size_t, 'string', serversdk.size_t], async (native_server: serversdk.qh3server_ptr, user_arg: Buffer, cid: Buffer, cid_len: number, path: string, buffer: string, len: number, headers: string, header_buffer_size: number) => {
            // debug_print(LOG_LEVEL_4, userserver.__LOGTAG__, `on_server_parse: ${path}, ${buffer}, len ${len}`);
            this.request_counter = this.request_counter + 1
            const parse_start_time = Date.now();
            const cached_cid = Buffer.from(ref.reinterpret(cid, cid_len));

            // setImmediate(async () => {
            let result: string | null = null;
            if (this.api_callbacks.has(path)) {
                let api_instance: interface_api | any = this.api_callbacks.get(path);
                result = await api_instance?.get_post_cb()?.(native_server, cached_cid, cached_cid.length, this, api_instance, path, buffer, len, headers, header_buffer_size);
            }
            if (result) {
                serversdk.serverplugin.qh3server_try_send_response(native_server, cached_cid, cached_cid.length, result, result.length, null, 0);
            } else {
                serversdk.serverplugin.qh3server_try_send_response(native_server, cached_cid, cached_cid.length, `{}`, 2, null, 0);
            }
            const execution_time = Date.now() - parse_start_time
            this.total_execution_time = this.total_execution_time + execution_time
            this.calculate_rps()
            // });
        }) as unknown as serversdk.type_on_server_parse;

        public register_api(api_instance: interface_api): void {
            if (this.api_callbacks.has(api_instance.get_path())) {
                return;
            }
            this.api_callbacks.set(api_instance.get_path(), api_instance);
            debug_print(LOG_LEVEL_0, userserver.__LOGTAG__, `api registered - ${api_instance.get_path()}`);
        }

        public unregister_api(path: string): void {
            if (this.api_callbacks.has(path)) {
                this.api_callbacks.delete(path);
                debug_print(LOG_LEVEL_0, userserver.__LOGTAG__, `api un-registered - ${path}`);
            }
        }

        public unregister_api_instance(api_instance: interface_api): void {
            if (this.api_callbacks.has(api_instance.get_path())) {
                this.api_callbacks.delete(api_instance.get_path());
                debug_print(LOG_LEVEL_0, userserver.__LOGTAG__, `api un-registered - ${api_instance.get_path()}`);
            }
        }

        private calculate_rps(): void {
            const current_time = Date.now(); // Get current time in milliseconds
            const elapsed_time_ms = current_time - this.start_time;

            if (elapsed_time_ms >= 1000) {
                const rps = (this.request_counter * 1000) / elapsed_time_ms;
                const avg_execution_time = this.total_execution_time / Math.max(this.request_counter, 1); // Avoid division by zero

                // Output the results to the console
                process.stdout.write(`\rRequests per second: ${rps.toFixed(2)} | Avg execution time: ${avg_execution_time.toFixed(2)} ms`);
                // process.stdout.write(`\rRequests per second: ${rps.toFixed(2)}`); // Uncomment if you don't need avg execution time
                process.stdout.write('\n'); // Ensure the output is written to the terminal

                // Reset the counter and time for the next interval
                this.request_counter = 0;
                this.total_execution_time = 0;
                this.start_time = current_time;
            }
        }

        public async run(native_router: Buffer): Promise<void> {
            // mongo setup
            this.mongo = new qmongo("", "gsdk_mongodb", this.router_config.mongodb_uri);
            await this.mongo?.connect();

            // redis setup
            const redis_ip_port = essentials.extract_ip_and_port(this.router_config.redis_address);
            if (redis_ip_port) {
                const [ip, port]: [string, number] = redis_ip_port;
                this.hiredis = new qhiredis("hiredis", ip, port, "gsdkuser", "Fr0gmoon123");
                await this.hiredis.connect_redis();
            } else {
                debug_error(userserver.__LOGTAG__, "Invalid hiredis address format");
                return;
            }

            // zookeeper setup
            const zk_ip_port = essentials.extract_ip_and_port(this.router_config.zk_uri);
            if (zk_ip_port) {
                const [ip, port]: [string, number] = zk_ip_port;
                this.qzk = new qzookeeper(this.router_config.zk_uri);
                await this.qzk.connect();
            } else {
                debug_error(userserver.__LOGTAG__, "Invalid zk address format");
                return;
            }

            // load server config
            this.zkconfig = new serverconfig(this.qzk, null);
            const config_path = path.join(this.router_config.root_dir, 'configs', 'dev', 'runtime-config.json');
            debug_print(LOG_LEVEL_4, userserver.__LOGTAG__, `reading ${config_path}`);
            await this.zkconfig.load(config_path, this.qzk, `/qh3server`);

            // server spawn
            serversdk.serverplugin.spawn_qh3server(
                native_router,
                this.router_config.router_address,
                this.router_config.mongodb_uri,
                this.router_config.redis_address,
                this.router_config.zk_uri,
                this.router_config.root_dir,
                this.router_config.command_port,
                this.router_config.router_port_return,
                this.router_config.app_id,
                this.on_server_pre_start,
                this.on_server_start,
                this.on_server_stop,
                this.on_server_error,
                this.on_server_parse,
                serversdk.nullptr
            );
        }
    }
}

export default server;