import { serversdk } from '../helpers/libserverplugin';
import { debug_error, debug_print, LOG_LEVEL_0, LOG_LEVEL_4 } from '../helpers/sdktypes';
import { server_inf_reader } from '../helpers/serverinforeader';

export namespace server {
    export class router {
        private static __LOGTAG__: string = `router`;
        private router_config: serversdk.qh3_router_input_config = {
            router_address: server_inf_reader.get_instance().get_value('router_address'),
            mongodb_uri: server_inf_reader.get_instance().get_value('router_mongodb_uri'),
            redis_address: server_inf_reader.get_instance().get_value('router_redis_uri'),
            zk_uri: server_inf_reader.get_instance().get_value('router_zk_uri'),
            root_dir: process.cwd(),
            command_port: server_inf_reader.get_instance().get_value_as_number('command_port', 4010),
            router_port_return: server_inf_reader.get_instance().get_value_as_number('router_port_return', 4005),
            app_id: server_inf_reader.get_instance().get_value('app_id')
        };
        private on_router_start_cb: (native_router: any) => Promise<void>;
        private on_router_stop_cb: (native_router: any) => Promise<void>;
        private cd_data: any = null;
        constructor(on_router_start_cb: (native_router: any) => Promise<void>, on_router_stop_cb: (native_router: any) => Promise<void>) {
            this.on_router_start_cb = on_router_start_cb;
            this.on_router_stop_cb = on_router_stop_cb;
        }

        // router events
        protected on_router_pre_start = (native_router: any, cd_data: any) => {
            debug_print(LOG_LEVEL_0, router.__LOGTAG__, `on_router_pre_start`);
            this.cd_data = cd_data;
        }
        protected on_router_start = (native_router: any) => {
            debug_print(LOG_LEVEL_0, router.__LOGTAG__, `on_router_start`);
            this.on_router_start_cb(native_router);
        }
        protected on_router_stop = (native_router: any) => {
            debug_print(LOG_LEVEL_0, router.__LOGTAG__, `on_router_stop:`);
            this.on_router_stop_cb(native_router);
            serversdk.sdklib.qh3router_release_callbacks(this.cd_data);
        }
        protected on_router_error = (router: any, error_code: any) => {
            debug_error(router.__LOGTAG__, `on_router_error: ${error_code}`);
        }

        public async run(): Promise<void> {
            serversdk.sdklib.spawn_qh3router(
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
                this.on_router_error
            );
        }
    }
}