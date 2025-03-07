import { api_ping, api_user_get, api_whoami, custom_gameserver, debug_error, debug_print, LOG_LEVEL_0, qh3server, routerserver, server_info_reader } from '@amudaliar/gsdk-serverplugin';

namespace app {
    export class server_app {
        private static __LOGTAG__: string = `server_app`;
        private static instance: server_app;
        private custom_gameserver_instance?: custom_gameserver;
        private router_instance?: routerserver.router;
        private userserver_instances?: qh3server.userserver[] = [];
        public static keep_alive_timer: any;
        public static active_router: number = 0;
        public static active_server: number = 0;
        public static active_gameserver: number = 0;

        constructor() {
            server_app.instance = this;
        }
        public get_instance(): server_app {
            return server_app.instance;
        }
        private static check_and_clear_keep_alive_timer(keep_alive_timer_release_in: number): void {
            if ((server_app.active_gameserver + server_app.active_router + server_app.active_server) <= 0) {
                debug_print(LOG_LEVEL_0, server_app.__LOGTAG__, `release the keep alive timer in ${keep_alive_timer_release_in / 1000} seconds ...`);
                setTimeout(() => {
                    clearInterval(app.server_app.keep_alive_timer);
                    debug_print(LOG_LEVEL_0, server_app.__LOGTAG__, `clear keep alive timer ${server_app.active_router}:${server_app.active_gameserver}`);
                }, keep_alive_timer_release_in);
            }
        }
        private async on_router_start_cb(native_router: any) {
            server_app.active_router++;
            server_app.instance.start_userserver(native_router);
            server_app.instance.start_userserver(native_router);
            server_app.instance.start_userserver(native_router);
        }

        private async on_router_stop_cb(native_router: any) {
            server_app.active_router--;
            server_app.check_and_clear_keep_alive_timer(10000);
        }

        private async start_router(): Promise<void> {
            if (this.router_instance != null) {
                debug_error(server_app.__LOGTAG__, `router_instance not null !!!`);
                return;
            }
            this.router_instance = new routerserver.router(this.on_router_start_cb, this.on_router_stop_cb);
            this.router_instance.run();
        }

        private async on_server_start_cb(native_server: any) {
            server_app.active_server++;
        }
        private async on_server_stop_cb(native_server: any) {
            server_app.active_server--;
            server_app.check_and_clear_keep_alive_timer(10000);
        }
        private async start_userserver(native_router: any = null): Promise<void> {
            let userserver_instance: qh3server.userserver = new qh3server.userserver(this.on_server_start_cb, this.on_server_stop_cb);
            userserver_instance.register_api(new api_whoami());
            userserver_instance.register_api(new api_ping());
            userserver_instance.register_api(new api_user_get());
            userserver_instance.run(native_router);
            this.userserver_instances?.push(userserver_instance);
        }

        private async on_custom_gameserver_start_cb(native_server: any) {
            server_app.active_gameserver++;
        }
        private async on_custom_gameserver_stop_cb(native_server: any) {
            server_app.active_gameserver--;
            server_app.check_and_clear_keep_alive_timer(10000);
        }
        private async start_gameserver(): Promise<void> {
            if (this.custom_gameserver_instance != null) {
                debug_error(server_app.__LOGTAG__, `custom_gameserver_instance not null !!!`);
                return;
            }
            this.custom_gameserver_instance = new custom_gameserver(this.on_custom_gameserver_start_cb, this.on_custom_gameserver_stop_cb);
            await this.custom_gameserver_instance.start_custom_gameserver();
        }

        public async run(): Promise<void> {
            try {
                await this.start_router();
                // await this.start_userserver();
                await this.start_gameserver();
            } catch (error) {
                debug_error(server_app.__LOGTAG__, `Error starting server: ${error}`);
            }
        }
    }
}

if (process.env.NODE_ENV === "production") {
    console.log("Running in production mode");
    server_info_reader.get_instance().load_config('./serverconfig.rel.inf');
} else {
    console.log("Running in development mode");
    server_info_reader.get_instance().load_config('./serverconfig.rel.inf');
}

export const app_instance = new app.server_app();
app_instance.run();

app.server_app.keep_alive_timer = setInterval(() => {
    console.log('Keep-alive ping...');
}, 5 * 60 * 1000); // Every 5 minute
console.log('Keep-alive exit...');
