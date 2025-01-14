import { pid } from "process";
import { debug_error, debug_print, debug_warn, LOG_LEVEL_1, LOG_LEVEL_2, LOG_LEVEL_4 } from "../helpers/sdktypes";
import { server_config_reader } from "../helpers/serverconfig-reader";
import { serversdk } from "../helpers/serversdk";
import * as ffi from 'ffi-napi';

export namespace server {
    export interface interface_gameserver {
        ongameserver_room_create(room: number, bet_id: string): interface_room;
    }

    export interface interface_room {
        onroom_create(room: number, bet_id: string): void;
        onroom_start(): void;
        onroom_player_added(pid: string, cid_hash: number): void;
        onroom_message(pid: string, cid_hash: number, msg: string): void;
        onroom_player_removed(pid: string, cid_hash: number): void;
        onroom_end(): void;
        onroom_countdown_to_start(count: number, max_count: number): void;
        onroom_countdown_cancelled(): void;
        can_allow_reconnection(pid: string, cid_hash: number): boolean;
    }

    export class gameserver {
        private static __LOGTAG__: string = `gameserver`;
        private qserver_config: serversdk.qserver_input_config = {
            server_address: server_config_reader.get_instance().get_value('gameserver_address'),
            redis_address: server_config_reader.get_instance().get_value('gameserver_redis_uri'),
            zk_uri: server_config_reader.get_instance().get_value('gameserver_zk_uri'),
            root_dir: process.cwd(),
            app_id: server_config_reader.get_instance().get_value('app_id')
        };
        private gameserver_interface: interface_gameserver | undefined;
        private static gserver_id_cntr: number = 0;
        private gserver_id: number = 0;

        constructor(gameserver_interface: interface_gameserver) {
            this.gameserver_interface = gameserver_interface;
            this.gserver_id = ++gameserver.gserver_id_cntr;
        }

        private rooms: Map<number, Map<number, interface_room>> = new Map<number, Map<number, interface_room>>();

        protected on_server_pre_start = ffi.Callback('void', ['pointer'], (native_server: serversdk.qserver_ptr) => {
            debug_print(LOG_LEVEL_2, gameserver.__LOGTAG__, `on_server_pre_start, server_id: ${this.gserver_id}`);
        }) as unknown as serversdk.type_on_qserver_pre_start;
        protected on_server_start = ffi.Callback('void', ['pointer', 'string', serversdk.uint16], async (native_server: serversdk.qserver_ptr, ip: string, port: number) => {
            debug_print(LOG_LEVEL_2, gameserver.__LOGTAG__, `on_server_start ${ip}:${port}, server_id: ${this.gserver_id}`);
            if (!this.rooms.has(this.gserver_id)) {
                this.rooms.set(this.gserver_id, new Map<number, interface_room>());
            } else {
                debug_error(gameserver.__LOGTAG__, `native_server already present in rooms map !!!. This need to check !!!, server_id: ${this.gserver_id}`);
            }
        }) as unknown as serversdk.type_on_qserver_start;
        protected on_server_stop = ffi.Callback('void', ['pointer'], async (native_server: serversdk.qserver_ptr) => {
            debug_print(LOG_LEVEL_2, gameserver.__LOGTAG__, `on_server_stop:, server_id: ${this.gserver_id}`);
            // TODO(amudaliar): need to gracefully remove all rooms
        }) as unknown as serversdk.type_on_qserver_stop;
        protected on_server_error = ffi.Callback('void', ['pointer'], (native_server: serversdk.qserver_ptr, error_code: number) => {
            debug_error(gameserver.__LOGTAG__, `on_server_error: ${error_code}, server_id: ${this.gserver_id}`);
            // TODO(amudaliar): need to gracefully remove all rooms if the error cant be recoverable
        }) as unknown as serversdk.type_on_qserver_error;

        protected room_event_create = ffi.Callback('void', [serversdk.voidp, serversdk.int], (native_server: serversdk.qserver_ptr, room: number) => {
            if (!this.gameserver_interface) {
                return;
            }

            if (this.rooms.has(this.gserver_id)) {
                if (this.rooms.get(this.gserver_id)?.has(room)) {
                    debug_warn(gameserver.__LOGTAG__, `room already present in rooms map !!!, server_id: ${this.gserver_id}`);
                } else {
                    let bet_id = "";
                    let room_interface: interface_room = this.gameserver_interface.ongameserver_room_create(room, bet_id);
                    if (room_interface) {
                        this.rooms.get(this.gserver_id)?.set(room, room_interface);
                        room_interface.onroom_create(room, bet_id);
                    } else {
                        debug_error(gameserver.__LOGTAG__, `ongameserver_room_create returned null !!!, server_id: ${this.gserver_id}`);
                    }
                }
            } else {
                debug_error(gameserver.__LOGTAG__, `native_server not present in rooms map !!!, server_id: ${this.gserver_id}`);
            }

        }) as unknown as serversdk.type_on_room_event_create;

        protected room_event_start = ffi.Callback('void', [serversdk.voidp, serversdk.int], (native_server: serversdk.qserver_ptr, room: number) => {
            if (!this.rooms.has(this.gserver_id)) {
                return;
            }
            let native_server_buffer = this.rooms.get(this.gserver_id);
            if (native_server_buffer?.has(room)) {
                native_server_buffer.get(room)?.onroom_start();
            }
        }) as unknown as serversdk.type_on_room_event_start;

        protected room_event_player_added = ffi.Callback('void', [serversdk.voidp, serversdk.int, 'string', serversdk.uint], (native_server: serversdk.qserver_ptr, room: number, pid: string, cid_hash: number) => {
            if (!this.rooms.has(this.gserver_id)) {
                return;
            }
            let native_server_buffer = this.rooms.get(this.gserver_id);
            if (native_server_buffer?.has(room)) {
                native_server_buffer.get(room)?.onroom_player_added(pid, cid_hash);
            }
        }) as unknown as serversdk.type_on_room_event_player_added;

        protected room_event_message = ffi.Callback('void', [serversdk.voidp, serversdk.int, 'string', serversdk.uint, 'string'], (native_server: serversdk.qserver_ptr, room: number, pid: string, cid_hash: number, message: string) => {
            if (!this.rooms.has(this.gserver_id)) {
                return;
            }
            let native_server_buffer = this.rooms.get(this.gserver_id);
            if (native_server_buffer?.has(room)) {
                native_server_buffer.get(room)?.onroom_message(pid, cid_hash, message);
            }
        }) as unknown as serversdk.type_on_room_event_message;

        protected room_event_player_removed = ffi.Callback('void', [serversdk.voidp, serversdk.int, 'string', serversdk.uint], (native_server: serversdk.qserver_ptr, room: number, pid: string, cid_hash: number) => {
            if (!this.rooms.has(this.gserver_id)) {
                return;
            }
            let native_server_buffer = this.rooms.get(this.gserver_id);
            if (native_server_buffer?.has(room)) {
                native_server_buffer.get(room)?.onroom_player_removed(pid, cid_hash);
            }
        }) as unknown as serversdk.type_on_room_event_player_removed;

        protected room_event_end = ffi.Callback('void', [serversdk.voidp, serversdk.int], (native_server: serversdk.qserver_ptr, room: number) => {
            if (!this.rooms.has(this.gserver_id)) {
                return;
            }
            let native_server_buffer = this.rooms.get(this.gserver_id);
            if (native_server_buffer?.has(room)) {
                native_server_buffer.get(room)?.onroom_end();
            }
        }) as unknown as serversdk.type_on_room_event_end;

        protected room_event_countdown_to_start = ffi.Callback('void', [serversdk.voidp, serversdk.int, serversdk.int, serversdk.int], (native_server: serversdk.qserver_ptr, room: number, count: number, max_count: number) => {
            if (!this.rooms.has(this.gserver_id)) {
                return;
            }
            let native_server_buffer = this.rooms.get(this.gserver_id);
            if (native_server_buffer?.has(room)) {
                native_server_buffer.get(room)?.onroom_countdown_to_start(count, max_count);
            }
        }) as unknown as serversdk.type_on_room_event_countdown_to_start;

        protected room_event_countdown_cancelled = ffi.Callback('void', [serversdk.voidp, serversdk.int], (native_server: serversdk.qserver_ptr, room: number) => {
            if (!this.rooms.has(this.gserver_id)) {
                return;
            }
            let native_server_buffer = this.rooms.get(this.gserver_id);
            if (native_server_buffer?.has(room)) {
                native_server_buffer.get(room)?.onroom_countdown_cancelled();
            }
        }) as unknown as serversdk.type_on_room_event_countdown_cancelled;

        public async run(): Promise<number> {
            let spawned = serversdk.serverplugin.spawn_qserver(
                this.qserver_config.server_address,
                this.qserver_config.redis_address,
                this.qserver_config.zk_uri,
                this.qserver_config.root_dir,
                this.qserver_config.app_id,
                this.on_server_pre_start,
                this.on_server_start,
                this.on_server_stop,
                this.on_server_error,
                this.room_event_create,
                this.room_event_start,
                this.room_event_player_added,
                this.room_event_message,
                this.room_event_player_removed,
                this.room_event_end,
                this.room_event_countdown_to_start,
                this.room_event_countdown_cancelled
            );
            return spawned;
        }
    }
}