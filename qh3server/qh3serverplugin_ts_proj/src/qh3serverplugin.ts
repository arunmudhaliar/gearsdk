import * as ffi from 'ffi-napi';
import * as ref from 'ref-napi';
import * as path from 'path';
// import StructType from 'ref-struct-napi';
// const StructType = require('ref-struct-napi');

export namespace qh3serversdk {
    const lib_path = path.join(__dirname, './../qh3serverplugin/libqh3serverplugin-debug.dylib');
    // const lib_path = path.join(__dirname, './../qh3serverplugin/libqh3serverplugin.dylib');         // release version

    // Define the exported C functions' TypeScript types
    export interface qh3_router_input_config {
        router_address: string;
        mongodb_uri: string;
        redis_address: string;
        zk_uri: string;
        root_dir: string;
        command_port: number;
        router_port_return: number;
        app_id: string;
    }

    // router events
    export type type_on_router_pre_start = (router: Buffer) => void;
    export type type_on_router_start = (router: Buffer) => void;
    export type type_on_router_stop = () => void;
    export type type_on_router_error = (error_code: number) => void;

    // server events
    export type type_on_server_pre_start = (server: Buffer) => void;
    export type type_on_server_start = (router: Buffer) => void;
    export type type_on_server_stop = (server: Buffer) => void;
    export type type_on_server_error = (server: Buffer, error_code: number) => void;
    export type type_on_server_parse = (server: Buffer, path: string, buffer: string, len: number) => void;

    // Define the interface for the library's methods
    interface interface_qh3serverplugin {
        setup_signal_handler() : void;
        pre_init_qh3serverplugin_sdk() : void;
        spawn_qh3router(
            router_address: string,
            mongodb_uri: string,
            redis_address: string,
            zk_uri: string,
            root_dir: string,
            command_port: number,
            router_port_return: number,
            app_id: string,
            pre_start_cb: type_on_router_pre_start,
            start_cb: type_on_router_start,
            stop_cb: type_on_router_stop,
            error_cb: type_on_router_error
        ): void;
        spawn_qh3server(
            router: Buffer,
            router_address: string,
            mongodb_uri: string,
            redis_address: string,
            zk_uri: string,
            root_dir: string,
            command_port: number,
            router_port_return: number,
            app_id: string,
            pre_start_cb: type_on_server_pre_start,
            start_cb: type_on_server_start,
            stop_cb: type_on_server_stop,
            error_cb: type_on_server_error,
            parse_cb: type_on_server_parse
        ): void;
        test_func() : number;
    }

    // Load the C library and cast it to the interface_qh3serverplugin
    export const qh3serverplugin = ffi.Library(lib_path, {
        setup_signal_handler: ['void', []],
        pre_init_qh3serverplugin_sdk: ['void', []],
        spawn_qh3router: ['void', ['string', 'string', 'string', 'string', 'string', 'uint16', 'uint16', 'string', 'pointer', 'pointer', 'pointer', 'pointer']],
        spawn_qh3server: ['void', ['pointer', 'string', 'string', 'string', 'string', 'string', 'uint16', 'uint16', 'string', 'pointer', 'pointer', 'pointer', 'pointer', 'pointer']],
        test_func: ['int', []],
    }) as interface_qh3serverplugin;

    console.log('qh3serverplugin loaded successfully.');
    console.log("Current Directory:", process.cwd());
    qh3serverplugin.setup_signal_handler();
    qh3serverplugin.pre_init_qh3serverplugin_sdk();
    // console.log(`test_func returned ${qh3serverplugin.test_func()}`);

    const router_config : qh3_router_input_config = {
        router_address: `127.0.0.1:4004`,
        mongodb_uri: `mongodb://3.109.144.159:27017`,
        redis_address: `3.109.144.159:6379`,
        zk_uri: `3.109.144.159:2181`,
        root_dir: process.cwd(),
        command_port: 4010,
        router_port_return: 4005,
        app_id: `qh3serverplugin-app`
    };

    // const structFactory = StructType(ref);
    // // Define the struct layout for response
    // const parse_response_struct = structFactory({
    //     data: 'string', // Pointer to the response buffer
    //     size: 'int',       // Size of the response buffer
    // });

    // server events
    export const on_server_pre_start = ffi.Callback('void', ['pointer'], (server: Buffer) => {
        console.log(`on_server_pre_start`);
    }) as unknown as type_on_server_pre_start;
    export const on_server_start = ffi.Callback('void', ['pointer'], (server: Buffer) => {
        console.log(`on_server_start`);
    }) as unknown as type_on_server_start;
    export const on_server_stop = ffi.Callback('void', ['pointer'], (server: Buffer) => {
        console.log(`on_server_stop:`);
    }) as unknown as type_on_server_stop;
    export const on_server_error = ffi.Callback('void', ['pointer'], (server: Buffer, error_code: number) => {
        console.log(`on_server_error: ${error_code}`);
    }) as unknown as type_on_server_error;
    export const on_server_parse = ffi.Callback('void', ['pointer', 'string', 'string', 'int'], (server: Buffer, path: string, buffer: string, len: number) => {
        console.log(`on_server_parse: ${path}, ${buffer}, len ${len}`);
        // Create the response buffer
        // const response_data = `{test}`;
        // const response_bytes = Buffer.from(response_data, 'utf-8');

        // // Allocate the response buffer and copy data
        // const response_buffer = Buffer.alloc(response_bytes.length + 1); // +1 for null-terminator
        // response_bytes.copy(response_buffer);
        // response_buffer[response_bytes.length] = 0; // Null-terminate

        // // Create the struct instance
        // const response = new parse_response_struct({
        //     data: response_buffer,
        //     size: response_bytes.length,
        // });

        // console.log(`Response constructed: ${response_data}`);
        // return response.ref(); // Return pointer to the struct
    }) as unknown as type_on_server_parse;

    qh3serverplugin.spawn_qh3server(
        ref.NULL,
        router_config.router_address,
        router_config.mongodb_uri,
        router_config.redis_address,
        router_config.zk_uri,
        router_config.root_dir,
        router_config.command_port,
        router_config.router_port_return,
        router_config.app_id,
        on_server_pre_start,
        on_server_start,
        on_server_stop,
        on_server_error,
        on_server_parse
    );

    /*
    // router events
    export const on_router_pre_start = ffi.Callback('void', ['pointer'], (router: Buffer) => {
        console.log(`on_router_pre_start`);
    }) as unknown as type_on_router_pre_start;
    export const on_router_start = ffi.Callback('void', ['pointer'], (router: Buffer) => {
        console.log(`on_router_start`);
    }) as unknown as type_on_router_start;
    export const on_router_stop = ffi.Callback('void', [], () => {
        console.log(`on_router_stop:`);
    }) as unknown as type_on_router_stop;
    export const on_router_error = ffi.Callback('void', [], (error_code: number) => {
        console.log(`on_router_error: ${error_code}`);
    }) as unknown as type_on_router_error;

    qh3serverplugin.spawn_qh3router(
        router_config.router_address,
        router_config.mongodb_uri,
        router_config.redis_address,
        router_config.zk_uri,
        router_config.root_dir,
        router_config.command_port,
        router_config.router_port_return,
        router_config.app_id,
        on_router_pre_start,
        on_router_start,
        on_router_stop,
        on_router_error
    );
    */
}