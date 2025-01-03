import * as ffi from 'ffi-napi';
import { qh3serversdk } from '../helpers/qh3serversdk';
import * as ref from 'ref-napi';
import { plainToClass } from "class-transformer";
import { msg_room_config_list, res_msg_user_get, rq_msg_user_get } from './messages';
import { essentials } from '../helpers/essentials';
import { qmongo } from '../helpers/qmongo';
import qhiredis from '../helpers/qhiredis';
import qzookeeper from '../helpers/qzookeeper';
import { debug_print, debug_print_error, LOG_LEVEL_4 } from '../helpers/sdktypes';
import { serverconfig } from '../helpers/serverconfig';
import * as path from 'path';

export namespace server {
    const DEFAULT_USER_TOKEN_EXPIRY_TIME = 300;

    export class userserver {
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
            // try {
                if (path === '/user_get') {
                    result = await this.parse_user_get(buffer);
                } else if (path === '/whoami') {
                    // not implemented
                }
                if (result) {
                    qh3serversdk.qh3serverplugin.qh3server_try_send_response(server, conn, result, result.length, null, 0);
                } else {
                    qh3serversdk.qh3serverplugin.qh3server_try_send_response(server, conn, `{}`, 2, null, 0);
                }
            // } catch (error) {
            //     debug_print_error(userserver.__LOGTAG__, JSON.stringify(error));
            // }


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
            let time_result = essentials.get_time_utc_readable();
            let last_login_utc_time_value : Date = time_result.utc_date;
            user_get_msg_respose.last_login = time_result.formatted_time;

            // const result = await this.exampleUsage();
            let redis_format_pid: string = `tokens:${user_get_msg_respose.pid}`;
            let token_in_redis: string | null | undefined = await this.hiredis?.get_value(redis_format_pid);
            if (token_in_redis!=null && token_in_redis?.length != 0) {
                user_get_msg_respose.token = token_in_redis ?? '';
                debug_print(LOG_LEVEL_4, userserver.__LOGTAG__, `token '${user_get_msg_respose.token}' retreived from redis for user id : ${crc}`);
            } else {
                user_get_msg_respose.token = essentials.sha256(JSON.stringify(user_get_msg_respose));
                debug_print(LOG_LEVEL_4, userserver.__LOGTAG__, `new token '${user_get_msg_respose.token}' for user id : ${crc}`);
            }

            let user_token_expiry_time: number = DEFAULT_USER_TOKEN_EXPIRY_TIME;
            if ((await this.hiredis?.set_value(redis_format_pid, user_get_msg_respose.token, user_token_expiry_time)) !== 0) {
                debug_print(LOG_LEVEL_4, userserver.__LOGTAG__, `Failed to set token on redis.`);
            }

            let gservers_map: Map<string, string[]> = new Map<string, string[]>();
            await this.getgservers(gservers_map);
            user_get_msg_respose.gservers = Object.fromEntries(gservers_map);

            const query_result = await this.mongo?.find_and_upsert(
                'users',
                (find_query: Record<string, any>) => {
                    find_query['user.pid'] = user_get_msg_respose.pid;
                },
                (update_query: Record<string, any>) => {
                    update_query['user.last_login'] = user_get_msg_respose.last_login;
                    // Append the "last_login" as a timestamp value (stored as a number)
                    update_query['user.last_login_timestamp'] = last_login_utc_time_value;
                },
                (insert_query: Record<string, any>) => {
                    insert_query['user.pid'] = user_get_msg_respose.pid;
                    insert_query['user.name'] = user_get_msg_respose.user_name;
                    insert_query['user.device.sys_name'] = user_get_msg_rq.device.sys_name;
                    insert_query['user.device.node_name'] = user_get_msg_rq.device.node_name;
                    insert_query['user.device.arch'] = user_get_msg_rq.device.arch;
                }
            );

            if (query_result === 0) { // Assuming `EXIT_SUCCESS` is represented by 0 in TypeScript
                let room_config : string | any = this.zkconfig?.get_string(`gserver/roomconfig`, '');
                user_get_msg_respose.room_list = JSON.parse(room_config) as msg_room_config_list;
            } else {
                console.info(`user_get failed`);
                return '{}';
            }
            let response_json = JSON.stringify(user_get_msg_respose);
            // console.log(`${response_json}`);
            return response_json;
        }
        
                
        private async getgservers(gservers_map: Map<string, string[]>) : Promise<void> {
            await this.hiredis?.scan(`gservers`, (key: string, field: string, value: string, arg?: any) => {
                debug_print(LOG_LEVEL_4, userserver.__LOGTAG__, `${key} - ${field}:${value}`);
                if (gservers_map.has(key)) {
                    gservers_map.get(key)!.push(value);
                } else {
                    gservers_map.set(key, [value]);
                }
            });
        }
        // private getgservers(key: string, field: string, value: string, arg?: any) : void {

        // }

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
            const redis_ip_port = essentials.extract_ip_and_port(this.router_config.redis_address);
            if (redis_ip_port) {
                const [ip, port]: [string, number] = redis_ip_port;
                this.hiredis = new qhiredis("hiredis", ip, port, "gsdkuser", "Fr0gmoon123");
            } else {
                console.log("Invalid hiredis address format");
                return;
            }
            try {
                await this.hiredis.connect_redis();
            } catch (error) {
                console.error(error);
            }

            const zk_ip_port = essentials.extract_ip_and_port(this.router_config.zk_uri);
            if (zk_ip_port) {
                const [ip, port]: [string, number] = zk_ip_port;
                this.qzk = new qzookeeper(this.router_config.zk_uri);
            } else {
                console.log("Invalid zk address format");
                return;
            }

            try {
                await this.qzk.connect();
            } catch (error) {
                console.error(error);
            }

            // server config
            this.zkconfig = new serverconfig(this.qzk, null);
            const config_path = path.join(this.router_config.root_dir, 'configs', 'dev', 'runtime-config.json');
            debug_print(LOG_LEVEL_4, userserver.__LOGTAG__, `reading ${config_path}`);
            await this.zkconfig.load(config_path, this.qzk, `/qh3server`);

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