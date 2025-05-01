//
//  Copyright 2024 homenet25
//  room.cpp
//  qserver
//
//  Created by Arun A on 28/10/23.
//

#include "room.hpp"

#include "../../networkcommon/source/roommessage.hpp"

int room::room_id_counter = 0;

// MARK: - room
room::room(roomserver_interface* interface, const roomconfig& room_config) : ROOM_ID(room::room_id_counter++), CREATION_TIME(ev_now(interface->get_netowrk_main_loop())), ROOM_CONFIG(room_config), roomserverinterface(interface) {
	native_server = dynamic_cast<qnetworkserver*>(interface);
	observer = (roomserverinterface != nullptr) ? roomserverinterface->get_main_observer() : nullptr;
	set_loop(roomserverinterface->get_netowrk_main_loop());
	// set_state(ROOM_WAITING);
}

room::~room() {
	if (!performed_cleanup) {
		cleanup();
		debug_print_warn(__LOGTAG__, "avoid cleanup in destructor, since it will not call the derived class overriden functions !!!");
	}
	const qstring& host_id = roomserverinterface->get_host_id();
	const qstring& port_id = roomserverinterface->get_port_id();
	qhiredis* hiredis = roomserverinterface->get_hiredis();
	const qstring& key = get_room_signature(state == ROOM_WAITING ? "wroom:" : "room:", host_id, port_id);
	long long count_waiting_room_of_this_type = 0;
	int result = hiredis->decr_by(key, 1, count_waiting_room_of_this_type);
	debug_warn_cond(__LOGTAG__, result != 0, "hiredis decr_by failed for key %s, result %d", key.c_str(), result);

	debug_print_important(__LOGTAG__, "room %d: destructor", ROOM_ID);
}

void room::cleanup() {
	kick_all_except(nullptr);
	for (auto it = playermap.cbegin(); it != playermap.cend(); it++) {
		player* player_to_rem = (*it).second;
		debug_print_important2(__LOGTAG__, "room %d: player %0x: removed ", ROOM_ID, player_to_rem->qconnection->cid_hash_val);
		onroom_player_removed(player_to_rem);
		GX_DELETE(player_to_rem);
	}
	destroy_all_disconnected_players();
	performed_cleanup = true;
	set_state(ROOM_DESTROY);
}

void room::send_event_player_add_or_remove(player* p, bool add) {
	std::vector<room_player*>* players = nullptr;
	message_base* msg = nullptr;
	if (add) {
		msg_room_server_event_player_add* msg_add = (msg_room_server_event_player_add*) msg_room_server_event_player_add::create();
		msg_add->room_id = ROOM_ID;
		players = &msg_add->players;
		msg = msg_add;
	} else {
		msg_room_server_event_player_remove* msg_remove = (msg_room_server_event_player_remove*) msg_room_server_event_player_remove::create();
		msg_remove->room_id = ROOM_ID;
		players = &msg_remove->players;
		msg = msg_remove;
	}

	for (auto it = playermap.cbegin(); it != playermap.cend(); it++) {
		player* player_ptr = (*it).second;
		room_player* pobj = DEBUG_NEW room_player();
		pobj->hash = player_ptr->qconnection->cid_hash_val;
		pobj->pid = player_ptr->pid;
		pobj->flag = (p == player_ptr);
		players->push_back(pobj);
	}

	Document doc;
	msg->serialize(doc, doc.GetAllocator());

	// send event
	for (auto it = playermap.cbegin(); it != playermap.cend(); it++) {
		Value::MemberIterator itr = doc.FindMember("self");
		if (itr != doc.MemberEnd()) {
			// Remove the member from the document/object
			doc.RemoveMember(itr);
		}
		player* player_ptr = (*it).second;
		doc.AddMember("self", Value().SetUint(player_ptr->qconnection->cid_hash_val), doc.GetAllocator());

		// Convert JSON document to string
		StringBuffer buffer;
		Writer<StringBuffer> writer(buffer);
		doc.Accept(writer);
		player_ptr->qconnection->sendmessage(buffer.GetString(), buffer.GetSize(), true);
	}

	GX_DELETE(msg);
}

void room::send_event_room_start_or_end(bool room_start) {
	msg_room_server_event_base* msg = nullptr;
	if (room_start) {
		msg_room_server_event_start* msg_start = (msg_room_server_event_start*) msg_room_server_event_start::create();
		msg_start->room_id = ROOM_ID;
		msg = msg_start;
	} else {
		msg_room_server_event_end* msg_end = (msg_room_server_event_end*) msg_room_server_event_end::create();
		msg_end->room_id = ROOM_ID;
		msg = msg_end;
	}

	Document doc;
	msg->serialize(doc, doc.GetAllocator());
	//	doc.SetObject();
	//
	//	// Create a JSON object to hold the data
	//	Document::AllocatorType& allocator = doc.GetAllocator();
	//	doc.AddMember("room_event", Value().SetString(room_start ? "room_start" : "room_end", allocator), allocator);
	//	doc.AddMember("room_id", Value().SetInt(ROOM_ID), allocator);
	//	// Convert JSON document to string
	StringBuffer buffer;
	Writer<StringBuffer> writer(buffer);
	doc.Accept(writer);

	// send event
	for (auto it = playermap.cbegin(); it != playermap.cend(); it++) {
		player* player_ptr = (*it).second;
		player_ptr->qconnection->sendmessage(buffer.GetString(), buffer.GetSize(), true);
	}
	GX_DELETE(msg);
}

void room::onroom_create() {
	if (observer) {
		observer->room_event_create(native_server, this->ROOM_ID, this);
	}
}

void room::onroom_start() {
	if (observer) {
		observer->room_event_start(native_server, this->ROOM_ID, this);
	}
}

void room::onroom_player_added(player* p) {
	if (observer) {
		observer->room_event_player_added(native_server, this->ROOM_ID, this, p->pid, p->qconnection->cid_hash_val);
	}
}
void room::onroom_message(player* p, unsigned long recv_len, const uint8_t* buf) {
	const char* char_buf = (const char*) buf;
	debug_print(LOG_LEVEL_3, __LOGTAG__, "room %d: received '%.*s' from player %0x", ROOM_ID, recv_len, char_buf, p->qconnection->cid_hash_val);
	if (observer) {
		observer->room_event_message(native_server, this->ROOM_ID, this, p->pid, p->qconnection->cid_hash_val, recv_len, buf);
	}
}
void room::onroom_player_removed(player* p) {
	if (observer) {
		observer->room_event_player_removed(native_server, this->ROOM_ID, this, p->pid, p->qconnection->cid_hash_val);
	}
}
void room::onroom_end() {
	if (observer) {
		observer->room_event_end(native_server, this->ROOM_ID, this);
	}
}

void room::onroom_destroy() {
	if (observer) {
		observer->room_event_destroy(native_server, this->ROOM_ID, this);
	}
}

void room::onroom_countdown_to_start(int count, int max_count) {
	if (observer) {
		observer->room_event_countdown_to_start(native_server, this->ROOM_ID, this, count, max_count);
	}
}
void room::onroom_countdown_cancelled() {
	if (observer) {
		observer->room_event_countdown_cancelled(native_server, this->ROOM_ID, this);
	}
}
bool room::can_allow_reconnection(unsigned cid_hash) {
	UNUSED(cid_hash);
	return false;
}
void room::pass_message_to_room(player* p, ssize_t recv_len, const uint8_t* buf) {
	onroom_message(p, recv_len, buf);
}
ssize_t room::try_add_connection(qconn_io* qconnection, const qstring& pid, bool& replaced_by_disconnected_player, unsigned prev_cid_hash_val) {
	replaced_by_disconnected_player = false;
	if (qconnection == nullptr) {
		debug_print_error(__LOGTAG__, "room %d: try_add_connection: qconnection == null !!!", ROOM_ID);
		return -1;
	}
	if (playermap.find(qconnection->cid_hash_val) != playermap.end()) {
		debug_print_error(__LOGTAG__, "room %d: qconnection already in the playermap !!!", ROOM_ID);
		return -2;
	}
	struct disconnected_player* disconnected_player_state = find_in_disconnected_players(prev_cid_hash_val);
	bool reconnection = prev_cid_hash_val > 0 && disconnected_player_state != nullptr;
	if (!reconnection && state > ROOM_WAITING) {
		if (!ROOM_CONFIG.ALLOW_JOIN_AFTER_START) {
			debug_print_error(__LOGTAG__, "room %d: room not in waiting state and allow_join_after_start==false !!!", ROOM_ID);
			return -3;
		}
	}
	if ((int) playermap.size() >= ROOM_CONFIG.MAX_PLAYERS) {
		debug_print_error(__LOGTAG__, "room %d: room max cpacity reached !!!", ROOM_ID);
		return -4;
	}
	if (reconnection) {
		if (!can_allow_reconnection(qconnection->cid_hash_val)) {
			debug_print_warn(__LOGTAG__, "room %d: reconnection rejected !!!", ROOM_ID);
			return -5;
		}
	}
	player* player_ptr = nullptr;
	// check on disconnection list
	if (reconnection) {
		replaced_by_disconnected_player = true;
		player_ptr = disconnected_player_state->player_ptr;
		player_ptr->qconnection = qconnection;	// setting the new connection for player.
		remove_from_disconnected_players(prev_cid_hash_val, false);
		debug_print_important(__LOGTAG__, "room %d: player %0x's [prev_cid_hash_val hash (%0x)] entry removed from disconnected hash list. count(%d)", ROOM_ID, qconnection->cid_hash_val, prev_cid_hash_val, disconnected_players_count());
		debug_print_important2(__LOGTAG__, "room %d: player %0x re-added", ROOM_ID, qconnection->cid_hash_val);
	}

	// if couldn't find on disconnection list, create new
	if (player_ptr == nullptr) {
		player_ptr = DEBUG_NEW player(qconnection, pid);
		debug_print_important2(__LOGTAG__, "room %d: player %0x added", ROOM_ID, qconnection->cid_hash_val);
	}
	playermap[qconnection->cid_hash_val] = player_ptr;
	send_event_player_add_or_remove(player_ptr, true);
	onroom_player_added(player_ptr);
	if (is_min_capacity_reached() && get_state() < ROOM_START) {
		if ((int) playermap.size() == ROOM_CONFIG.MAX_PLAYERS) {
			if (cancel_and_destroy_timer(count_down_timer)) {
				count_down_timer = nullptr;
			}
			set_state(ROOM_START);
		} else {
			debug_print_important2(__LOGTAG__, "room %d: start count down  ...", ROOM_ID);
			const int MAX_COUNT_DOWN = 5;
			const float DELAY_BETWEEN_COUNT_DOWN = 1.5f;
			cancel_and_destroy_timer(count_down_timer);
			count_down_timer = schedule_count_timer(
				[this](qtimer& timer) {
					debug_print_important(__LOGTAG__, "room %d: count down %d  ...", ROOM_ID, timer.count);
					if (timer.count > 0) {
						onroom_countdown_to_start(timer.count, MAX_COUNT_DOWN);
					} else {
						set_state(ROOM_START);
					}
				},
				DELAY_BETWEEN_COUNT_DOWN, MAX_COUNT_DOWN);
			UNUSED(count_down_timer);
		}
	}
	return playermap.size();
}

player* room::get_player(qconn_io* qconnection) {
	if (qconnection == nullptr) {
		debug_print_error(__LOGTAG__, "room %d: f:get_player: qconnection == null !!!", ROOM_ID);
		return nullptr;
	}
	std::map<unsigned, player*>::iterator it = playermap.find(qconnection->cid_hash_val);
	if (it == playermap.end()) {
		debug_print_error(__LOGTAG__, "room %d: f:get_player: qconnection not in the playermap !!! %0x", ROOM_ID, qconnection->cid_hash_val);
		return nullptr;
	}
	return it->second;
}

player* room::get_player(unsigned cid_hash) {
	std::map<unsigned, player*>::iterator it = playermap.find(cid_hash);
	if (it != playermap.end()) {
		return it->second;
	}
	debug_print_error(__LOGTAG__, "room %d: f:get_player: cid_hash not in the playermap !!! %0x", ROOM_ID, cid_hash);
	return nullptr;
}

// Function to find a player by hash in the hash table
struct disconnected_player* room::find_in_disconnected_players(unsigned hash) {
	struct disconnected_player* found_player = nullptr;
	HASH_FIND(hh, disconnected_players, &hash, sizeof(unsigned), found_player);
	return found_player;
}

void room::remove_from_disconnected_players(unsigned hash, bool delete_player) {
	struct disconnected_player* player_to_remove = nullptr;
	HASH_FIND(hh, disconnected_players, &hash, sizeof(unsigned), player_to_remove);
	if (player_to_remove) {
		HASH_DEL(disconnected_players, player_to_remove);
		if (delete_player) {
			GX_DELETE(player_to_remove->player_ptr);
		}
		GX_DELETE(player_to_remove);
		debug_print_important2(__LOGTAG__, "Removed player with hash: %u from disconnected_players", hash);
	}
}

// Function to add a player to the hash table
void room::add_to_disconnected_players(player* player_ptr) {
	if (player_ptr == nullptr || player_ptr->qconnection == nullptr) {
		debug_print_error(__LOGTAG__, "f:add_to_disconnected_player_list - player or qconnection NULL pointer ");
		return;
	}
	// Allocate memory for the new disconnected_player entry
	struct disconnected_player* new_player = DEBUG_NEW struct disconnected_player;
	if (new_player == nullptr) {
		debug_print_error(__LOGTAG__, "f:add_to_disconnected_player_list - Failed to allocate memory for disconnected_player");
		return;
	}

	// Set the hash and player pointer
	unsigned hash = player_ptr->qconnection->cid_hash_val;
	new_player->hash = hash;
	new_player->player_ptr = player_ptr;
	new_player->player_ptr->qconnection = nullptr;	// because server will destroy this connection in near future.
	// Add to the hash table using HASH_ADD
	HASH_ADD(hh, disconnected_players, hash, sizeof(unsigned), new_player);
	debug_print_important2(__LOGTAG__, "added player with hash: %u to disconnected_players", hash);
}

void room::destroy_all_disconnected_players() {
	unsigned total_disconnected_cids = disconnected_players_count();
	struct disconnected_player *p = nullptr, *tmp = nullptr;
	HASH_ITER(hh, disconnected_players, p, tmp) {
		HASH_DEL(disconnected_players, p);
		GX_DELETE(p->player_ptr);
		GX_DELETE(p);
	}
	debug_print(LOG_LEVEL_0, __LOGTAG__, "All (%d) disconnected players removed", total_disconnected_cids);
}

// Function to print all players in the hash table (for demonstration)
void room::print_disconnected_players() {
	struct disconnected_player *p = nullptr, *tmp = nullptr;
	debug_print_important(__LOGTAG__, "Disconnected players");
	HASH_ITER(hh, disconnected_players, p, tmp) {
		printf("Hash: %u\n", p->hash);
	}
}

ssize_t room::remove_connection(qconn_io* qconnection) {
	if (qconnection == nullptr) {
		debug_print_error(__LOGTAG__, "room %d: f:remove_connection - qconnection == null !!!", ROOM_ID);
		return -1;
	}
	std::map<unsigned, player*>::iterator it = playermap.find(qconnection->cid_hash_val);
	if (it == playermap.end()) {
		debug_print_error(__LOGTAG__, "room %d: f:remove_connection - qconnection not in the playermap !!! %s", ROOM_ID, qconnection->cid);
		remove_from_disconnected_players(qconnection->cid_hash_val, false);
		return -1;
	}
	player* removed_player = (*it).second;
	send_event_player_add_or_remove(removed_player, false);
	playermap.erase(it);
	debug_print_important2(__LOGTAG__, "room %d: player %0x removed", ROOM_ID, qconnection->cid_hash_val);
	onroom_player_removed(removed_player);
	//	GX_DELETE(removed_player);	// Better to cache this than delete. He may rejoin.

	if (state > ROOM_UNINITIALISED && state < ROOM_START && !is_min_capacity_reached()) {
		if (cancel_and_destroy_timer(count_down_timer)) {
			count_down_timer = nullptr;
		}
		onroom_countdown_cancelled();
		debug_print_important2(__LOGTAG__, "room %d: count down cancelled ...", ROOM_ID);
	}

	// player leaving between gameplay
	if (state >= ROOM_START) {
		if ((int) playermap.size() < ROOM_CONFIG.MIN_PLAYERS) {
			set_state(ROOM_END);
			kick_all_except(nullptr);
		} else {
			if (!find_in_disconnected_players(qconnection->cid_hash_val)) {
				add_to_disconnected_players(removed_player);
			}
		}
	}

	// if not in disconnection list. Delete the player
	if (!find_in_disconnected_players(qconnection->cid_hash_val)) {
		GX_DELETE(removed_player);
	}

	//    if (playermap.size()==0 && state>=ROOM_START) {
	//        set_state(ROOM_END);
	//    }
	return playermap.size();
}

void room::kick_all_except(qconn_io* qconnection) {
	debug_print_important(__LOGTAG__, "room %d: kick all", ROOM_ID);
	for (auto it = playermap.cbegin(); it != playermap.cend(); it++) {
		player* player_ptr = it->second;
		if (player_ptr->qconnection == qconnection) {  // to avoid recursive Close.
			continue;
		}
		player_ptr->qconnection->close();
		debug_print_important(__LOGTAG__, "room %d: kick player %0x", ROOM_ID, player_ptr->qconnection->cid_hash_val);
	}
}

void room::set_state(states state) {
	states prev_state = this->state;
	this->state = state;
	if (prev_state != this->state) {
		on_state_change(prev_state);
	}
}

void room::on_state_change(states prev_state) {
	switch (state) {
		case ROOM_UNINITIALISED: {
			break;
		}
		case ROOM_WAITING: {
			if (prev_state == ROOM_UNINITIALISED) {
				debug_print_important2(__LOGTAG__, "room %d: create", ROOM_ID);
				onroom_create();
			}
		} break;
		case ROOM_START: {
			roomserverinterface->onroom_pre_start(this);
			debug_print_important2(__LOGTAG__, "room %d: start", ROOM_ID);
			send_event_room_start_or_end(true);
			onroom_start();
			break;
		}
		case ROOM_END: {
			debug_print_important2(__LOGTAG__, "room %d: end", ROOM_ID);
			send_event_room_start_or_end(false);
			onroom_end();
			break;
		}
		case ROOM_DESTROY: {
			debug_print_important2(__LOGTAG__, "room %d: destroy", ROOM_ID);
			onroom_destroy();
		}
	}
}

void room::print_info() {
	debug_print_important2(__LOGTAG__, "room %d: state : %s, t:%4.2fs, p:%d", ROOM_ID, get_state_string().c_str(), since_creation(), playermap.size());
}

ev_tstamp room::since_creation() {
	return ev_now(roomserverinterface->get_netowrk_main_loop()) - CREATION_TIME;
}

const qstring& room::get_state_string() {
	return states_string[state];
}

void room::broadcast(unsigned long recv_len, const uint8_t* buf) {
	const char* char_buf = (const char*) buf;
	debug_print(LOG_LEVEL_2, __LOGTAG__, "room %d: f:broadcast %.*s", ROOM_ID, recv_len, char_buf);
	for (auto it = playermap.cbegin(); it != playermap.cend(); it++) {
		player* player_ptr = it->second;
		player_ptr->qconnection->sendmessage(char_buf, recv_len, true);
	}
}

bool room::broadcast_except(player* p, unsigned long recv_len, const uint8_t* buf) {
	const char* char_buf = (const char*) buf;
	std::map<unsigned, player*>::iterator it_except = playermap.find(p->qconnection->cid_hash_val);
	if (it_except == playermap.end()) {
		debug_warn(LOG_LEVEL_0, __LOGTAG__, "room %d: f:broadcast_except - player %0x not found in map, ignoring !!!", ROOM_ID, p->qconnection->cid_hash_val);
		return false;
	}

	for (auto it = playermap.cbegin(); it != playermap.cend(); it++) {
		player* player_ptr = it->second;
		if (it_except == it) {
			continue;
		}
		player_ptr->qconnection->sendmessage(char_buf, recv_len, true);
	}
	return true;
}

bool room::sendto(player* p, unsigned long recv_len, const uint8_t* buf) {
	std::map<unsigned, player*>::iterator it = playermap.find(p->qconnection->cid_hash_val);
	if (it == playermap.end()) {
		debug_warn(LOG_LEVEL_0, __LOGTAG__, "room %d: f:sendto - player %0x not found in map, ignoring !!!", ROOM_ID, p->qconnection->cid_hash_val);
		return false;
	}
	p->qconnection->sendmessage((const char*) buf, recv_len, true);
	return true;
}

qstring room::get_room_signature(const qstring& prefix, const qstring& host_id, const qstring& port_id) {
	qstring signature(qstring::format_string("%s%s#%s-%d-%d-%d-%d", prefix.c_str(), host_id.c_str(), port_id.c_str(), ROOM_CONFIG.MIN_PLAYERS, ROOM_CONFIG.MAX_PLAYERS, ROOM_CONFIG.BET_AMOUNTX, ROOM_CONFIG.REWARD_MULTIPLIERX));
	return signature;
}
