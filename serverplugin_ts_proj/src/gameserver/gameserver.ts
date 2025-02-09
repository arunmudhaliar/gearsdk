import { debug_error, debug_print, debug_warn, LOG_LEVEL_1, LOG_LEVEL_2, LOG_LEVEL_4 } from "../helpers/sdktypes";
import { server_inf_reader } from "../helpers/serverinforeader";
import { serversdk } from '../helpers/libserverplugin';

export namespace server {
    export interface interface_gameserver {
        ongameserver_pre_start(native_server: serversdk.qserver_ptr): void;
        ongameserver_start(native_server: serversdk.qserver_ptr): void;
        ongameserver_stop(native_server: serversdk.qserver_ptr): void;
        ongameserver_error(native_server: serversdk.qserver_ptr, error_code: number): void;
        ongameserver_room_create(native_server: serversdk.qserver_ptr, room: number, bet_id: string): interface_room;
    }

    export interface interface_room {
        onroom_create(room: number, room_ptr: serversdk.room_ptr, bet_id: string): void;
        onroom_start(room: number, room_ptr: serversdk.room_ptr): void;
        onroom_player_added(room_ptr: serversdk.room_ptr, pid: string, cid_hash: number): void;
        onroom_message(room_ptr: serversdk.room_ptr, pid: string, cid_hash: number, msg: string): void;
        onroom_player_removed(room_ptr: serversdk.room_ptr, pid: string, cid_hash: number): void;
        onroom_end(room_ptr: serversdk.room_ptr): void;
        onroom_countdown_to_start(room_ptr: serversdk.room_ptr, count: number, max_count: number): void;
        onroom_countdown_cancelled(room_ptr: serversdk.room_ptr): void;
        can_allow_reconnection(room_ptr: serversdk.room_ptr, pid: string, cid_hash: number): boolean;

        broadcast_except(
            room_ptr: serversdk.room_ptr,
            cid_hash: number,
            message: string
        ): boolean;
        broadcast(
            room_ptr: serversdk.room_ptr,
            message: string
        ): void;
        send_to(
            room_ptr: serversdk.room_ptr,
            cid_hash: number,
            message: string
        ): boolean;
    }

    export class gameserver {
        private static __LOGTAG__: string = `gameserver`;
        private qserver_config: serversdk.qserver_input_config = {
            server_address: server_inf_reader.get_instance().get_value('gameserver_address'),
            redis_address: server_inf_reader.get_instance().get_value('gameserver_redis_uri'),
            zk_uri: server_inf_reader.get_instance().get_value('gameserver_zk_uri'),
            root_dir: process.cwd(),
            app_id: server_inf_reader.get_instance().get_value('app_id')
        };
        private gameserver_interface: interface_gameserver | undefined;
        private static gserver_id_cntr: number = 0;
        private gserver_id: number = 0;

        constructor(gameserver_interface: interface_gameserver) {
            this.gameserver_interface = gameserver_interface;
            this.gserver_id = ++gameserver.gserver_id_cntr;
        }

        private rooms: Map<number, Map<number, interface_room>> = new Map<number, Map<number, interface_room>>();

        protected on_server_pre_start: serversdk.type_on_qserver_pre_start = (native_server: serversdk.qserver_ptr) => {
            debug_print(LOG_LEVEL_2, gameserver.__LOGTAG__, `on_server_pre_start, server_id: ${this.gserver_id}`);
            this.gameserver_interface?.ongameserver_pre_start(native_server);
        }
        protected on_server_start: serversdk.type_on_qserver_start = async (native_server: serversdk.qserver_ptr, ip: string, port: number) => {
            debug_print(LOG_LEVEL_2, gameserver.__LOGTAG__, `on_server_start ${ip}:${port}, server_id: ${this.gserver_id}`);
            if (!this.rooms.has(this.gserver_id)) {
                this.rooms.set(this.gserver_id, new Map<number, interface_room>());
            } else {
                debug_error(gameserver.__LOGTAG__, `native_server already present in rooms map !!!. This need to check !!!, server_id: ${this.gserver_id}`);
            }
            this.gameserver_interface?.ongameserver_start(native_server);
        }
        protected on_server_stop: serversdk.type_on_qserver_stop = async (native_server: serversdk.qserver_ptr) => {
            debug_print(LOG_LEVEL_2, gameserver.__LOGTAG__, `on_server_stop:, server_id: ${this.gserver_id}`);
            // TODO(amudaliar): need to gracefully remove all rooms
            this.gameserver_interface?.ongameserver_stop(native_server);
            serversdk.sdklib.qserver_release_callbacks(native_server);
        }
        protected on_server_error: serversdk.type_on_qserver_error = (native_server: serversdk.qserver_ptr, error_code: number) => {
            debug_error(gameserver.__LOGTAG__, `on_server_error: ${error_code}, server_id: ${this.gserver_id}`);
            // TODO(amudaliar): need to gracefully remove all rooms if the error cant be recoverable
            this.gameserver_interface?.ongameserver_error(native_server, error_code);
        }

        protected room_event_create: serversdk.type_on_room_event_create = (native_server: serversdk.qserver_ptr, room: number, room_ptr: serversdk.room_ptr) => {
            if (!this.gameserver_interface) {
                return;
            }

            if (this.rooms.has(this.gserver_id)) {
                if (this.rooms.get(this.gserver_id)?.has(room)) {
                    debug_warn(gameserver.__LOGTAG__, `room already present in rooms map !!!, server_id: ${this.gserver_id}`);
                } else {
                    let bet_id = "";
                    let room_interface: interface_room = this.gameserver_interface.ongameserver_room_create(native_server, room, bet_id);
                    if (room_interface) {
                        this.rooms.get(this.gserver_id)?.set(room, room_interface);
                        room_interface.onroom_create(room, room_ptr, bet_id);
                    } else {
                        debug_error(gameserver.__LOGTAG__, `ongameserver_room_create returned null !!!, server_id: ${this.gserver_id}`);
                    }
                }
            } else {
                debug_error(gameserver.__LOGTAG__, `native_server not present in rooms map !!!, server_id: ${this.gserver_id}`);
            }

        }

        protected room_event_start: serversdk.type_on_room_event_start = (native_server: serversdk.qserver_ptr, room: number, room_ptr: serversdk.room_ptr) => {
            if (!this.rooms.has(this.gserver_id)) {
                return;
            }
            let native_server_buffer = this.rooms.get(this.gserver_id);
            if (native_server_buffer?.has(room)) {
                native_server_buffer.get(room)?.onroom_start(room, room_ptr);
            }
        }

        protected room_event_player_added: serversdk.type_on_room_event_player_added = (native_server: serversdk.qserver_ptr, room: number, room_ptr: serversdk.room_ptr, pid: string, cid_hash: number) => {
            if (!this.rooms.has(this.gserver_id)) {
                return;
            }
            let native_server_buffer = this.rooms.get(this.gserver_id);
            if (native_server_buffer?.has(room)) {
                native_server_buffer.get(room)?.onroom_player_added(room_ptr, pid, cid_hash);
            }
        }

        protected room_event_message: serversdk.type_on_room_event_message = (native_server: serversdk.qserver_ptr, room: number, room_ptr: serversdk.room_ptr, pid: string, cid_hash: number, message: string) => {
            if (!this.rooms.has(this.gserver_id)) {
                return;
            }
            let native_server_buffer = this.rooms.get(this.gserver_id);
            if (native_server_buffer?.has(room)) {
                native_server_buffer.get(room)?.onroom_message(room_ptr, pid, cid_hash, message);
            }
        }

        protected room_event_player_removed: serversdk.type_on_room_event_player_removed = (native_server: serversdk.qserver_ptr, room: number, room_ptr: serversdk.room_ptr, pid: string, cid_hash: number) => {
            if (!this.rooms.has(this.gserver_id)) {
                return;
            }
            let native_server_buffer = this.rooms.get(this.gserver_id);
            if (native_server_buffer?.has(room)) {
                native_server_buffer.get(room)?.onroom_player_removed(room_ptr, pid, cid_hash);
            }
        }

        protected room_event_end: serversdk.type_on_room_event_end = (native_server: serversdk.qserver_ptr, room: number, room_ptr: serversdk.room_ptr) => {
            if (!this.rooms.has(this.gserver_id)) {
                return;
            }
            let native_server_buffer = this.rooms.get(this.gserver_id);
            if (native_server_buffer?.has(room)) {
                native_server_buffer.get(room)?.onroom_end(room_ptr);
            }
        }

        protected room_event_countdown_to_start: serversdk.type_on_room_event_countdown_to_start = (native_server: serversdk.qserver_ptr, room: number, room_ptr: serversdk.room_ptr, count: number, max_count: number) => {
            if (!this.rooms.has(this.gserver_id)) {
                return;
            }
            let native_server_buffer = this.rooms.get(this.gserver_id);
            if (native_server_buffer?.has(room)) {
                native_server_buffer.get(room)?.onroom_countdown_to_start(room_ptr, count, max_count);
            }
        }

        protected room_event_countdown_cancelled: serversdk.type_on_room_event_countdown_cancelled = (native_server: serversdk.qserver_ptr, room: number, room_ptr: serversdk.room_ptr) => {
            if (!this.rooms.has(this.gserver_id)) {
                return;
            }
            let native_server_buffer = this.rooms.get(this.gserver_id);
            if (native_server_buffer?.has(room)) {
                native_server_buffer.get(room)?.onroom_countdown_cancelled(room_ptr);
            }
        }

        public async run(): Promise<number> {
            let spawned = serversdk.sdklib.spawn_qserver(
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