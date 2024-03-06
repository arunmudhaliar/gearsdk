//
//  gameserver.cpp
//  NetworkServer
//
//  Created by Arun A on 15/10/23.
//

#include "gameserver.hpp"

// MARK: - game_room
game_room::game_room(roomserver_interface* interface, const roomconfig& room_config) : room(interface, room_config) {
    
}

game_room::~game_room() {
    
}

void game_room::onroom_create() {
}
void game_room::onroom_start() {
}
void game_room::onroom_player_added(player* p) {
}
void game_room::onroom_message(player* p, const qstring& msg) {
    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "room %d: received '%.*s' from player %0x", room_id, msg.length(), msg.c_str(), p->qconnection->cid_hash_val);
    // passing to other clients
    broadcast_except(p, msg);
}
void game_room::onroom_player_removed(player* p) {
}
void game_room::onroom_end() {
}

// MARK: - gameserver
//----------------------------------------------------------------------------
//---------------------------------gameserver---------------------------------
//----------------------------------------------------------------------------
void gameserver::on_network_server_init() {
    roomserver::on_network_server_init();
    DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "gameserver::init");
    message_parser.register_message_type<msg_room_config>();
    message_parser.register_message_type<msg_room_server_shutdown>();
}

room* gameserver::create_room(const msg_room_config* room_config_msg) {
    return DEBUG_NEW game_room(this, roomconfig(room_config_msg));
}
