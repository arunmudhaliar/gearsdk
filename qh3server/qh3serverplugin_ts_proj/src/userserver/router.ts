import * as ffi from 'ffi-napi';
import { qh3serversdk } from '../helpers/qh3serversdk';
import { debug_error, debug_print, LOG_LEVEL_4 } from '../helpers/sdktypes';

export namespace server {
    export class router {
        private static __LOGTAG__: string = `router`;
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
        private on_router_start_cb: (native_router: Buffer) => Promise<void>;
        constructor(on_router_start_cb:(native_router: Buffer) => Promise<void>) {
            this.on_router_start_cb = on_router_start_cb;
        }

        public async run() : Promise<void> {
            // router events
            const on_router_pre_start = ffi.Callback('void', ['pointer'], (native_router: Buffer) => {
                debug_print(LOG_LEVEL_4, router.__LOGTAG__, `on_router_pre_start`);
            }) as unknown as qh3serversdk.type_on_router_pre_start;
            const on_router_start = ffi.Callback('void', ['pointer'], (native_router: Buffer) => {
                debug_print(LOG_LEVEL_4, router.__LOGTAG__, `on_router_start`);
                this.on_router_start_cb(native_router);
            }) as unknown as qh3serversdk.type_on_router_start;
            const on_router_stop = ffi.Callback('void', [], () => {
                debug_print(LOG_LEVEL_4, router.__LOGTAG__, `on_router_stop:`);
            }) as unknown as qh3serversdk.type_on_router_stop;
            const on_router_error = ffi.Callback('void', [], (error_code: number) => {
                debug_error(router.__LOGTAG__, `on_router_error: ${error_code}`);
            }) as unknown as qh3serversdk.type_on_router_error;

            qh3serversdk.qh3serverplugin.spawn_qh3router(
                this.router_config.router_address,
                this.router_config.mongodb_uri,
                this.router_config.redis_address,
                this.router_config.zk_uri,
                this.router_config.root_dir,
                this.router_config.command_port,
                this.router_config.router_port_return,
                this.router_config.app_id,
                on_router_pre_start,
                on_router_start,
                on_router_stop,
                on_router_error
            );
        }
    }
}