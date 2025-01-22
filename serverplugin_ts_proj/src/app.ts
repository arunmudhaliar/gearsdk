import * as ref from 'ref-napi';
import { server as routerserver } from "./userserver/router";
import { server as qh3server } from './userserver/userserver';
import { server as qserver } from './gameserver/gameserver';
import api_user_get from './features/user_get/api_user_get';
import api_whoami from './features/user_get/api_whoami';
import api_ping from './features/user_get/api_ping';
import { debug_error } from './helpers/sdktypes';
import { server_config_reader } from './helpers/serverconfig-reader';
import * as path from 'path';
import { custom_gameserver } from './gameroom/custom_gameserver';

namespace app {
    export class server_app {
        private static __LOGTAG__: string = `server_app`;
        private static instance: server_app;
        private custom_gameserver_instance?: custom_gameserver;
        private router_instance?: routerserver.router;
        private userserver_instances?: qh3server.userserver[] = [];

        constructor() {
            server_app.instance = this;
        }
        public get_instance(): server_app {
            return server_app.instance;
        }
        private async on_router_start_cb(native_router: Buffer) {
            server_app.instance.start_userserver(native_router);
            // server_app.instance.start_userserver(native_router);
            // server_app.instance.start_userserver(native_router);
        }

        private async start_router(): Promise<void> {
            if (this.router_instance != null) {
                debug_error(server_app.__LOGTAG__, `router_instance not null !!!`);
                return;
            }
            this.router_instance = new routerserver.router(this.on_router_start_cb);
            this.router_instance.run();
        }

        private async start_userserver(native_router: Buffer = ref.NULL): Promise<void> {
            let userserver_instance: qh3server.userserver = new qh3server.userserver();
            userserver_instance.register_api(new api_whoami());
            userserver_instance.register_api(new api_ping());
            userserver_instance.register_api(new api_user_get());
            userserver_instance.run(native_router);
            this.userserver_instances?.push(userserver_instance);
        }

        private async start_gameserver(): Promise<void> {
            if (this.custom_gameserver_instance != null) {
                debug_error(server_app.__LOGTAG__, `custom_gameserver_instance not null !!!`);
                return;
            }
            this.custom_gameserver_instance = new custom_gameserver();
            await this.custom_gameserver_instance.start_custom_gameserver();
        }

        public async run(): Promise<void> {
            try {
                await this.start_router();
                // await this.start_userserver();
                // await this.start_gameserver();
            } catch (error) {
                debug_error(server_app.__LOGTAG__, `Error starting server: ${error}`);
            }
        }
    }
}

if (process.env.NODE_ENV === "production") {
    console.log("Running in production mode");
    server_config_reader.get_instance().load_config('./serversonfig.rel.inf');
} else {
    console.log("Running in development mode");
    server_config_reader.get_instance().load_config('./serversonfig.rel.inf');
}

export const app_instance = new app.server_app();
app_instance.run();
setInterval(() => {
    console.log('Keep-alive ping...');
}, 5 * 60 * 1000); // Every 5 minute
