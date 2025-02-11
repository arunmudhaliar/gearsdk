import { serversdk } from '../helpers/libserverplugin';
import { essentials } from '../helpers/essentials';
import { qmongo } from '../helpers/qmongo';
import qhiredis from '../helpers/qhiredis';
import qzookeeper from '../helpers/qzookeeper';
import { debug_print, debug_error, LOG_LEVEL_0, LOG_LEVEL_4 } from '../helpers/sdktypes';
import { serverconfig } from '../helpers/serverconfig';
import * as path from 'path';
import * as filelogger from '../helpers/filelogger';
import { server_info_reader } from '../helpers/serverinforeader';

export namespace server {
    export type type_api_callback = (native_server: serversdk.qh3server_ptr, cid: Buffer, cid_len: number, user_server_interface: interface_userserver, api_instance: interface_api, path: string, buffer: string, headers: string) => Promise<string | null | any>;

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
            router_address: server_info_reader.get_instance().get_value('userserver_address'),
            mongodb_uri: server_info_reader.get_instance().get_value('userserver_mongodb_uri'),
            mongodb_db: server_info_reader.get_instance().get_value('userserver_mongodb_name'),
            redis_address: server_info_reader.get_instance().get_value('userserver_redis_uri'),
            redis_user: server_info_reader.get_instance().get_value('userserver_redis_user'),
            redis_password: server_info_reader.get_instance().get_value('userserver_redis_password'),
            zk_uri: server_info_reader.get_instance().get_value('userserver_zk_uri'),
            root_dir: process.cwd(),
            inf_file: `${process.cwd()}/${process.env.NODE_ENV === "production" ? "serverconfig.rel.inf" : "serverconfig.dev.inf"}`,
            command_port: server_info_reader.get_instance().get_value_as_number('userserver_command_port', 4010),
            router_port_return: server_info_reader.get_instance().get_value_as_number('userserver_port_return', 4005),
            app_id: server_info_reader.get_instance().get_value('app_id')
        };

        private mongo: qmongo | null = null;
        private hiredis: qhiredis | null = null;
        private qzk: qzookeeper | null = null;
        private zkconfig: serverconfig | null = null;
        private api_callbacks: Map<string, interface_api> = new Map();

        private start_time: number;
        private request_counter: number;
        private total_execution_time: number;
        private on_server_start_cb: (native_server: serversdk.qh3server_ptr) => Promise<void>;
        private on_server_stop_cb: (native_server: serversdk.qh3server_ptr) => Promise<void>;

        constructor(on_server_start_cb: (native_server: serversdk.qh3server_ptr) => Promise<void>, on_server_stop_cb: (native_server: serversdk.qh3server_ptr) => Promise<void>) {
            this.start_time = Date.now(); // Initialize the start time
            this.request_counter = 0;     // Initialize the request counter
            this.total_execution_time = 0; // Initialize the total execution time
            this.on_server_start_cb = on_server_start_cb;
            this.on_server_stop_cb = on_server_stop_cb;
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

        protected on_server_pre_start = (native_server: serversdk.qh3server_ptr) => {
            debug_print(LOG_LEVEL_4, userserver.__LOGTAG__, `on_server_pre_start`);
        }
        protected on_server_start = async (native_server: serversdk.qh3server_ptr, ip: string, port: number) => {
            debug_print(LOG_LEVEL_4, userserver.__LOGTAG__, `on_server_start`);
            await this.hiredis?.set_hash_value(`servers:${serversdk.sdklib.get_device_public_ip()}`, `server-${port}`, `${ip}:${port}`);
            this.on_server_start_cb(native_server);
        }
        protected on_server_stop = (native_server: serversdk.qh3server_ptr) => {
            debug_print(LOG_LEVEL_4, userserver.__LOGTAG__, `on_server_stop:`);
            serversdk.sdklib.qh3server_release_callbacks(native_server);
            this.on_server_stop_cb(native_server);
            this.mongo?.disconnect();
            this.hiredis?.disconnect_redis();
            this.qzk?.close();
        }
        protected on_server_error = (native_server: serversdk.qh3server_ptr, error_code: number) => {
            debug_error(userserver.__LOGTAG__, `on_server_error: ${error_code}`);
        }
        protected on_server_parse = async (native_server: serversdk.qh3server_ptr, cid: any, cid_len: number, path: string, buffer: string, headers: string) => {
            // debug_print(LOG_LEVEL_0, userserver.__LOGTAG__, `on_server_parse: ${path}, ${buffer}`);
            this.process_request(native_server, cid, cid_len, path, buffer, headers);
        }

        private async process_request(native_server: serversdk.qh3server_ptr, cid: any, cid_len: number, path: string, buffer: string, headers: string) {
            this.request_counter = this.request_counter + 1
            const parse_start_time = Date.now();
            // const cached_cid = Buffer.from(ref.reinterpret(cid, cid_len));
            let result: string | null = null;
            if (this.api_callbacks.has(path)) {
                let api_instance: interface_api | any = this.api_callbacks.get(path);
                result = await api_instance?.get_post_cb()?.(native_server, cid, cid_len, this, api_instance, path, buffer, headers);
            }
            if (result) {
                serversdk.sdklib.qh3server_try_send_response(native_server, cid, cid_len, result, result.length);
            } else {
                serversdk.sdklib.qh3server_try_send_response(native_server, cid, cid_len, `{}`, 2);
            }
            const execution_time = Date.now() - parse_start_time
            this.total_execution_time = this.total_execution_time + execution_time
            this.calculate_rps()
            // filelogger.QH3_INFO(native_server, userserver.__LOGTAG__, "FROM TS");
        }

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

        public async run(native_router: any): Promise<void> {
            // mongo setup
            this.mongo = new qmongo("", this.router_config.mongodb_db, this.router_config.mongodb_uri);
            await this.mongo?.connect();

            // redis setup
            const redis_ip_port = essentials.extract_ip_and_port(this.router_config.redis_address);
            if (redis_ip_port) {
                const [ip, port]: [string, number] = redis_ip_port;
                this.hiredis = new qhiredis("hiredis", ip, port, this.router_config.redis_user, this.router_config.redis_password);
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
            const config_path = path.join(this.router_config.root_dir, 'configs', (process.env.NODE_ENV === 'production') ? 'prod' : 'dev', 'runtime-config.json');
            debug_print(LOG_LEVEL_4, userserver.__LOGTAG__, `reading ${config_path}`);
            let zk_root_node: string = server_info_reader.get_instance().get_value('userserver_zk_root_node');
            await this.zkconfig.load(config_path, this.qzk, zk_root_node);

            // server spawn
            serversdk.sdklib.spawn_qh3server(
                native_router,
                this.router_config.router_address,
                this.router_config.mongodb_uri,
                this.router_config.redis_address,
                this.router_config.zk_uri,
                this.router_config.root_dir,
                this.router_config.inf_file,
                this.router_config.command_port,
                this.router_config.router_port_return,
                this.router_config.app_id,
                this.on_server_pre_start,
                this.on_server_start,
                this.on_server_stop,
                this.on_server_error,
                this.on_server_parse
            );
        }
    }
}

export default server;