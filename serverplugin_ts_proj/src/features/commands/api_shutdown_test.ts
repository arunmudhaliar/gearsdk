import server from '../../userserver/userserver';
import { serversdk } from '../../helpers/libserverplugin';

class api_shutdown_test implements server.interface_api {
    private static __LOGTAG__: string = `api_shutdown_test`;
    public get_path(): string {
        return `/shutdown_test`;
    }
    public get_post_cb(): server.type_api_callback {
        return this.parse_shutdown_test;
    }
    private async parse_shutdown_test(native_server: serversdk.qh3server_ptr, cid: Buffer, cid_len: number, user_server_interface: server.interface_userserver, api_instance: server.interface_api, path: string, buffer: string, headers: string): Promise<string | null> {
        const response_json = {
            server: 'qh3pluginserver',
            msg: 'shutdown-ack'
        };
        serversdk.sdklib.qh3server_shutdown(native_server);
        return JSON.stringify(response_json);
    }
}

export default api_shutdown_test;