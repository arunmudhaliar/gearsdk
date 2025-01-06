import * as ref from 'ref-napi';
import { server as routerserver } from "./userserver/router";
import { server as qh3server } from './userserver/userserver';
import api_user_get from './features/user_get/api_user_get';
import api_whoami from './features/user_get/api_whoami';
import api_ping from './features/user_get/api_ping';

namespace app {
    export class qh3serverplugin_app {
        private static instance: qh3serverplugin_app;
        constructor() {
            qh3serverplugin_app.instance = this;
        }
        public get_instance(): qh3serverplugin_app {
            return qh3serverplugin_app.instance;
        }
        private async on_router_start_cb(native_router: Buffer) {
            qh3serverplugin_app.instance.start_userserver(native_router);
            qh3serverplugin_app.instance.start_userserver(native_router);
            qh3serverplugin_app.instance.start_userserver(native_router);
        }

        private async start_router(): Promise<void> {
            let router_instance: routerserver.router = new routerserver.router(this.on_router_start_cb);
            router_instance.run();
        }

        private async start_userserver(native_router: Buffer = ref.NULL): Promise<void> {
            let userserver_instance: qh3server.userserver = new qh3server.userserver();
            userserver_instance.register_api(new api_whoami());
            userserver_instance.register_api(new api_ping());
            userserver_instance.register_api(new api_user_get());
            userserver_instance.run(native_router);
        }

        public async run(): Promise<void> {
            await this.start_router();
            // await this.start_userserver();
        }
    }
}

export const app_instance = new app.qh3serverplugin_app();
app_instance.run();
setInterval(() => {
    console.log('Keep-alive ping...');
}, 5 * 60 * 1000); // Every 5 minute
