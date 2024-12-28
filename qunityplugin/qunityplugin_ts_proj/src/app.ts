import * as ffi from 'ffi-napi';
import { qunitysdk } from './qunitysdk';
const qunity_plugin = qunitysdk.client.qunity_plugin;

namespace app {
    export class qunityplugin_app {
        // Define callback functions with proper types
        qh3client_plugin_helper_cb = ffi.Callback('void', ['string', 'pointer', 'bool'], (payload, arg, success) => {
            console.log('Callback received:');
            console.log('Payload:', payload);
            console.log('Success:', success);
        }) as unknown as qunitysdk.client.type_async_request_cb;

        cb_qsocket_onconnect = ffi.Callback('void', ['ulong', 'pointer'], (guid_crc, arg) => {
            console.log('Connected to server with GUID CRC:', guid_crc);
            const payload = '{"sig":31387,"t_crc":3673067835,"room_config":{"min":2,"max":4,"betx":0,"rewardx":8192,"allow_after_start":false},"prev_cid_hash_val":0,"room_id":-1,"pid":"57f5159b"}';
            const sendResult = qunity_plugin.qsocket_sendMessage(guid_crc as unknown as number, payload, payload.length, false);
        }) as unknown as qunitysdk.client.type_qsocket_onconnect_cb;

        cb_qsocket_onmessage = ffi.Callback('void', ['ulong', 'ulong', 'string'], (guid_crc, recv_len, buf) => {
            const message = buf.substring(0, recv_len as unknown as number);
            console.log('Message received from server (GUID CRC:', guid_crc, '):', message);
        }) as unknown as qunitysdk.client.type_qsocket_onmessage_cb;

        cb_qsocket_onreleaseconnection = ffi.Callback('void', ['ulong'], (guid_crc) => {
            console.log('Connection released for GUID CRC:', guid_crc);
        }) as unknown as qunitysdk.client.type_qsocket_onreleaseconnection_cb;

        cb_qsocket_onclose = ffi.Callback('void', ['ulong'], (guid_crc) => {
            console.log('Connection closed for GUID CRC:', guid_crc);
        }) as unknown as qunitysdk.client.type_qsocket_onclose_cb;

        constructor() {
            console.log('Starting qunityplugin_app');
            this.init();
        }

        public init() : void {
            const guid_crc = 987654321; // Replace this with the actual GUID CRC value
            const status1 = qunity_plugin.qsocket_connect(
                guid_crc,
                '15.206.79.30',
                '4000',
                qunitysdk.null_ptr,
                this.cb_qsocket_onconnect,
                this.cb_qsocket_onmessage,
                this.cb_qsocket_onreleaseconnection,
                this.cb_qsocket_onclose
            );
        
            if (status1) {
                console.log('status1: Successfully connected to the server!');
            } else {
                console.log('status1: Connection failed.');
            }


            const guid2_crc = 123456789; // Replace this with the actual GUID CRC value
            const status2 = qunity_plugin.qsocket_connect(
                guid2_crc,
                '15.206.79.30',
                '4000',
                qunitysdk.null_ptr,
                this.cb_qsocket_onconnect,
                this.cb_qsocket_onmessage,
                this.cb_qsocket_onreleaseconnection,
                this.cb_qsocket_onclose
            );
        
            if (status2) {
                console.log('status2: Successfully connected to the server!');
            } else {
                console.log('status2: Connection failed.');
            }
        }

        public run() : void {
            const keepAlive = async () => {
                while (true) {
                    await new Promise(resolve => setTimeout(resolve, 10000));
                    console.log(`Keeping the event loop alive`);
                }
            };
            keepAlive();
        }
    }
}

export const app_instance = new app.qunityplugin_app();
app_instance.run();