// import * as path from 'path';
// import { sdklib } from './helpers/libserverplugin';
// // import * as serverplugin_addon from './helpers/libserverplugin-debug';

// // const serverplugin_addon = require(path.resolve(__dirname, '../../serverplugin/libserverplugin-debug.node'));
// // serverplugin_addon.setup_signal_handler();
// // serverplugin_addon.pre_init_serverplugin_sdk();

// // serverplugin_addon.serverplugin_addon.spawn_qh3router
// sdklib.spawn_qh3router(
//     "127.0.0.1:4004",
//     "mongodb://3.109.144.159:27017",
//     "3.109.144.159:6379",
//     "3.109.144.159:2181",
//     process.cwd(),
//     4010,
//     4005,
//     "app_id",
//     (router: any, userArg: any) => {
//         // serverplugin_addon.set_pointer(router);
//         console.log("TESSRRRR");
//         console.log(`Pre-start callback ${router}, ${userArg}`);
//     },
//     (router: any, userArg: any) => {
//         console.log("Start callback");
//     },
//     (router: any, userArg: any) => {
//         console.log("Stop callback");
//     },
//     (router: any, userArg: any, err: any) => {
//         console.error("Error callback", err);
//     }
// );

// const tname = serverplugin_addon.get_thread_name();
// console.log('The result is:', tname);
// const result = serverplugin_addon.add(7, 5, (result: number) => {
//     serverplugin_addon.test()
//     console.log('The result is:', result);  // Output: The result is: 7
// });
// serverplugin_addon.set_thread_name('test_thread');
// const ttt = serverplugin_addon.get_thread_name();
// console.log('The result is:', ttt);
// console.log(`7 + 5 = ${result}`);


// const pp = serverplugin_addon.get_pointer();
// console.log('The result is:', pp);
// serverplugin_addon.set_pointer(pp);
// const ppp = serverplugin_addon.get_pointer();
// console.log('The result is:', ppp);

// import * as ref from 'ref-napi';
import { server as routerserver } from "./userserver/router";
import { server as qh3server } from './userserver/userserver';
import { server as qserver } from './gameserver/gameserver';
import api_user_get from './features/user_get/api_user_get';
import api_whoami from './features/user_get/api_whoami';
import api_ping from './features/user_get/api_ping';
import { debug_error } from './helpers/sdktypes';
import { server_config_reader } from './helpers/serverconfig-reader';
import { custom_gameserver } from './gameroom/custom_gameserver';
// import * as path from 'path';

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
        private async on_router_start_cb(native_router: any) {
            server_app.instance.start_userserver(native_router);
            server_app.instance.start_userserver(native_router);
            server_app.instance.start_userserver(native_router);
        }

        private async start_router(): Promise<void> {
            if (this.router_instance != null) {
                debug_error(server_app.__LOGTAG__, `router_instance not null !!!`);
                return;
            }
            this.router_instance = new routerserver.router(this.on_router_start_cb);
            this.router_instance.run();
        }

        private async start_userserver(native_router: any = null): Promise<void> {
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
                // await this.start_router();
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
