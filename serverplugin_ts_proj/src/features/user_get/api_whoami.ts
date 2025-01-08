import { header, header_utils } from '../../helpers/header_utils';
import { serversdk } from '../../helpers/serversdk';
import server from '../../userserver/userserver';

class api_whoami implements server.interface_api {
    private static __LOGTAG__: string = `api_whoami`;
    public get_path(): string {
        return `/whoami`;
    }
    public get_post_cb(): server.type_api_callback {
        return this.parse_whoami;
    }
    private async parse_whoami(native_server: serversdk.qh3server_ptr, conn: Buffer, user_server_interface: server.interface_userserver, api_instance: server.interface_api, path: string, buffer: string, len: number, headers: string, header_buffer_size: number): Promise<string | null> {
        // let header_map = header_utils.json_to_map(headers);
        // let path_header : header | any =  header_utils.get_header(':path', header_map);
        const response_json = {
            name:'qh3pluginserver',
            active_connections: serversdk.serverplugin.get_live_connection_count(native_server)
        };
        return JSON.stringify(response_json);
    }
}

export default api_whoami;