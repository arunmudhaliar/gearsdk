//
//  Copyright 2024 homenet25
//  gameserver.cpp
//  NetworkServer
//
//  Created by Arun A on 15/10/23.
//

#include "gameserver.hpp"

// MARK: - game_room
game_room::game_room(roomserver_interface* interface, const roomconfig& room_config) : room(interface, room_config) {}

game_room::~game_room() {}

void game_room::onroom_create() {
	room::onroom_create();
}
void game_room::onroom_start() {
	room::onroom_start();
}
void game_room::onroom_player_added(player* p) {
	room::onroom_player_added(p);
}
void game_room::onroom_message(player* p, const qstring& msg) {
	room::onroom_message(p, msg);
	debug_print(LOG_LEVEL_3, __LOGTAG__, "room %d: received '%.*s' from player %0x", ROOM_ID, msg.length(), msg.c_str(), p->qconnection->cid_hash_val);
	// passing to other clients
	broadcast_except(p, msg);
}
void game_room::onroom_player_removed(player* p) {
	room::onroom_player_removed(p);
}
void game_room::onroom_end() {
	room::onroom_end();
}
bool game_room::can_allow_reconnection(unsigned cid_hash) {
	return true;
}

// MARK: - gameserver
//----------------------------------------------------------------------------
//---------------------------------gameserver---------------------------------
//----------------------------------------------------------------------------
gameserver::gameserver() : roomserver() {}

gameserver::~gameserver() {}

void gameserver::on_network_server_init() {
	roomserver::on_network_server_init();
	debug_print_important2(__LOGTAG__, "gameserver::init");
}

room* gameserver::create_room(const msg_room_config* room_config_msg) {
	return DEBUG_NEW game_room(this, roomconfig(room_config_msg));
}
