//
//  gameserver.cpp
//  NetworkServer
//
//  Created by Arun A on 15/10/23.
//

#include "gameserver.hpp"

void gameserver::onroom_create(room* r)  {
//    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "room creation %d", r->room_index);
}
void gameserver::onroom_start(room* r)  {
//    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "room start %d", r->room_index);
}
void gameserver::onroom_player_added(room* r , player* p)  {
//    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "player %d added to room %d", p->qconnection->cid_hash_val,  r->room_index);
}
void gameserver::onroom_message(room* r, player* p, const qstring& msg) {
    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "room %d: received '%.*s' from player %0x", r->room_index, msg.length(), msg.c_str(), p->qconnection->cid_hash_val);
}
void gameserver::onroom_player_removed(room* r, player* p)  {
//    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "player %d removed from room %d", p->qconnection->cid_hash_val,  r->room_index);
}
void gameserver::onroom_end(room* r)  {
//    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "room end %d",  r->room_index);
}
