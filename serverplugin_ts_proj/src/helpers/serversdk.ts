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

    export const ulong = ref.types.ulong;
    export const long = ref.types.long;
    export const Bytef = ref.refType(ref.types.uchar); // Pointer to a buffer
    export const size_t = ref.types.size_t;
    export const voidp = ref.refType('void');
    export const nullptr = ref.NULL;
    export const int = ref.types.int;
    export const uint = ref.types.uint;
    export const uint8 = ref.types.uint8;
    export const uint8_p = ref.refType(uint8);
    export const uint16 = ref.types.uint16;
    export const uint64 = ref.types.uint64;

    export type qh3server_ptr = ref.Pointer<void>;
    export type qserver_ptr = ref.Pointer<void>;

    export enum log_lvls { LEVEL_0, LEVEL_1, LEVEL_2, LEVEL_3, LEVEL_4 };
    export enum elog_type { INFO_LOG, DEBUG_LOG, WARN_LOG, ERROR_LOG, LOG_TYPE_MAX };

    const __LOGTAG__: string = `serversdk`;

    let lib_path: string;
    let lib_debug: string = 'libserverplugin-debug';
    let lib_release: string = 'libserverplugin';
    let lib_serverplugin: string = (process.env.NODE_ENV === 'production') ? lib_release : lib_debug;
    if (process.platform === 'darwin') {
        lib_path = path.join(__dirname, `./../serverplugin/${lib_serverplugin}.dylib`);
    } else if (process.platform === 'linux') {
        lib_path = path.join(__dirname, `./../serverplugin/${lib_serverplugin}.so`);
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
    export type type_on_router_pre_start = (router: Buffer, user_arg: Buffer) => void;
    export type type_on_router_start = (router: Buffer, user_arg: Buffer) => void;
    export type type_on_router_stop = (router: Buffer, user_arg: Buffer) => void;
    export type type_on_router_error = (router: Buffer, user_arg: Buffer, error_code: number) => void;

    // qh3server events
    export type type_on_server_pre_start = (server: qh3server_ptr) => void;
    export type type_on_server_start = (router: qh3server_ptr, ip: string, port: number) => void;
    export type type_on_server_stop = (server: qh3server_ptr) => void;
    export type type_on_server_error = (server: qh3server_ptr, error_code: number) => void;
    export type type_on_server_parse = (server: qh3server_ptr, cid: Buffer, cid_len: number, path: string, buffer: string, len: number, headers: string, header_buffer_size: number) => void;

    // qserver events
    export type type_on_qserver_pre_start = (server: qserver_ptr) => void;
    export type type_on_qserver_start = (router: qserver_ptr, ip: string, port: number) => void;
    export type type_on_qserver_stop = (server: qserver_ptr) => void;
    export type type_on_qserver_error = (server: qserver_ptr, error_code: number) => void;

    // room events
    export type type_on_room_event_create = (native_server: qserver_ptr, room: number) => void;
    export type type_on_room_event_start = (native_server: qserver_ptr, room: number) => void;
    export type type_on_room_event_player_added = (native_server: qserver_ptr, room: number, pid: string, cid_hash: number) => void;
    export type type_on_room_event_message = (native_server: qserver_ptr, room: number, pid: string, cid_hash: number, message: string) => void;
    export type type_on_room_event_player_removed = (native_server: qserver_ptr, room: number, pid: string, cid_hash: number) => void;
    export type type_on_room_event_end = (native_server: qserver_ptr, room: number) => void;
    export type type_on_room_event_countdown_to_start = (native_server: qserver_ptr, room: number, count: number, max_count: number) => void;
    export type type_on_room_event_countdown_cancelled = (native_server: qserver_ptr, room: number) => void;

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
            error_cb: type_on_router_error,
            user_arg: any
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
        qh3server_try_send_response(native_server: qh3server_ptr, cid: Buffer, cid_len: number, payload: string, len: number, user_data: string | null, user_data_len: number): void;
        get_live_connection_count(native_server: qh3server_ptr): number;
        get_device_public_ip(): string;
        qh3server_logfile(native_server: qh3server_ptr, lvl: log_lvls, type: elog_type, tag: string, pid: string, roomid: string, message: string): number;
        qh3server_stats_count(native_server: qh3server_ptr, counter: string, count_val: number, session: string, pid: string, version: string /*= ``*/, epic: string /*= ``*/, myth: string /*= ``*/, legend: string /*= ``*/,
            story: string /*= ``*/, message: string /*= ``*/): number;
        qserver_logfile(native_server: qh3server_ptr, lvl: log_lvls, type: elog_type, tag: string, pid: string, roomid: string, message: string): number;
        qserver_stats_count(native_server: qh3server_ptr, counter: string, count_val: number, session: string, pid: string, version: string /*= ``*/, epic: string /*= ``*/, myth: string /*= ``*/, legend: string /*= ``*/,
            story: string /*= ``*/, message: string /*= ``*/): number;
    }

    // Load the C library and cast it to the interface_qh3serverplugin
    export const serverplugin = ffi.Library(lib_path, {
        setup_signal_handler: ['void', []],
        pre_init_serverplugin_sdk: ['void', []],
        spawn_qh3router: ['void', ['string', 'string', 'string', 'string', 'string', uint16, uint16, 'string', 'pointer', 'pointer', 'pointer', 'pointer', 'pointer']],
        spawn_qh3server: ['void', ['pointer', 'string', 'string', 'string', 'string', 'string', uint16, uint16, 'string', 'pointer', 'pointer', 'pointer', 'pointer', 'pointer']],
        spawn_qserver: [int, ['string', 'string', 'string', 'string', 'string', 'pointer', 'pointer', 'pointer', 'pointer', 'pointer', 'pointer', 'pointer', 'pointer', 'pointer', 'pointer', 'pointer', 'pointer']],
        get_crc32: [ulong, ['string', int]],
        mod_crc32: [ulong, [ulong, 'string', size_t]],
        qh3server_try_send_response: ['void', ['pointer', uint8_p, uint16, 'string', size_t, 'string', size_t]],
        get_live_connection_count: [uint, ['pointer']],
        get_device_public_ip: ['string', []],
        qh3server_logfile: [uint64, ['pointer', int, int, 'string', 'string', 'string', 'string']],
        qh3server_stats_count: [size_t, ['pointer', 'string', long, 'string', 'string', 'string', 'string', 'string', 'string', 'string', 'string']],
        qserver_logfile: [uint64, ['pointer', int, int, 'string', 'string', 'string', 'string']],
        qserver_stats_count: [size_t, ['pointer', 'string', long, 'string', 'string', 'string', 'string', 'string', 'string', 'string', 'string']],
    }) as interface_serverplugin;

    debug_print(LOG_LEVEL_0, __LOGTAG__, 'serverplugin loaded successfully.');
    debug_print(LOG_LEVEL_0, __LOGTAG__, "current directory:", process.cwd());
    serverplugin.setup_signal_handler();
    serverplugin.pre_init_serverplugin_sdk();
}