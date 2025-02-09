import * as path from 'path';
import * as fs from "fs";
import os from 'os';
import { debug_print, LOG_LEVEL_0, LOG_LEVEL_4 } from './sdktypes';

export namespace serversdk {
    const __LOGTAG__: string = `serversdk`;
    let lib_path: string;
    let lib_debug: string = `libserverplugin-${os.platform()}-debug.node`;
    let lib_release: string = `libserverplugin-${os.platform()}.node`;
    let lib_debug_path: string = path.join(__dirname, `./../../dist/${lib_debug}`);
    let lib_release_path: string = path.join(__dirname, `./../../dist/${lib_release}`);
    if (fs.existsSync(lib_debug_path)) {
        lib_path = lib_debug_path;
        debug_print(LOG_LEVEL_0, __LOGTAG__, "DEBUG version:", lib_path);
    } else if (fs.existsSync(lib_release_path)) {
        lib_path = lib_release_path;
        debug_print(LOG_LEVEL_0, __LOGTAG__, "RELEASE version:", lib_path);
    } else {
        throw new Error("No valid libserverplugin{configuration}.node file found!");
    }

    export const sdklib = require(lib_path) as serversdk.interface_serverplugin;

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

    export type qh3router_ptr = any;
    export type qh3server_ptr = any;
    export type qserver_ptr = any;
    export type room_ptr = any;

    export enum log_lvls { LEVEL_0, LEVEL_1, LEVEL_2, LEVEL_3, LEVEL_4 };
    export enum elog_type { INFO_LOG, DEBUG_LOG, WARN_LOG, ERROR_LOG, LOG_TYPE_MAX };

    // router events
    export type type_on_router_pre_start = (router: any, cd_data: any) => void;
    export type type_on_router_start = (router: any) => void;
    export type type_on_router_stop = (router: any) => void;
    export type type_on_router_error = (router: any, error_code: number) => void;

    // qh3server events
    export type type_on_server_pre_start = (server: qh3server_ptr) => void;
    export type type_on_server_start = (router: qh3server_ptr, ip: string, port: number) => void;
    export type type_on_server_stop = (server: qh3server_ptr) => void;
    export type type_on_server_error = (server: qh3server_ptr, error_code: number) => void;
    export type type_on_server_parse = (server: qh3server_ptr, cid: any, cid_len: number, path: string, buffer: string, headers: string) => void;

    // qserver events
    export type type_on_qserver_pre_start = (server: qserver_ptr) => void;
    export type type_on_qserver_start = (router: qserver_ptr, ip: string, port: number) => void;
    export type type_on_qserver_stop = (server: qserver_ptr) => void;
    export type type_on_qserver_error = (server: qserver_ptr, error_code: number) => void;

    // room events
    export type type_on_room_event_create = (native_server: qserver_ptr, room: number, room_ptr: room_ptr) => void;
    export type type_on_room_event_start = (native_server: qserver_ptr, room: number, room_ptr: room_ptr) => void;
    export type type_on_room_event_player_added = (native_server: qserver_ptr, room: number, room_ptr: room_ptr, pid: string, cid_hash: number) => void;
    export type type_on_room_event_message = (native_server: qserver_ptr, room: number, room_ptr: room_ptr, pid: string, cid_hash: number, message: string) => void;
    export type type_on_room_event_player_removed = (native_server: qserver_ptr, room: number, room_ptr: room_ptr, pid: string, cid_hash: number) => void;
    export type type_on_room_event_end = (native_server: qserver_ptr, room: number, room_ptr: room_ptr) => void;
    export type type_on_room_event_countdown_to_start = (native_server: qserver_ptr, room: number, room_ptr: room_ptr, count: number, max_count: number) => void;
    export type type_on_room_event_countdown_cancelled = (native_server: qserver_ptr, room: number, room_ptr: room_ptr) => void;


    export interface interface_serverplugin {
        setup_signal_handler(): void;
        pre_init_serverplugin_sdk(): void;
        get_live_connection_count(native_server: qh3server_ptr): number;
        get_device_public_ip(): string;
        get_crc32(str: Buffer): number;
        mod_crc32(adler: number, buf: Buffer | null): number;
        spawn_qh3router(routerAddress: string,
            mongodbUri: string,
            redisAddress: string,
            zkUri: string,
            rootDir: string,
            commandPort: number,
            routerPortReturn: number,
            appId: string,
            preStartCallback: type_on_router_pre_start,
            startCallback: type_on_router_start,
            stopCallback: type_on_router_stop,
            errorCallback: type_on_router_error,
        ): void;
        spawn_qh3server(
            native_router: any,
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
        qh3server_try_send_response(
            native_server: qh3server_ptr,
            cid: any, cid_len: number,
            payload: string,
            payload_len: number
        ): void;
        qh3server_logfile(native_server: qh3server_ptr, lvl: log_lvls, type: elog_type, tag: string, pid: string, roomid: string, message: string): number;
        qh3server_stats_count(native_server: qh3server_ptr, counter: string, count_val: number, session: string, pid: string, version: string /*= ``*/, epic: string /*= ``*/, myth: string /*= ``*/, legend: string /*= ``*/,
            story: string /*= ``*/, message: string /*= ``*/): number;
        qh3server_shutdown(native_server: qh3server_ptr): void;

        // gserver
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
            room_event_create: type_on_room_event_create,
            room_event_start: type_on_room_event_start,
            room_event_player_added: type_on_room_event_player_added,
            room_event_message: type_on_room_event_message,
            room_event_player_removed: type_on_room_event_player_removed,
            room_event_end: type_on_room_event_end,
            room_event_countdown_to_start: type_on_room_event_countdown_to_start,
            room_event_countdown_cancelled: type_on_room_event_countdown_cancelled
        ): number;
        room_broadcast_except(native_server: qserver_ptr,
            room: room_ptr,
            cid_hash: number,
            message: string
        ): boolean;
        room_broadcast(native_server: qserver_ptr,
            room: room_ptr,
            message: string
        ): void;
        room_send_to(native_server: qserver_ptr,
            room: room_ptr,
            cid_hash: number,
            message: string
        ): boolean;
        qserver_release_callbacks(native_server: qserver_ptr): void;
        qh3server_release_callbacks(native_server: qh3server_ptr): void;
        qh3router_release_callbacks(cb_data: any): void;
    }

    debug_print(LOG_LEVEL_0, __LOGTAG__, 'serverplugin loaded successfully.');
    debug_print(LOG_LEVEL_0, __LOGTAG__, "current directory:", process.cwd());

    sdklib.setup_signal_handler();
    sdklib.pre_init_serverplugin_sdk();
}
