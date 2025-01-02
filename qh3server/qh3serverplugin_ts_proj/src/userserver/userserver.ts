import * as ffi from 'ffi-napi';
import { qh3serversdk } from '../helpers/qh3serversdk';
import * as ref from 'ref-napi';
import { plainToClass } from "class-transformer";
import { res_msg_user_get, rq_msg_user_get } from './messages';
import { essentials } from '../helpers/essentials';
import { qmongo } from '../helpers/qmongo';

export namespace server {
    export class userserver {
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

        protected on_server_pre_start = ffi.Callback('void', ['pointer'], (server: Buffer) => {
            console.log(`on_server_pre_start`);
        }) as unknown as qh3serversdk.type_on_server_pre_start;
        protected on_server_start = ffi.Callback('void', ['pointer'], async (server: Buffer) => {
            console.log(`on_server_start`);
        }) as unknown as qh3serversdk.type_on_server_start;
        protected on_server_stop = ffi.Callback('void', ['pointer'], (server: Buffer) => {
            console.log(`on_server_stop:`);
        }) as unknown as qh3serversdk.type_on_server_stop;
        protected on_server_error = ffi.Callback('void', ['pointer'], (server: Buffer, error_code: number) => {
            console.log(`on_server_error: ${error_code}`);
        }) as unknown as qh3serversdk.type_on_server_error;
        protected on_server_parse = ffi.Callback('void', ['pointer', 'pointer', 'string', 'string', 'int'], async (server: Buffer, conn: Buffer, path: string, buffer: string, len: number) => {
            console.log(`on_server_parse: ${path}, ${buffer}, len ${len}`);
            let result: string | null = null;
            if (path === '/user_get') {
                result = await this.parse_user_get(buffer);
            } else if (path === '/whoami') {

            }
            if (result) {
                try {
                    qh3serversdk.qh3serverplugin.qh3server_try_send_response(server, conn, result, result.length, null, 0);    
                } catch (error) {
                    console.error(error);
                }
            }
            // const response_string = `{msg:\"test message\"}`;
            // // Use strdup to allocate and return a copy of the string
            // result = qh3serversdk.libc.strdup(response_string);
            // if (result.isNull()) {
            //     throw new Error('Memory allocation failed using strdup.');
            // }
            // return result; // Return the pointer to C++
        }) as unknown as qh3serversdk.type_on_server_parse;

        private async parse_user_get(request_payload: string) : Promise<string | null> {

            // const jsonBuffer = '{"pid": "12345", "token": "abcdef"}';
            const user_get_msg_rq = plainToClass(rq_msg_user_get, JSON.parse(request_payload));
            console.log("Deserialized Message:", user_get_msg_rq);

            let crc : number = qh3serversdk.qh3serverplugin.mod_crc32(0, null, 0);
            crc = qh3serversdk.qh3serverplugin.mod_crc32(crc, user_get_msg_rq.device.sys_name, user_get_msg_rq.device.sys_name.length);
            crc = qh3serversdk.qh3serverplugin.mod_crc32(crc, user_get_msg_rq.device.node_name, user_get_msg_rq.device.node_name.length);
            crc = qh3serversdk.qh3serverplugin.mod_crc32(crc, user_get_msg_rq.device.release, user_get_msg_rq.device.release.length);
            crc = qh3serversdk.qh3serverplugin.mod_crc32(crc, user_get_msg_rq.device.arch, user_get_msg_rq.device.arch.length);

            let user_get_msg_respose: res_msg_user_get = new res_msg_user_get();
            user_get_msg_respose.pid = `${crc.toString(16)}`;
            user_get_msg_respose.user_name = `guest-${crc.toString(16)}`;
            user_get_msg_respose.last_login = essentials.get_time_utc_readable();

            const result = await this.exampleUsage();

            return JSON.stringify(user_get_msg_respose);
        }

        async exampleUsage() : Promise<void> {
            // const dbClient = new YourDatabaseClient(); // assuming this is your class that includes the `find` method
            const collectionName = "users"; // Example collection name
            const filter = { age: { $gte: 18 } }; // Example filter to find users older than or equal to 18
        
            try {
                // Call the `find` method and await the result
                const users = await this.mongo?.find(collectionName, filter);
        
                // Log the result (the list of documents)
                console.log("Users found:", users);
            } catch (error) {
                // Handle error (e.g., collection not found or query failure)
                console.error("Error fetching users:", error);
            }
        }

        public async run(native_router: Buffer) : Promise<void> {
            this.mongo = new qmongo("", "gsdk_mongodb", this.router_config.mongodb_uri);
            await this.mongo?.connect();
            await this.exampleUsage();
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
            /*
            this.mongo = new qmongo("", "gsdk_mongodb", this.router_config.mongodb_uri);
            try {
                this.mongo.connect().then(() => {
                    console.log("MongoDB connection successful");
                    // Do something after the connection
                    qh3serversdk.qh3serverplugin.spawn_qh3server(
                        ref.NULL,
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
                })
                .catch((error) => {
                    console.log("Error connecting to MongoDB:", error);
                })
                .finally(() => {
                    console.log("Finished attempting MongoDB connection");
                    // this.mongo?.disconnect();
                    // This block will always execute, regardless of success or failure
                });
            } catch (error) {
                console.error(error);
            }
 */

            // qh3serversdk.qh3serverplugin.spawn_qh3server(
            //     ref.NULL,
            //     this.router_config.router_address,
            //     this.router_config.mongodb_uri,
            //     this.router_config.redis_address,
            //     this.router_config.zk_uri,
            //     this.router_config.root_dir,
            //     this.router_config.command_port,
            //     this.router_config.router_port_return,
            //     this.router_config.app_id,
            //     this.on_server_pre_start,
            //     this.on_server_start,
            //     this.on_server_stop,
            //     this.on_server_error,
            //     this.on_server_parse
            // );
        }
    }
}