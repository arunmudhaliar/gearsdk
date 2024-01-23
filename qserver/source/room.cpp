//
//  room.cpp
//  qserver
//
//  Created by Arun A on 28/10/23.
//

#include "room.hpp"

int room::room_id_counter = 0;

room::room(roomserver_interface* interface, const roomconfig& room_config) :
roomserverinterface(interface),
room_config(room_config),
room_id(room::room_id_counter++),
creation_time(ev_now(interface->get_netowrk_main_loop()))
{
    set_state(room_waiting);
}

room::~room() {
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "room destructor - %d", room_id);
    for(auto it = playermap.cbegin();it!=playermap.cend();it++) {
        player* player_to_rem = (*it).second;
        DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "player %0x removed from %d", player_to_rem->qconnection->cid_hash_val, room_id);
        onroom_player_removed(player_to_rem);
    }
}

void room::onroom_create() {
}
void room::onroom_start() {
}
void room::onroom_player_added(player* p) {
}
void room::onroom_message(player* p, const qstring& msg) {
    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "room %d: received '%.*s' from player %0x", room_id, msg.length(), msg.c_str(), p->qconnection->cid_hash_val);
}
void room::onroom_player_removed(player* p) {
}
void room::onroom_end() {
}

void room::pass_message_to_room(player* p, const qstring& msg) {
    onroom_message(p, msg);
}
ssize_t room::try_add_connection(qpeerconnection* qconnection) {
    if (qconnection == nullptr) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "try_add_connection: qconnection == null !!!");
        return -1;
    }
    if (playermap.find(qconnection) != playermap.end()) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "qconnection already in the playermap !!!");
        return -2;
    }
    if (state>room_waiting) {
        if (!room_config.allow_join_after_start) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "room not in waiting state !!!");
            return -3;
        }
    }
    if (playermap.size()>=room_config.max_players) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "room max cpacity reached !!!");
        return -4;
    }
    player* player_ = DEBUG_NEW player(qconnection);
    playermap[qconnection] = player_;
    DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "player %0x added to room %d", qconnection->cid_hash_val, room_id);
    onroom_player_added(player_);
    if(is_min_capacity_reached()) {
        set_state(room_start);
    }
    return playermap.size();
}

player* room::get_player(qpeerconnection* qconnection) {
    if (qconnection == nullptr) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "get_player: qconnection == null !!!");
        return nullptr;
    }
    std::map<qpeerconnection*, player*>::iterator it = playermap.find(qconnection);
    if (it == playermap.end()) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "get_player: qconnection not in the playermap !!! %s", qconnection->cid);
        return nullptr;
    }
    return (*it).second;
}

ssize_t room::remove_connection(qpeerconnection* qconnection) {
    if (qconnection == nullptr) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "remove_connection: qconnection == null !!!");
        return -1;
    }
    std::map<qpeerconnection*, player*>::iterator it = playermap.find(qconnection);
    if (it == playermap.end()) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "remove_connection: qconnection not in the playermap !!! %s", qconnection->cid);
        return -1;
    }
    player* removed_player = (*it).second;
    playermap.erase(it);
    DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "player %0x removed from %d", qconnection->cid_hash_val, room_id);
    onroom_player_removed(removed_player);
    GX_DELETE(removed_player);  // Better to cache this than delete. He may rejoin.
    
    // player leaving between gameplay and gone below min threshold
    if (playermap.size()<room_config.min_players && state>=room_start) {
        kick_all_except(nullptr);
    }
    
    if (playermap.size()==0 && state>=room_start) {
        set_state(room_end);
    }
    return playermap.size();
}

void room::kick_all_except(qpeerconnection* qconnection) {
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "room : kick all");
    for(auto it = playermap.cbegin();it!=playermap.cend();it++) {
        player* player_ = it->second;
        if (player_->qconnection == qconnection) {  // to avoid recursive Close.
            continue;
        }
        player_->qconnection->close();
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "room : kick player %0x", player_->qconnection->cid_hash_val);
    }
}

void room::set_state(states state) {
    states prev_state = this->state;
    this->state = state;
    if (prev_state!=this->state) {
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
        }
            break;
        case room_start: {
            roomserverinterface->onroom_pre_start(this);
            DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "room - start %d", room_id);
            onroom_start();
            break;
        }
        case room_end: {
            DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "room - end %d", room_id);
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
