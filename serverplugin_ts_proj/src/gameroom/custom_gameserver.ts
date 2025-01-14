import { server as qserver } from '../gameserver/gameserver';
import { debug_error, debug_print, LOG_LEVEL_4 } from '../helpers/sdktypes';

// namespace app {
export class custom_room implements qserver.interface_room {
    private static __LOGTAG__: string = `custom_room`;
    private room_id: number;
    private bet_id: string;
    constructor(room_id: number, bet_id: string) {
        this.room_id = room_id;
        this.bet_id = bet_id;
    }
    onroom_create(room: number, bet_id: string): void {
        debug_print(LOG_LEVEL_4, custom_room.__LOGTAG__, `room_event_create called r:${room}, b:${bet_id}`);
    }
    onroom_start(): void {
        debug_print(LOG_LEVEL_4, custom_room.__LOGTAG__, `room_event_start called r:${this.room_id}`);
    }
    onroom_player_added(pid: string, cid_hash: number): void {
        debug_print(LOG_LEVEL_4, custom_room.__LOGTAG__, `room_event_player_added called r:${this.room_id}, pid:${pid}, h:${cid_hash.toString(16)}`);
    }
    onroom_message(pid: string, cid_hash: number, msg: string): void {
        debug_print(LOG_LEVEL_4, custom_room.__LOGTAG__, `room_event_message called r:${this.room_id}, pid:${pid}, h:${cid_hash.toString(16)}, msg:${msg}`);
    }
    onroom_player_removed(pid: string, cid_hash: number): void {
        debug_print(LOG_LEVEL_4, custom_room.__LOGTAG__, `room_event_player_removed called r:${this.room_id}, pid:${pid}, h:${cid_hash.toString(16)}`);
    }
    onroom_end(): void {
        debug_print(LOG_LEVEL_4, custom_room.__LOGTAG__, `room_event_end called r:${this.room_id}`);
    }
    onroom_countdown_to_start(count: number, max_count: number): void {
        debug_print(LOG_LEVEL_4, custom_room.__LOGTAG__, `room_event_countdown_to_start called r:${this.room_id} - ${count}/${max_count}`);
    }
    onroom_countdown_cancelled(): void {
        debug_print(LOG_LEVEL_4, custom_room.__LOGTAG__, `room_event_countdown_cancelled called r:${this.room_id}`);
    }
    can_allow_reconnection(pid: string, cid_hash: number): boolean {
        throw new Error('Method not implemented.');
    }
}

export class custom_gameserver implements qserver.interface_gameserver {
    private static __LOGTAG__: string = `custom_gameserver`;
    private gameserver_instance?: qserver.gameserver;

    ongameserver_room_create(room: number, bet_id: string): qserver.interface_room {
        return new custom_room(room, bet_id);
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