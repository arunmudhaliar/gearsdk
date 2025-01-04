import server from '../../userserver/userserver';

class api_whoami implements server.interface_api {
    private static __LOGTAG__: string = `api_whoami`;
    public get_path(): string {
        return `/whoami`;
    }
    public get_post_cb(): (user_server_interface: server.interface_userserver, api_instance: server.interface_api, path: string, buffer: string, len: number) => Promise<string | null | any> {
        return this.parse_user_get;
    }
    private async parse_user_get(user_server_interface: server.interface_userserver, api_instance: server.interface_api, path: string, buffer: string, len: number) : Promise<string | null> {
        const response_json = {
            name:'qh3pluginserver'
        };
        return JSON.stringify(response_json);
    }
}

export default api_whoami;