import * as ffi from 'ffi-napi';
import { qh3serversdk } from '../helpers/qh3serversdk';
import * as ref from 'ref-napi';
import { essentials } from '../helpers/essentials';
import { qmongo } from '../helpers/qmongo';
import qhiredis from '../helpers/qhiredis';
import qzookeeper from '../helpers/qzookeeper';
import { debug_print, debug_error, LOG_LEVEL_0, LOG_LEVEL_4 } from '../helpers/sdktypes';
import { serverconfig } from '../helpers/serverconfig';
import * as path from 'path';

export namespace server {
    type type_api_callback = (user_server_interface: interface_userserver, api_instance: interface_api, path: string, buffer: string, len: number) => Promise<string | null | any>;

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
        private router_config : qh3serversdk.qh3_router_input_config = {
            router_address: `127.0.0.1:4004`,
            mongodb_uri: `mongodb://3.109.144.159:27017`,
            redis_address: `3.109.144.159:6379`,
            zk_uri: `3.109.144.159:2181`,
            root_dir: process.cwd(),
            command_port: 4010,
            router_port_return: 4005,
            app_id: `qh3serverplugin-app`
        };
        private mongo : qmongo | null = null;
        private hiredis: qhiredis | null = null;
        private qzk: qzookeeper | null = null;
        private zkconfig: serverconfig | null = null;
        private api_callbacks: Map<string, interface_api> = new Map();

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

        protected on_server_pre_start = ffi.Callback('void', ['pointer'], (native_server: Buffer) => {
            // debug_print(LOG_LEVEL_4, userserver.__LOGTAG__, `on_server_pre_start`);
        }) as unknown as qh3serversdk.type_on_server_pre_start;
        protected on_server_start = ffi.Callback('void', ['pointer'], async (native_server: Buffer) => {
            // debug_print(LOG_LEVEL_4, userserver.__LOGTAG__, `on_server_start`);
        }) as unknown as qh3serversdk.type_on_server_start;
        protected on_server_stop = ffi.Callback('void', ['pointer'], (native_server: Buffer) => {
            // debug_print(LOG_LEVEL_4, userserver.__LOGTAG__, `on_server_stop:`);
        }) as unknown as qh3serversdk.type_on_server_stop;
        protected on_server_error = ffi.Callback('void', ['pointer'], (native_server: Buffer, error_code: number) => {
            // debug_error(userserver.__LOGTAG__, `on_server_error: ${error_code}`);
        }) as unknown as qh3serversdk.type_on_server_error;
        protected on_server_parse = ffi.Callback('void', ['pointer', 'pointer', 'string', 'string', 'int'], async (native_server: Buffer, conn: Buffer, path: string, buffer: string, len: number) => {
            debug_print(LOG_LEVEL_4, userserver.__LOGTAG__, `on_server_parse: ${path}, ${buffer}, len ${len}`);
            let result: string | null = null;

            if (this.api_callbacks.has(path)) {
                let api_instance: interface_api | any= this.api_callbacks.get(path);
                result = await api_instance?.get_post_cb()?.(this, api_instance, path, buffer, len);
            }

            if (result) {
                qh3serversdk.qh3serverplugin.qh3server_try_send_response(native_server, conn, result, result.length, null, 0);
            } else {
                qh3serversdk.qh3serverplugin.qh3server_try_send_response(native_server, conn, `{}`, 2, null, 0);
            }

            // const response_string = `{msg:\"test message\"}`;
            // // Use strdup to allocate and return a copy of the string
            // result = qh3serversdk.libc.strdup(response_string);
            // if (result.isNull()) {
            //     throw new Error('Memory allocation failed using strdup.');
            // }
            // return result; // Return the pointer to C++
        }) as unknown as qh3serversdk.type_on_server_parse;

        public register_api(api_instance : interface_api) : void {
            if (this.api_callbacks.has(api_instance.get_path())) {
                return;
            }
            this.api_callbacks.set(api_instance.get_path(), api_instance);
            debug_print(LOG_LEVEL_0, userserver.__LOGTAG__, `api registered - ${api_instance.get_path()}`);
        }

        public unregister_api(path:string) : void {
            if (this.api_callbacks.has(path)) {
                this.api_callbacks.delete(path);
                debug_print(LOG_LEVEL_0, userserver.__LOGTAG__, `api un-registered - ${path}`);
            }
        }

        public unregister_api_instance(api_instance : interface_api) : void {
            if (this.api_callbacks.has(api_instance.get_path())) {
                this.api_callbacks.delete(api_instance.get_path());
                debug_print(LOG_LEVEL_0, userserver.__LOGTAG__, `api un-registered - ${api_instance.get_path()}`);
            }
        }

        public async run(native_router: Buffer) : Promise<void> {
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
            qh3serversdk.qh3serverplugin.spawn_qh3server(
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
                this.on_server_parse
            );
        }
    }
}

export default server;