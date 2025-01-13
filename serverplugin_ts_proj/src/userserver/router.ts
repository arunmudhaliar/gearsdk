import * as ffi from 'ffi-napi';
import { serversdk } from '../helpers/serversdk';
import { debug_error, debug_print, LOG_LEVEL_0, LOG_LEVEL_4 } from '../helpers/sdktypes';
import { server_config_reader } from '../helpers/serverconfig-reader';

export namespace server {
    export class router {
        private static __LOGTAG__: string = `router`;
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
        private on_router_start_cb: (native_router: Buffer) => Promise<void>;
        constructor(on_router_start_cb: (native_router: Buffer) => Promise<void>) {
            this.on_router_start_cb = on_router_start_cb;
        }

        // router events
        protected on_router_pre_start = ffi.Callback('void', ['pointer', 'pointer'], (native_router: Buffer, user_arg: Buffer) => {
            debug_print(LOG_LEVEL_0, router.__LOGTAG__, `on_router_pre_start`);
        }) as unknown as serversdk.type_on_router_pre_start;
        protected on_router_start = ffi.Callback('void', ['pointer', 'pointer'], (native_router: Buffer, user_arg: Buffer) => {
            debug_print(LOG_LEVEL_0, router.__LOGTAG__, `on_router_start`);
            this.on_router_start_cb(native_router);
        }) as unknown as serversdk.type_on_router_start;
        protected on_router_stop = ffi.Callback('void', ['pointer', 'pointer'], (native_router: Buffer, user_arg: Buffer) => {
            debug_print(LOG_LEVEL_0, router.__LOGTAG__, `on_router_stop:`);
        }) as unknown as serversdk.type_on_router_stop;
        protected on_router_error = ffi.Callback('void', ['pointer', 'pointer', 'int'], (native_router: Buffer, user_arg: Buffer, error_code: number) => {
            debug_error(router.__LOGTAG__, `on_router_error: ${error_code}`);
        }) as unknown as serversdk.type_on_router_error;

        public async run(): Promise<void> {
            serversdk.serverplugin.spawn_qh3router(
                this.router_config.router_address,
                this.router_config.mongodb_uri,
                this.router_config.redis_address,
                this.router_config.zk_uri,
                this.router_config.root_dir,
                this.router_config.command_port,
                this.router_config.router_port_return,
                this.router_config.app_id,
                this.on_router_pre_start,
                this.on_router_start,
                this.on_router_stop,
                this.on_router_error,
                serversdk.nullptr
            );
        }
    }
}