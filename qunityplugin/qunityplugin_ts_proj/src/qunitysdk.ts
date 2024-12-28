import * as ffi from 'ffi-napi';
import * as ref from 'ref-napi';
import * as path from 'path';

export namespace qunitysdk {
    export const null_ptr = ref.NULL;
    export namespace client {
    const libPath = path.join(__dirname, './../release_build/libqunityplugin.dylib');

    // Define TypeScript types for each function
    export type type_async_request_cb = (payload: string, arg: Buffer, success: boolean) => void;

    export type type_qsocket_onconnect_cb = (guid_crc: number, arg: Buffer) => void;
    export type type_qsocket_onmessage_cb = (guid_crc: number, recv_len: number, buf: string) => void;
    export type type_qsocket_onreleaseconnection_cb = (guid_crc: number) => void;
    export type type_qsocket_onclose_cb = (guid_crc: number) => void;

    // Define callback functions with proper types
    export const type_qh3client_plugin_helper_cb = ffi.Callback('void', ['string', 'pointer', 'bool'], (payload, arg, success) => {
        console.log('Callback received:');
        console.log('Payload:', payload);
        console.log('Success:', success);
    }) as unknown as type_async_request_cb;

    export const type_qsocket_onconnect = ffi.Callback('void', ['ulong', 'pointer'], (guid_crc, arg) => {
        console.log('Connected to server with GUID CRC:', guid_crc);
        const payload = '{"sig":31387,"t_crc":3673067835,"room_config":{"min":2,"max":4,"betx":0,"rewardx":8192,"allow_after_start":false},"prev_cid_hash_val":0,"room_id":-1,"pid":"57f5159b"}';
        const sendResult = qunity_plugin.qsocket_sendMessage(guid_crc as unknown as number, payload, payload.length, false);
    }) as unknown as type_qsocket_onconnect_cb;

    export const type_qsocket_onmessage = ffi.Callback('void', ['ulong', 'ulong', 'string'], (guid_crc, recv_len, buf) => {
        const message = buf.substring(0, recv_len as unknown as number);
        console.log('Message received from server (GUID CRC:', guid_crc, '):', message);
    }) as unknown as type_qsocket_onmessage_cb;

    export const type_qsocket_onreleaseconnection = ffi.Callback('void', ['ulong'], (guid_crc) => {
        console.log('Connection released for GUID CRC:', guid_crc);
    }) as unknown as type_qsocket_onreleaseconnection_cb;

    export const type_qsocket_onclose = ffi.Callback('void', ['ulong'], (guid_crc) => {
        console.log('Connection closed for GUID CRC:', guid_crc);
    }) as unknown as type_qsocket_onclose_cb;

    // Define an interface for the plugin's methods to provide IntelliSense
    interface interface_qunityplugin {
        pre_init_sdk(): void;
        send_async_request(
            host: string,
            port: string,
            path: string,
            payload: string,
            arg: Buffer,
            callback: type_async_request_cb,
            timeout: number
        ): number;
        qsocket_connect(
            guid_crc: number,
            host: string,
            port: string,
            arg: Buffer,
            onconnect: type_qsocket_onconnect_cb,
            onmessage: type_qsocket_onmessage_cb,
            onreleaseconnection: type_qsocket_onreleaseconnection_cb,
            onclose: type_qsocket_onclose_cb
        ): boolean;
        qsocket_sendMessage(guid_crc: number, message: string, message_len: number, close_after_send: boolean): number;
        qsocket_close(guid_crc: number): number;
    }

    // Load the library and cast it to the interface_qunityplugin interface
    export const qunity_plugin = ffi.Library(libPath, {
        'pre_init_sdk': ['void', []],
        'send_async_request': ['int', ['string', 'string', 'string', 'string', 'pointer', 'pointer', 'int']],
        'qsocket_connect': ['bool', ['ulong', 'string', 'string', 'pointer', 'pointer', 'pointer', 'pointer', 'pointer']],
        'qsocket_sendMessage': ['int', ['ulong', 'string', 'ulong', 'bool']],
        'qsocket_close': ['int', ['ulong']],
    }) as interface_qunityplugin;

    qunity_plugin.pre_init_sdk();
    console.log('qunityplugin loaded successfully.');

    // Example usage
    // const payload = '{"pid":"","token":"","device":{"sys_name":"Ubuntu","node_name":"ubuntu-vm.local","release":"20.04","arch":"x86_64"}}';
    // const null_ptr = ref.NULL;

    // const result = qunity_plugin.send_async_request('15.206.79.30', '4004', '/user_get', payload, null_ptr, type_qh3client_plugin_helper_cb, 3);
    // console.log('Send Async Request Result:', result);

    // const guid_crc = 123456789; // Replace this with your actual GUID CRC value
    // const connected = qunity_plugin.qsocket_connect(guid_crc, '15.206.79.30', '4000', null_ptr, type_qsocket_onconnect, type_qsocket_onmessage, type_qsocket_onreleaseconnection, type_qsocket_onclose);
    // if (connected) {
    //     console.log('Attempting to connect to the server...');
    // }

    // const guid_crc2 = 987654321; // Replace this with your actual GUID CRC value
    // const connected2 = qunity_plugin.qsocket_connect(guid_crc2, '15.206.79.30', '4000', null_ptr, type_qsocket_onconnect, type_qsocket_onmessage, type_qsocket_onreleaseconnection, type_qsocket_onclose);
    // if (connected2) {
    //     console.log('Attempting to connect2 to the server...');
    // }

    // // Prevent the main process from exiting
    // setInterval(() => {
    //     console.log(`alive`);
    // }, 10000); // Keeps the event loop alive with a repeating timer.
}
}
