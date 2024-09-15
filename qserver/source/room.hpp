//
//  Copyright 2024 homenet25
//  room.hpp
//  qserver
//
//  Created by Arun A on 28/10/23.
//

#ifndef room_hpp
#define room_hpp

#include "../../common/typex.h"
#include "../../networkcommon/source/message.hpp"
#include "qnetworkserver.hpp"

#include <map>
#include <vector>

#undef __LOGTAG__
#define __LOGTAG__ "room"

// MARK: -
struct player {
	player(conn_io* qcon, const qstring& pid) : qconnection(qcon), pid(pid) {}
	~player() { debug_print_important(__LOGTAG__, "player destructor"); }
	conn_io* qconnection;
	qstring pid;
};

// MARK: -
class room;
class roomserver_interface {
   public:
	virtual void onroom_pre_start(room* r) = 0;
	virtual struct ev_loop* get_netowrk_main_loop() = 0;
};

class room_interface {
	virtual void onroom_create() = 0;
	virtual void onroom_start() = 0;
	virtual void onroom_player_added(player* p) = 0;
	virtual void onroom_message(player* p, const qstring& msg) = 0;
	virtual void onroom_player_removed(player* p) = 0;
	virtual void onroom_end() = 0;
	virtual void onroom_countdown_to_start(int count, int max_count) = 0;
	virtual void onroom_countdown_cancelled() = 0;
	virtual bool can_allow_reconnection(unsigned cid_hash) = 0;
};

struct roomconfig {
	roomconfig(const roomconfig& room_config)
		: MIN_PLAYERS(room_config.MIN_PLAYERS), MAX_PLAYERS(room_config.MAX_PLAYERS), BET_AMOUNTX(room_config.BET_AMOUNTX), REWARD_MULTIPLIERX(room_config.REWARD_MULTIPLIERX), ALLOW_JOIN_AFTER_START(room_config.ALLOW_JOIN_AFTER_START) {}
	roomconfig(int min_players, int max_players, intx bet_amountx, intx reward_multiplierx, bool allow_join_after_start)
		: MIN_PLAYERS(min_players), MAX_PLAYERS(max_players), BET_AMOUNTX(bet_amountx), REWARD_MULTIPLIERX(reward_multiplierx), ALLOW_JOIN_AFTER_START(allow_join_after_start) {}
	roomconfig(const msg_room_config* room_config_msg) : MIN_PLAYERS(room_config_msg->min), MAX_PLAYERS(room_config_msg->max), ALLOW_JOIN_AFTER_START(room_config_msg->allow_after_start) {}
	const int MIN_PLAYERS = 1;
	const int MAX_PLAYERS = 1;
	const intx BET_AMOUNTX = 0;	 // Note: bet_amountx & reward_multiplierx are in fixed point values
	const intx REWARD_MULTIPLIERX = FX_TWO;
	const bool ALLOW_JOIN_AFTER_START = false;
};

// MARK: -
class room : public room_interface, public qtimer_scheduler {
   private:
	room() : CREATION_TIME(0), ROOM_CONFIG(roomconfig(1, 1, 0, FX_TWO, false)) {}

   public:
	enum states { ROOM_UNINITIALISED, ROOM_WAITING, ROOM_START, ROOM_END };
	room(roomserver_interface*, const roomconfig& room_config);
	virtual ~room();

	ssize_t try_add_connection(conn_io* qconnection, const qstring& pid, unsigned prev_cid_hash_val = 0);
	ssize_t remove_connection(conn_io* qconnection);
	player* get_player(conn_io* qconnection);
	inline bool is_min_capacity_reached() { return (int) playermap.size() >= ROOM_CONFIG.MIN_PLAYERS; }
	inline bool is_max_capacity_reached() { return (int) playermap.size() >= ROOM_CONFIG.MAX_PLAYERS; }
	inline ssize_t get_playermap_count() { return playermap.size(); }
	states get_state() { return state; }
	const qstring& get_state_string();
	bool is_state(states state) { return this->state == state; }
	void kick_all_except(conn_io* qconnection);
	void print_info();
	ev_tstamp since_creation();

	std::map<unsigned, player*> playermap;
	std::map<unsigned, ev_tstamp> disconnected_players_hash_after_room_start;
	const int ROOM_ID = 0;
	const ev_tstamp CREATION_TIME;

	void pass_message_to_room(player* p, const qstring& msg);

	void broadcast(const qstring& msg);
	void broadcast_except(player* p, const qstring& msg);
	void sendto(player* p, const qstring& msg);
	inline const roomconfig& get_room_config() { return ROOM_CONFIG; }

	qstring get_room_signature(const qstring& prefix, const qstring& host_id, const qstring& port_id);

	bool is_cid_hash_in_disconnected_players_hash_list(unsigned cid_hash);

   protected:
	void onroom_create() override;
	void onroom_start() override;
	void onroom_player_added(player* p) override;
	void onroom_message(player* p, const qstring& msg) override;
	void onroom_player_removed(player* p) override;
	void onroom_end() override;
	void onroom_countdown_to_start(int count, int max_count) override;
	void onroom_countdown_cancelled() override;
	bool can_allow_reconnection(unsigned cid_hash) override;

   private:
	void set_state(states state);
	void on_state_change(states prev_state);

	void send_event_player_add_or_remove(player* p, bool add);
	void send_event_room_start_or_end(bool room_start);

	const roomconfig ROOM_CONFIG;
	states state = ROOM_UNINITIALISED;
	roomserver_interface* roomserverinterface;
	static int room_id_counter;
	qstring states_string[4] = {"uninitialised", "waiting", "start", "end"};
	qtimer* count_down_timer = nullptr;	 // Note :- Do not try to access this timer outside the waiting period, since it may get destroyed by the scheduler.
};

#endif /* room_hpp */
