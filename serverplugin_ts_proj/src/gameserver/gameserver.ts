import { debug_error, debug_print, LOG_LEVEL_1, LOG_LEVEL_2, LOG_LEVEL_4 } from "../helpers/sdktypes";
import { server_config_reader } from "../helpers/serverconfig-reader";
import { serversdk } from "../helpers/serversdk";
import * as ffi from 'ffi-napi';

export namespace server {
    export class gameserver {
        private static __LOGTAG__: string = `gameserver`;
        private qserver_config: serversdk.qserver_input_config = {
            server_address: server_config_reader.get_instance().get_value('gameserver_address'),
            redis_address: server_config_reader.get_instance().get_value('gameserver_redis_uri'),
            zk_uri: server_config_reader.get_instance().get_value('gameserver_zk_uri'),
            root_dir: process.cwd(),
            app_id: server_config_reader.get_instance().get_value('app_id')
        };

        // private rooms: Map<Buffer, Map<Buffer, room>> = new Map<Buffer, Map<Buffer, room>>();

        protected on_server_pre_start = ffi.Callback('void', ['pointer'], (native_server: serversdk.qserver_ptr) => {
            debug_print(LOG_LEVEL_2, gameserver.__LOGTAG__, `on_server_pre_start`);
        }) as unknown as serversdk.type_on_qserver_pre_start;
        protected on_server_start = ffi.Callback('void', ['pointer', 'string', serversdk.uint16], async (native_server: serversdk.qserver_ptr, ip: string, port: number) => {
            debug_print(LOG_LEVEL_2, gameserver.__LOGTAG__, `on_server_start ${ip}:${port}`);
        }) as unknown as serversdk.type_on_qserver_start;
        protected on_server_stop = ffi.Callback('void', ['pointer'], async (native_server: serversdk.qserver_ptr) => {
            debug_print(LOG_LEVEL_2, gameserver.__LOGTAG__, `on_server_stop:`);
        }) as unknown as serversdk.type_on_qserver_stop;
        protected on_server_error = ffi.Callback('void', ['pointer'], (native_server: serversdk.qserver_ptr, error_code: number) => {
            debug_error(gameserver.__LOGTAG__, `on_server_error: ${error_code}`);
        }) as unknown as serversdk.type_on_qserver_error;

        protected room_event_create = ffi.Callback('void', [serversdk.voidp, serversdk.int], (native_server: serversdk.qserver_ptr, room: number) => {
            debug_print(LOG_LEVEL_4, gameserver.__LOGTAG__, `room_event_create called r:${room}`);
        }) as unknown as serversdk.type_on_room_event_create;

        protected room_event_start = ffi.Callback('void', [serversdk.voidp, serversdk.int], (native_server: serversdk.qserver_ptr, room: number) => {
            debug_print(LOG_LEVEL_4, gameserver.__LOGTAG__, `room_event_start called r:${room}`);
        }) as unknown as serversdk.type_on_room_event_start;

        protected room_event_player_added = ffi.Callback('void', [serversdk.voidp, serversdk.int, 'string', serversdk.uint], (native_server: serversdk.qserver_ptr, room: number, pid: string, cid_hash: number) => {
            debug_print(LOG_LEVEL_4, gameserver.__LOGTAG__, `room_event_player_added called r:${room}, pid:${pid}, h:${cid_hash.toString(16)}`);
        }) as unknown as serversdk.type_on_room_event_player_added;

        protected room_event_message = ffi.Callback('void', [serversdk.voidp, serversdk.int, 'string', serversdk.uint, 'string'], (native_server: serversdk.qserver_ptr, room: number, pid: string, cid_hash: number, message: string) => {
            debug_print(LOG_LEVEL_4, gameserver.__LOGTAG__, `room_event_message called r:${room}, pid:${pid}, h:${cid_hash.toString(16)}, msg:${message}`);
        }) as unknown as serversdk.type_on_room_event_message;

        protected room_event_player_removed = ffi.Callback('void', [serversdk.voidp, serversdk.int, 'string', serversdk.uint], (native_server: serversdk.qserver_ptr, room: number, pid: string, cid_hash: number) => {
            debug_print(LOG_LEVEL_4, gameserver.__LOGTAG__, `room_event_player_removed called r:${room}, pid:${pid}, h:${cid_hash.toString(16)}`);
        }) as unknown as serversdk.type_on_room_event_player_removed;

        protected room_event_end = ffi.Callback('void', [serversdk.voidp, serversdk.int], (native_server: serversdk.qserver_ptr, room: number) => {
            debug_print(LOG_LEVEL_4, gameserver.__LOGTAG__, `room_event_end called r:${room}`);
        }) as unknown as serversdk.type_on_room_event_end;

        protected room_event_countdown_to_start = ffi.Callback('void', [serversdk.voidp, serversdk.int, serversdk.int, serversdk.int], (native_server: serversdk.qserver_ptr, room: number, count: number, max_count: number) => {
            debug_print(LOG_LEVEL_4, gameserver.__LOGTAG__, `room_event_countdown_to_start called r:${room} - ${count}/${max_count}`);
        }) as unknown as serversdk.type_on_room_event_countdown_to_start;

        protected room_event_countdown_cancelled = ffi.Callback('void', [serversdk.voidp, serversdk.int], (native_server: serversdk.qserver_ptr, room: number) => {
            debug_print(LOG_LEVEL_4, gameserver.__LOGTAG__, `room_event_countdown_cancelled called r:${room}`);
        }) as unknown as serversdk.type_on_room_event_countdown_cancelled;

        // private add_room(native_server: Buffer, room: Buffer): room | undefined {
        //     if (!this.rooms.has(native_server)) {
        //         this.rooms.set(native_server, new Map<Buffer, room>());
        //     }
        //     if (!this.rooms.get(native_server)?.has(room)) {
        //         this.rooms.get(native_server)?.set(room, new room());
        //     }
        //     return this.rooms.get(native_server)?.get(room);
        // }

        // private remove_room(native_server: Buffer, room: Buffer): room | undefined {
        //     if (!this.rooms.has(native_server)) {
        //         this.rooms.set(native_server, new Map<Buffer, room>());
        //     }
        //     if (!this.rooms.get(native_server)?.has(room)) {
        //         this.rooms.get(native_server)?.set(room, new room());
        //     }
        //     return this.rooms.get(native_server)?.get(room);
        // }

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