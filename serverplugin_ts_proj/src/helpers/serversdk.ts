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

    export const ulong = ref.types.ulong; // Represents `unsigned long`
    export const Bytef = ref.refType(ref.types.uchar); // Pointer to a buffer
    export const size_t = ref.types.size_t; // Represents size_t
    export const voidp = ref.refType('void');
    export const nullptr = ref.NULL;
    export const int = ref.types.int;
    export const uint = ref.types.uint;
    export const uint8 = ref.types.uint8;
    export const uint16 = ref.types.uint16;

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

    export interface qserver_input_config {
        server_address: string;
        redis_address: string;
        zk_uri: string;
        root_dir: string;
        app_id: string;
    }

    // router events
    export type type_on_router_pre_start = (router: Buffer) => void;
    export type type_on_router_start = (router: Buffer) => void;
    export type type_on_router_stop = () => void;
    export type type_on_router_error = (error_code: number) => void;

    // qh3server events
    export type type_on_server_pre_start = (server: Buffer) => void;
    export type type_on_server_start = (router: Buffer, ip: string, port: number) => void;
    export type type_on_server_stop = (server: Buffer) => void;
    export type type_on_server_error = (server: Buffer, error_code: number) => void;
    export type type_on_server_parse = (server: Buffer, conn: Buffer, path: string, buffer: string, len: number, headers: string, header_buffer_size: number) => void;

    // qserver events
    export type type_on_qserver_pre_start = (server: Buffer) => void;
    export type type_on_qserver_start = (router: Buffer, ip: string, port: number) => void;
    export type type_on_qserver_stop = (server: Buffer) => void;
    export type type_on_qserver_error = (server: Buffer, error_code: number) => void;

    // room events
    export type type_on_room_event_create = (native_server: Buffer, room: number) => void;
    export type type_on_room_event_start = (native_server: Buffer, room: number) => void;
    export type type_on_room_event_player_added = (native_server: Buffer, room: number, pid: string, cid_hash: number) => void;
    export type type_on_room_event_message = (native_server: Buffer, room: number, pid: string, cid_hash: number, message: string) => void;
    export type type_on_room_event_player_removed = (native_server: Buffer, room: number, pid: string, cid_hash: number) => void;
    export type type_on_room_event_end = (native_server: Buffer, room: number) => void;
    export type type_on_room_event_countdown_to_start = (native_server: Buffer, room: number, count: number, max_count: number) => void;
    export type type_on_room_event_countdown_cancelled = (native_server: Buffer, room: number) => void;

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
            server_address: string,
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
        spawn_qserver(
            server_address: string,
            redis_address: string,
            zk_uri: string,
            root_dir: string,
            app_id: string,
            pre_start_cb: type_on_qserver_pre_start,
            start_cb: type_on_qserver_start,
            stop_cb: type_on_qserver_stop,
            error_cb: type_on_qserver_error,
            room_event_create: type_on_room_event_create | null,
            room_event_start: type_on_room_event_start,
            room_event_player_added: type_on_room_event_player_added,
            room_event_message: type_on_room_event_message,
            room_event_player_removed: type_on_room_event_player_removed,
            room_event_end: type_on_room_event_end,
            room_event_countdown_to_start: type_on_room_event_countdown_to_start,
            room_event_countdown_cancelled: type_on_room_event_countdown_cancelled
        ): number;
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
        spawn_qh3router: ['void', ['string', 'string', 'string', 'string', 'string', uint16, uint16, 'string', 'pointer', 'pointer', 'pointer', 'pointer']],
        spawn_qh3server: ['void', ['pointer', 'string', 'string', 'string', 'string', 'string', uint16, uint16, 'string', 'pointer', 'pointer', 'pointer', 'pointer', 'pointer']],
        spawn_qserver: [int, ['string', 'string', 'string', 'string', 'string', 'pointer', 'pointer', 'pointer', 'pointer', 'pointer', 'pointer', 'pointer', 'pointer', 'pointer', 'pointer', 'pointer', 'pointer']],
        get_crc32: [ulong, ['string', int]],
        mod_crc32: [ulong, [ulong, 'string', size_t]],
        qh3server_try_send_response: ['void', ['pointer', 'pointer', 'string', size_t, 'string', size_t]],
        test_func: [int, []],
        get_live_connection_count: [uint, ['pointer']],
        get_device_public_ip: ['string', []],
    }) as interface_serverplugin;

    debug_print(LOG_LEVEL_0, __LOGTAG__, 'serverplugin loaded successfully.');
    debug_print(LOG_LEVEL_0, __LOGTAG__, "current directory:", process.cwd());
    serverplugin.setup_signal_handler();
    serverplugin.pre_init_serverplugin_sdk();
}