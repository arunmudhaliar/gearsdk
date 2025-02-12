import { plainToClass } from 'class-transformer';
import { essentials } from '../../helpers/essentials';
import { serversdk } from '../../helpers/libserverplugin';
import { debug_error, debug_print, EXIT_SUCCESS, LOG_LEVEL_4 } from '../../helpers/sdktypes';
import { rq_msg_user_get, res_msg_user_get, msg_room_config_list } from '../../userserver/messages';
import server from '../../userserver/userserver';

class api_user_get implements server.interface_api {
    private DEFAULT_USER_TOKEN_EXPIRY_TIME = 300;
    private static __LOGTAG__: string = `api_user_get`;
    public get_path(): string {
        return `/user_get`;
    }
    public get_post_cb(): server.type_api_callback {
        return this.parse_user_get;
    }
    private async parse_user_get(native_server: serversdk.qh3server_ptr, cid: Buffer, cid_len: number, user_server_interface: server.interface_userserver, api_instance: server.interface_api, path: string, buffer: string, headers: string): Promise<string | null> {
        let thiz:api_user_get = api_instance as api_user_get;

        const user_get_msg_rq = plainToClass(rq_msg_user_get, JSON.parse(buffer));
        // debug_print(LOG_LEVEL_4, userserver.__LOGTAG__, "Deserialized Message:", user_get_msg_rq);

        let crc: number = serversdk.sdklib.mod_crc32(0, Buffer.from(''));
        crc = serversdk.sdklib.mod_crc32(crc, Buffer.from(user_get_msg_rq.device.sys_name));
        crc = serversdk.sdklib.mod_crc32(crc, Buffer.from(user_get_msg_rq.device.node_name));
        crc = serversdk.sdklib.mod_crc32(crc, Buffer.from(user_get_msg_rq.device.release));
        crc = serversdk.sdklib.mod_crc32(crc, Buffer.from(user_get_msg_rq.device.arch));

        let user_get_msg_response: res_msg_user_get = new res_msg_user_get();
        user_get_msg_response.pid = `${crc.toString(16)}`;
        user_get_msg_response.user_name = `guest-${crc.toString(16)}`;
        let time_result = essentials.get_time_utc_readable();
        let last_login_utc_time_value : number = time_result.utc_date_number;
        user_get_msg_response.last_login = time_result.utc_date_string;

        let redis_format_pid: string = `tokens:${user_get_msg_response.pid}`;
        let token_in_redis: string | null | undefined = await user_server_interface.get_hiredis_driver()?.get_value(redis_format_pid);
        if (token_in_redis!=null && token_in_redis?.length != 0) {
            user_get_msg_response.token = token_in_redis ?? '';
            debug_print(LOG_LEVEL_4, api_user_get.__LOGTAG__, `token '${user_get_msg_response.token}' retreived from redis for user id : ${crc}`);
        } else {
            user_get_msg_response.token = essentials.sha256(JSON.stringify(user_get_msg_response));
            debug_print(LOG_LEVEL_4, api_user_get.__LOGTAG__, `new token '${user_get_msg_response.token}' for user id : ${crc}`);
        }

        let user_token_expiry_time: number | undefined = user_server_interface.get_zkconfig()?.get_int32("server_config/user_token_expiry_time", thiz.DEFAULT_USER_TOKEN_EXPIRY_TIME);
        if ((await user_server_interface.get_hiredis_driver()?.set_value(redis_format_pid, user_get_msg_response.token, user_token_expiry_time)) !== 0) {
            debug_error(api_user_get.__LOGTAG__, `Failed to set token on redis.`);
        }

        let gservers_map: Map<string, string[]> = new Map<string, string[]>();
        await api_user_get.getgservers(user_server_interface, gservers_map);
        user_get_msg_response.gservers = Array.from(gservers_map.entries()).map(([addr, ports]) => {
            return {
                addr,      // The key of the Map (gservers:15.206.79.30)
                ports,     // The array of ports (e.g., ["172.17.0.4:4000"])
            };
        });

        const query_result = await user_server_interface.get_mongo_driver()?.find_and_upsert(
            'users',
            (find_query: Record<string, any>) => {
                find_query['user.pid'] = user_get_msg_response.pid;
            },
            (update_query: Record<string, any>) => {
                update_query['user.last_login'] = user_get_msg_response.last_login;
                update_query['user.last_login_timestamp'] = last_login_utc_time_value;
            },
            (insert_query: Record<string, any>) => {
                insert_query['user.pid'] = user_get_msg_response.pid;
                insert_query['user.name'] = user_get_msg_response.user_name;
                insert_query['user.device.sys_name'] = user_get_msg_rq.device.sys_name;
                insert_query['user.device.node_name'] = user_get_msg_rq.device.node_name;
                insert_query['user.device.arch'] = user_get_msg_rq.device.arch;
            }
        );

        if (query_result === EXIT_SUCCESS) {
            let room_config : string | any = user_server_interface.get_zkconfig()?.get_string(`gserver/roomconfig`, '');
            user_get_msg_response.room_list = JSON.parse(room_config) as msg_room_config_list;
        } else {
            debug_error(api_user_get.__LOGTAG__, `user_get failed`);
            return '{}';
        }
        let response_json = JSON.stringify(user_get_msg_response);
        // debug_print(LOG_LEVEL_0, api_user_get.__LOGTAG__, `${response_json}`);
        return response_json;
    }

    private static async getgservers(user_server_interface: server.interface_userserver, gservers_map: Map<string, string[]>) : Promise<void> {
        await user_server_interface.get_hiredis_driver()?.scan(`gservers`, (key: string, field: string, value: string, arg?: any) => {
            debug_print(LOG_LEVEL_4, api_user_get.__LOGTAG__, `${key} - ${field}:${value}`);
            if (gservers_map.has(key)) {
                gservers_map.get(key)!.push(value);
            } else {
                gservers_map.set(key, [value]);
            }
        });
    }
}

export default api_user_get;