import { server as qserver } from '../gameserver/gameserver';
import { debug_error, debug_print, LOG_LEVEL_0, LOG_LEVEL_4 } from '../helpers/sdktypes';
import { serversdk } from '../helpers/libserverplugin';

// namespace app {
export class custom_room implements qserver.interface_room {
    private static __LOGTAG__: string = `custom_room`;
    private room_id: number;
    private bet_id: string;
    private native_server: serversdk.qserver_ptr;
    constructor(native_server: serversdk.qserver_ptr, room_id: number, bet_id: string) {
        this.room_id = room_id;
        this.bet_id = bet_id;
        this.native_server = native_server;
    }

    onroom_create(room: number, room_ptr: serversdk.room_ptr, bet_id: string): void {
        debug_print(LOG_LEVEL_4, custom_room.__LOGTAG__, `room_event_create called r:${room}, b:${bet_id}`);
    }
    onroom_start(): void {
        debug_print(LOG_LEVEL_4, custom_room.__LOGTAG__, `room_event_start called r:${this.room_id}`);
    }
    onroom_player_added(room_ptr: serversdk.room_ptr, pid: string, cid_hash: number): void {
        debug_print(LOG_LEVEL_4, custom_room.__LOGTAG__, `room_event_player_added called r:${this.room_id}, pid:${pid}, h:${cid_hash.toString(16)}`);
    }
    onroom_message(room_ptr: serversdk.room_ptr, pid: string, cid_hash: number, msg: string): void {
        debug_print(LOG_LEVEL_4, custom_room.__LOGTAG__, `room_event_message called r:${this.room_id}, pid:${pid}, h:${cid_hash.toString(16)}, msg:${msg}`);
        this.broadcast_except(room_ptr, cid_hash, msg)
    }
    onroom_player_removed(room_ptr: serversdk.room_ptr, pid: string, cid_hash: number): void {
        debug_print(LOG_LEVEL_4, custom_room.__LOGTAG__, `room_event_player_removed called r:${this.room_id}, pid:${pid}, h:${cid_hash.toString(16)}`);
    }
    onroom_end(room_ptr: serversdk.room_ptr): void {
        debug_print(LOG_LEVEL_4, custom_room.__LOGTAG__, `room_event_end called r:${this.room_id}`);
    }
    onroom_countdown_to_start(room_ptr: serversdk.room_ptr, count: number, max_count: number): void {
        debug_print(LOG_LEVEL_4, custom_room.__LOGTAG__, `room_event_countdown_to_start called r:${this.room_id} - ${count}/${max_count}`);
    }
    onroom_countdown_cancelled(room_ptr: serversdk.room_ptr): void {
        debug_print(LOG_LEVEL_4, custom_room.__LOGTAG__, `room_event_countdown_cancelled called r:${this.room_id}`);
    }
    can_allow_reconnection(room_ptr: serversdk.room_ptr, pid: string, cid_hash: number): boolean {
        throw new Error('Method not implemented.');
    }

    // broadcast and send
    broadcast_except(room_ptr: serversdk.room_ptr, cid_hash: number, message: string): boolean {
        return serversdk.sdklib.room_broadcast_except(this.native_server, room_ptr, cid_hash, message);
    }
    broadcast(room_ptr: serversdk.room_ptr, message: string): void {
        serversdk.sdklib.room_broadcast(this.native_server, room_ptr, message);
    }
    send_to(room_ptr: serversdk.room_ptr, cid_hash: number, message: string): boolean {
        return serversdk.sdklib.room_send_to(this.native_server, room_ptr, cid_hash, message);
    }
}

export class custom_gameserver implements qserver.interface_gameserver {
    private static __LOGTAG__: string = `custom_gameserver`;
    private gameserver_instance?: qserver.gameserver;

    private on_gameserver_start_cb: (native_server: any) => Promise<void>;
    private on_gameserver_stop_cb: (native_server: any) => Promise<void>;

    constructor(on_gameserver_start_cb: (native_server: any) => Promise<void>, on_gameserver_stop_cb: (native_server: any) => Promise<void>) {
        this.on_gameserver_start_cb = on_gameserver_start_cb;
        this.on_gameserver_stop_cb = on_gameserver_stop_cb;
    }

    ongameserver_pre_start(native_server: serversdk.qserver_ptr): void {
    }
    ongameserver_start(native_server: serversdk.qserver_ptr): void {
        this.on_gameserver_start_cb(native_server);
    }
    ongameserver_stop(native_server: serversdk.qserver_ptr): void {
        this.on_gameserver_stop_cb(native_server);
    }
    ongameserver_error(native_server: serversdk.qserver_ptr, error_code: number): void {
    }
    ongameserver_room_create(native_server: serversdk.qserver_ptr, room: number, bet_id: string): qserver.interface_room {
        return new custom_room(native_server, room, bet_id);
    }

    public async start_custom_gameserver(): Promise<void> {
        if (this.gameserver_instance != null) {
            debug_error(custom_gameserver.__LOGTAG__, `gameserver_instance not null !!!`);
            return;
        }
        this.gameserver_instance = new qserver.gameserver(this);
        this.gameserver_instance.run();
    }
}
// }