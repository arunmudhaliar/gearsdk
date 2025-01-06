import { plainToClass } from 'class-transformer';
import { rq_msg_ping } from '../../userserver/messages';
import server from '../../userserver/userserver';

class api_ping implements server.interface_api {
    private static __LOGTAG__: string = `api_ping`;
    public get_path(): string {
        return `/ping`;
    }
    public get_post_cb(): server.type_api_callback {
        return this.parse_ping;
    }
    private async parse_ping(native_server: Buffer, conn: Buffer, user_server_interface: server.interface_userserver, api_instance: server.interface_api, path: string, buffer: string, len: number, headers: string, header_buffer_size: number) : Promise<string | null> {
        const ping_rq = plainToClass(rq_msg_ping, JSON.parse(buffer));
        const response_json = {
            pong:'qh3pluginserver',
            msg:ping_rq.msg
        };
        return JSON.stringify(response_json);
    }
}

export default api_ping;