import * as ref from 'ref-napi';
import { server as routerserver} from "./userserver/router";
import { server as qh3server} from './userserver/userserver';
import api_user_get from './features/user_get/api_user_get';

namespace app {
    export class qh3serverplugin_app {
        constructor(){
        }

        private async on_router_start_cb(native_router: Buffer) {
            this.start_userserver();
        }
        private async start_router() : Promise<void> {
            let router_instance: routerserver.router = new routerserver.router(this.on_router_start_cb);
            router_instance.run();
        }

        private async start_userserver() : Promise<void> {
            let userserver_instance: qh3server.userserver = new qh3server.userserver();
            userserver_instance.register_api(new api_user_get())
            userserver_instance.run(ref.NULL);
        }
        public async run() : Promise<void> {
            // this.start_router();
            await this.start_userserver();
        }
    }
}

export const app_instance = new app.qh3serverplugin_app();
app_instance.run();
