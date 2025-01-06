import * as ffi from 'ffi-napi';
import * as path from 'path';
import ref from "ref-napi";
import { debug_print, LOG_LEVEL_0, LOG_LEVEL_4 } from './sdktypes';

export namespace serversdk {
    // Import libc's malloc and free
    export const libc = ffi.Library('libc', {
        strdup: ['pointer', ['string']], // strdup allocates memory for the string copy
        malloc: ['pointer', ['size_t']],
        free: ['void', ['pointer']],
    });

    export const uLong = ref.types.ulong; // Represents `unsigned long`
    export const Bytef = ref.refType(ref.types.uchar); // Pointer to a buffer
    export const size_t = ref.types.size_t; // Represents size_t
    const __LOGTAG__: string = `serversdk`;

    let lib_path: string;
    if (process.platform === 'darwin') {
        lib_path = path.join(__dirname, './../serverplugin/libserverplugin-debug.dylib');
        // lib_path = path.join(__dirname, './../serverplugin/libserverplugin.dylib');         // release version
    } else if (process.platform === 'linux') {
        lib_path = path.join(__dirname, './../serverplugin/libserverplugin-debug.so');
        // lib_path = path.join(__dirname, './../serverplugin/libserverplugin.so');            // release version
    } else {
        throw new Error('unsupported platform');
    }

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
    export type type_on_server_start = (router: Buffer, ip: string, port: number) => void;
    export type type_on_server_stop = (server: Buffer) => void;
    export type type_on_server_error = (server: Buffer, error_code: number) => void;
    export type type_on_server_parse = (server: Buffer, conn: Buffer, path: string, buffer: string, len: number, headers: string, header_buffer_size: number) => void;

    // Define the interface for the library's methods
    interface interface_serverplugin {
        setup_signal_handler(): void;
        pre_init_serverplugin_sdk(): void;
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
            native_router: Buffer,
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
        get_crc32(str: string, len: number): number;
        mod_crc32(adler: number, buf: string | null, len: number): number;
        qh3server_try_send_response(native_server: Buffer, conn: Buffer, payload: string, len: number, user_data: string | null, user_data_len: number): void;
        test_func(): number;
        get_live_connection_count(native_server: Buffer): number;
        get_device_public_ip(): string;
    }

    // Load the C library and cast it to the interface_qh3serverplugin
    export const serverplugin = ffi.Library(lib_path, {
        setup_signal_handler: ['void', []],
        pre_init_serverplugin_sdk: ['void', []],
        spawn_qh3router: ['void', ['string', 'string', 'string', 'string', 'string', 'uint16', 'uint16', 'string', 'pointer', 'pointer', 'pointer', 'pointer']],
        spawn_qh3server: ['void', ['pointer', 'string', 'string', 'string', 'string', 'string', 'uint16', 'uint16', 'string', 'pointer', 'pointer', 'pointer', 'pointer', 'pointer']],
        get_crc32: ['ulong', ['string', 'int']],
        mod_crc32: [uLong, [uLong, 'string', size_t]],
        qh3server_try_send_response: ['void', ['pointer', 'pointer', 'string', size_t, 'string', size_t]],
        test_func: ['int', []],
        get_live_connection_count: ['uint', ['pointer']],
        get_device_public_ip: ['string', []],
    }) as interface_serverplugin;

    debug_print(LOG_LEVEL_0, __LOGTAG__, 'serverplugin loaded successfully.');
    debug_print(LOG_LEVEL_0, __LOGTAG__, "current directory:", process.cwd());
    serverplugin.setup_signal_handler();
    serverplugin.pre_init_serverplugin_sdk();

    // const router_config : qh3_router_input_config = {
    //     router_address: `127.0.0.1:4004`,
    //     mongodb_uri: `mongodb://3.109.144.159:27017`,
    //     redis_address: `3.109.144.159:6379`,
    //     zk_uri: `3.109.144.159:2181`,
    //     root_dir: process.cwd(),
    //     command_port: 4010,
    //     router_port_return: 4005,
    //     app_id: `serverplugin-app`
    // };

    // const structFactory = StructType(ref);
    // // Define the struct layout for response
    // const parse_response_struct = structFactory({
    //     data: 'string', // Pointer to the response buffer
    //     size: 'int',       // Size of the response buffer
    // });

    /*
    // server events
    export const on_server_pre_start = ffi.Callback('void', ['pointer'], (server: Buffer) => {
        debug_print(LOG_LEVEL_4, userserver.__LOGTAG__, `on_server_pre_start`);
    }) as unknown as type_on_server_pre_start;
    export const on_server_start = ffi.Callback('void', ['pointer'], (server: Buffer) => {
        debug_print(LOG_LEVEL_4, userserver.__LOGTAG__, `on_server_start`);
    }) as unknown as type_on_server_start;
    export const on_server_stop = ffi.Callback('void', ['pointer'], (server: Buffer) => {
        debug_print(LOG_LEVEL_4, userserver.__LOGTAG__, `on_server_stop:`);
    }) as unknown as type_on_server_stop;
    export const on_server_error = ffi.Callback('void', ['pointer'], (server: Buffer, error_code: number) => {
        debug_print(LOG_LEVEL_4, userserver.__LOGTAG__, `on_server_error: ${error_code}`);
    }) as unknown as type_on_server_error;
    export const on_server_parse = ffi.Callback('pointer', ['pointer', 'string', 'string', 'int'], (server: Buffer, path: string, buffer: string, len: number) => {
        debug_print(LOG_LEVEL_4, userserver.__LOGTAG__, `on_server_parse: ${path}, ${buffer}, len ${len}`);
        const response_string = `{msg:\"test message\"}`;
        // Use strdup to allocate and return a copy of the string
        const result = libc.strdup(response_string);
        if (result.isNull()) {
            throw new Error('Memory allocation failed using strdup.');
        }
        return result; // Return the pointer to C++
    }) as unknown as type_on_server_parse;

    serverplugin.spawn_qh3server(
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
    */

    /*
    // router events
    export const on_router_pre_start = ffi.Callback('void', ['pointer'], (router: Buffer) => {
        debug_print(LOG_LEVEL_4, userserver.__LOGTAG__, `on_router_pre_start`);
    }) as unknown as type_on_router_pre_start;
    export const on_router_start = ffi.Callback('void', ['pointer'], (router: Buffer) => {
        debug_print(LOG_LEVEL_4, userserver.__LOGTAG__, `on_router_start`);
    }) as unknown as type_on_router_start;
    export const on_router_stop = ffi.Callback('void', [], () => {
        debug_print(LOG_LEVEL_4, userserver.__LOGTAG__, `on_router_stop:`);
    }) as unknown as type_on_router_stop;
    export const on_router_error = ffi.Callback('void', [], (error_code: number) => {
        debug_print(LOG_LEVEL_4, userserver.__LOGTAG__, `on_router_error: ${error_code}`);
    }) as unknown as type_on_router_error;

    serverplugin.spawn_qh3router(
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