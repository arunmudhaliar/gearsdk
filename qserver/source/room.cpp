//
//  room.cpp
//  qserver
//
//  Created by Arun A on 28/10/23.
//

#include "room.hpp"

int room::roomID = 0;

ssize_t room::try_add_connection(qpeerconnection* qconnection) {
    if (qconnection == nullptr) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "qconnection == null !!!");
        return -1;
    }
    if (playermap.find(qconnection) != playermap.end()) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "qconnection already in the playermap !!!");
        return -2;
    }
    if (state!=room_waiting) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "room not in waiting state !!!");
        return -3;
    }
    if (playermap.size()>=room_config.max_players) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "room max cpacity reached !!!");
        return -4;
    }
    player* player_ = DEBUG_NEW player(qconnection);
    playermap[qconnection] = player_;
    DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "player %0x added to room %d", qconnection->cid_hash_val, room_index);
    roominterface->onplayer_added(this, player_);
    if(is_min_capacity_reached()) {
        set_state(room_start);
    }
    return playermap.size();
}

ssize_t room::remove_connection(qpeerconnection* qconnection) {
    if (qconnection == nullptr) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "qconnection == null !!!");
        return -1;
    }
    std::map<qpeerconnection*, player*>::iterator it = playermap.find(qconnection);
    if (it == playermap.end()) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "qconnection not in the playermap !!! %s", qconnection->cid);
        return -1;
    }
    player* removed_player = (*it).second;
    playermap.erase(it);
    DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "player %0x removed from %d", qconnection->cid_hash_val, room_index);
    roominterface->onplayer_removed(this, removed_player);
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
                DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "room - create %d", room_index);
                roominterface->onroom_create(this);
            }
        }
            break;
        case room_start: {
            roominterface->onroom_pre_start(this);
            DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "room - start %d", room_index);
            roominterface->onroom_start(this);
            break;
        }
        case room_end: {
            DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "room - end %d", room_index);
            roominterface->onroom_end(this);
            break;
        }
    }
}


room::room(interfaceroom* interface, const roomconfig& room_config) :
roominterface(interface),
room_config(room_config),
room_index(room::roomID++),
creation_time(ev_now(interface->get_netowrk_main_loop()))
{
    set_state(room_waiting);
}

room::~room() {
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "Room destructor");
    for(auto it = playermap.cbegin();it!=playermap.cend();it++) {
        player* player_to_rem = (*it).second;
        DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "player %0x removed from %d", player_to_rem->qconnection->cid_hash_val, room_index);
        roominterface->onplayer_removed(this, player_to_rem);
    }
}

void room::print_info() {
    DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "room %d, state : %d, t:%4.2fs, p:%d",
                           room_index, state, since_creation(), playermap.size());
}

ev_tstamp room::since_creation() {
    return ev_now(roominterface->get_netowrk_main_loop()) - creation_time;
}
