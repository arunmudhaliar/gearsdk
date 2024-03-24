//
//  room.cpp
//  qserver
//
//  Created by Arun A on 28/10/23.
//

#include "room.hpp"

#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using namespace rapidjson;

int room::room_id_counter = 0;

// MARK: - room
room::room(roomserver_interface* interface, const roomconfig& room_config)
	: room_id(room::room_id_counter++),
	  creation_time(ev_now(interface->get_netowrk_main_loop())),
	  room_config(room_config),
	  roomserverinterface(interface) {
	set_state(room_waiting);
}

room::~room() {
	DEBUG_PRINT_IMPORTANT(__LOGTAG__, "room destructor - %d", room_id);
	for (auto it = playermap.cbegin(); it != playermap.cend(); it++) {
		player* player_to_rem = (*it).second;
		DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "player %0x removed from %d", player_to_rem->qconnection->cid_hash_val, room_id);
		onroom_player_removed(player_to_rem);
	}
}

void room::onroom_create() {
}
void room::onroom_start() {
}

void room::send_event_player_add_or_remove(player* p, bool add) {
	Document doc;
	doc.SetObject();

	// Create a JSON object to hold the data
	Document::AllocatorType& allocator = doc.GetAllocator();
	doc.AddMember("event", Value().SetString(add ? "player-add" : "player-remove", allocator), allocator);
	doc.AddMember("room_id", Value().SetInt(room_id), allocator);
	Value players_json_obj(kArrayType);

	for (auto it = playermap.cbegin(); it != playermap.cend(); it++) {
		player* player = (*it).second;
		//        if (p==player) {
		//            continue;
		//        }
		Value player_json_obj(kObjectType);
		// Convert the key and value to `Value` type
		Value f("hash", allocator);
		Value v(Value().SetUint(player->qconnection->cid_hash_val), allocator);
		player_json_obj.AddMember(f, v, allocator);

		Value f2("flag", allocator);
		Value v2(Value().SetBool(p == player), allocator);
		player_json_obj.AddMember(f2, v2, allocator);

		players_json_obj.PushBack(player_json_obj, allocator);
	}
	doc.AddMember("players", players_json_obj, allocator);

	// send event
	for (auto it = playermap.cbegin(); it != playermap.cend(); it++) {
		player* player = (*it).second;
		doc.AddMember("self", Value().SetUint(player->qconnection->cid_hash_val), allocator);

		// Convert JSON document to string
		StringBuffer buffer;
		Writer<StringBuffer> writer(buffer);
		doc.Accept(writer);

		player->qconnection->sendmessage(buffer.GetString(), buffer.GetSize(), true);

		Value::MemberIterator itr = doc.FindMember("self");
		if (itr != doc.MemberEnd()) {
			// Remove the member from the document/object
			doc.RemoveMember(itr);
		}
	}
}

void room::send_event_room_start_or_end(bool room_start) {
	Document doc;
	doc.SetObject();

	// Create a JSON object to hold the data
	Document::AllocatorType& allocator = doc.GetAllocator();
	doc.AddMember("event", Value().SetString(room_start ? "room_start" : "room_end", allocator), allocator);
	doc.AddMember("room_id", Value().SetInt(room_id), allocator);
	// Convert JSON document to string
	StringBuffer buffer;
	Writer<StringBuffer> writer(buffer);
	doc.Accept(writer);

	// send event
	for (auto it = playermap.cbegin(); it != playermap.cend(); it++) {
		player* player = (*it).second;
		player->qconnection->sendmessage(buffer.GetString(), buffer.GetSize(), true);
	}
}

void room::onroom_player_added(player* p) {
	UNUSED(p);
}
void room::onroom_message(player* p, const qstring& msg) {
	DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "room %d: received '%.*s' from player %0x", room_id, msg.length(), msg.c_str(), p->qconnection->cid_hash_val);
}
void room::onroom_player_removed(player* p) {
	UNUSED(p);
}
void room::onroom_end() {
}

void room::pass_message_to_room(player* p, const qstring& msg) {
	onroom_message(p, msg);
}
ssize_t room::try_add_connection(conn_io* qconnection) {
	if (qconnection == nullptr) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "try_add_connection: qconnection == null !!!");
		return -1;
	}
	if (playermap.find(qconnection->cid_hash_val) != playermap.end()) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "qconnection already in the playermap !!!");
		return -2;
	}
	if (state > room_waiting) {
		if (!room_config.allow_join_after_start) {
			DEBUG_PRINT_ERROR(__LOGTAG__, "room not in waiting state !!!");
			return -3;
		}
	}
	if ((int) playermap.size() >= room_config.max_players) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "room max cpacity reached !!!");
		return -4;
	}
	player* player_ = DEBUG_NEW player(qconnection);
	playermap[qconnection->cid_hash_val] = player_;
	DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "player %0x added to room %d", qconnection->cid_hash_val, room_id);
	send_event_player_add_or_remove(player_, true);
	onroom_player_added(player_);
	if (is_min_capacity_reached()) {
		set_state(room_start);
	}
	return playermap.size();
}

player* room::get_player(conn_io* qconnection) {
	if (qconnection == nullptr) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "get_player: qconnection == null !!!");
		return nullptr;
	}
	std::map<unsigned, player*>::iterator it = playermap.find(qconnection->cid_hash_val);
	if (it == playermap.end()) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "get_player: qconnection not in the playermap !!! %s", qconnection->cid);
		return nullptr;
	}
	return (*it).second;
}

ssize_t room::remove_connection(conn_io* qconnection) {
	if (qconnection == nullptr) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "remove_connection: qconnection == null !!!");
		return -1;
	}
	std::map<unsigned, player*>::iterator it = playermap.find(qconnection->cid_hash_val);
	if (it == playermap.end()) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "remove_connection: qconnection not in the playermap !!! %s", qconnection->cid);
		return -1;
	}
	player* removed_player = (*it).second;
	send_event_player_add_or_remove(removed_player, false);
	playermap.erase(it);
	DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "player %0x removed from %d", qconnection->cid_hash_val, room_id);
	onroom_player_removed(removed_player);
	GX_DELETE(removed_player); // Better to cache this than delete. He may rejoin.

	// player leaving between gameplay and gone below min threshold
	if ((int) playermap.size() < room_config.min_players && state >= room_start) {
		set_state(room_end);
		kick_all_except(nullptr);
	}

	//    if (playermap.size()==0 && state>=room_start) {
	//        set_state(room_end);
	//    }
	return playermap.size();
}

void room::kick_all_except(conn_io* qconnection) {
	DEBUG_PRINT_IMPORTANT(__LOGTAG__, "room : kick all");
	for (auto it = playermap.cbegin(); it != playermap.cend(); it++) {
		player* player_ = it->second;
		if (player_->qconnection == qconnection) { // to avoid recursive Close.
			continue;
		}
		player_->qconnection->close();
		DEBUG_PRINT_IMPORTANT(__LOGTAG__, "room : kick player %0x", player_->qconnection->cid_hash_val);
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
	case room_uninitialised: {
		break;
	}
	case room_waiting: {
		if (prev_state == room_uninitialised) {
			DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "room - create %d", room_id);
			onroom_create();
		}
	} break;
	case room_start: {
		roomserverinterface->onroom_pre_start(this);
		DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "room - start %d", room_id);
		send_event_room_start_or_end(true);
		onroom_start();
		break;
	}
	case room_end: {
		DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "room - end %d", room_id);
		send_event_room_start_or_end(false);
		onroom_end();
		break;
	}
	}
}

void room::print_info() {
	DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "room %d, state : %s, t:%4.2fs, p:%d",
						   room_id, get_state_string().c_str(), since_creation(), playermap.size());
}

ev_tstamp room::since_creation() {
	return ev_now(roomserverinterface->get_netowrk_main_loop()) - creation_time;
}

const qstring& room::get_state_string() {
	return states_string[state];
}

void room::broadcast(const qstring& msg) {
	DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "room %d: broadcast %s", room_id, msg.c_str());
	for (auto it = playermap.cbegin(); it != playermap.cend(); it++) {
		player* player_ = it->second;
		player_->qconnection->sendmessage(msg, true);
	}
}

void room::broadcast_except(player* p, const qstring& msg) {
	std::map<unsigned, player*>::iterator it_except = playermap.find(p->qconnection->cid_hash_val);
	if (it_except == playermap.end()) {
		DEBUG_WARN(LOG_LEVEL_0, __LOGTAG__, "broadcast_except - player %0x not found in map, ignoring !!!", p->qconnection->cid_hash_val);
		return;
	}

	for (auto it = playermap.cbegin(); it != playermap.cend(); it++) {
		player* player_ = it->second;
		if (it_except == it) {
			continue;
		}
		player_->qconnection->sendmessage(msg, true);
	}
}

void room::sendto(player* p, const qstring& msg) {
	std::map<unsigned, player*>::iterator it = playermap.find(p->qconnection->cid_hash_val);
	if (it == playermap.end()) {
		DEBUG_WARN(LOG_LEVEL_0, __LOGTAG__, "sendto - player %0x not found in map, ignoring !!!", p->qconnection->cid_hash_val);
		return;
	}
	p->qconnection->sendmessage(msg, true);
}

qstring room::get_room_signature(const qstring& prefix, const qstring& host_id, const qstring& port_id) {
	qstring signature(qstring::format_string("%s%s#%s-%d-%d-%d-%d", prefix.c_str(),
											 host_id.c_str(), port_id.c_str(),
											 room_config.min_players, room_config.max_players,
											 room_config.bet_amountx, room_config.reward_multiplierx));
	return signature;
}
