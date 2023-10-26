//
//  roomserver.cpp
//  NetworkServer
//
//  Created by Arun A on 26/10/23.
//

#include "roomserver.hpp"

int room::roomID = 0;

ssize_t room::try_add_connection(QPeerConnection* qconnection) {
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
    player* player_ = new player(qconnection);
    playermap[qconnection] = player_;
    roominterface->onplayer_added(this, player_);
    if(is_min_capacity_reached()) {
        set_state(room_start);
    }
    return playermap.size();
}

ssize_t room::remove_connection(QPeerConnection* qconnection) {
    if (qconnection == nullptr) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "qconnection == null !!!");
        return -1;
    }
    std::map<QPeerConnection*, player*>::iterator it = playermap.find(qconnection);
    if (it == playermap.end()) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "qconnection not in the playermap !!! %s", qconnection->cid);
        return -1;
    }
    player* removed_player = (*it).second;
    playermap.erase(it);
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

void room::kick_all_except(QPeerConnection* qconnection) {
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "room : kick all");
    for(auto it = playermap.cbegin();it!=playermap.cend();it++) {
        player* player_ = it->second;
        if (player_->qconnection == qconnection) {  // to avoid recursive Close.
            continue;
        }
        player_->qconnection->Close();
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
                roominterface->onroom_create(this);
            }
        }
            break;
        case room_start: {
            roominterface->onroom_pre_start(this);
            roominterface->onroom_start(this);
            break;
        }
        case room_end: {
            roominterface->onroom_end(this);
            break;
        }
    }
}


room::room(interfaceroom* interface, const roomconfig& room_config) :
roominterface(interface),
room_config(room_config),
room_index(room::roomID++)
{
    set_state(room_waiting);
}

room::~room() {
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "Room destructor");
    for(auto it = playermap.cbegin();it!=playermap.cend();it++) {
        player* player_to_rem = (*it).second;
        roominterface->onplayer_removed(this, player_to_rem);
    }
}

void roomserver::onroom_pre_start(room* room) {
    // check and delete the room from waiting list
    int oldSz = (int)waiting_rooms.size();
    waiting_rooms.erase(std::remove(waiting_rooms.begin(), waiting_rooms.end(), room), waiting_rooms.end());
    if(oldSz<=waiting_rooms.size()) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "coudn't add the room to rooms list. CHECK !!!");
        return;
    }
    
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "room : removed from waiting list and pushed to rooms list %d", room->room_index);
    if (std::find(waiting_rooms.begin(), waiting_rooms.end(), room) == waiting_rooms.end()) {
        rooms.push_back(room);
    } else {
        DEBUG_PRINT_ERROR(__LOGTAG__, "coudn't add the room to rooms list (Duplicate). CHECK !!!");
    }
}
void roomserver::onroom_create(room* room) {
    DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "room - create %d", room->room_index);
}
void roomserver::onroom_start(room* room) {
    DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "room - start %d", room->room_index);
}
void roomserver::onplayer_added(room* room, player* player) {
    DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "player %0x added to room %d", player->qconnection->cid_hash_val, room->room_index);
}
void roomserver::onplayer_removed(room* room, player* player) {
    DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "player %0x removed from %d", player->qconnection->cid_hash_val, room->room_index);
}
void roomserver::onroom_end(room* room) {
    DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "room - end %d", room->room_index);
}

void roomserver::OnMessage(ssize_t recv_len, uint8_t* buf, QPeerConnection* qconnection) {
    
}

void roomserver::OnConnection(QPeerConnection* qconnection) {
    DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "ONConnect !!! - %0x", qconnection->cid_hash_val);
    // check if he was part of any active room.
    room* room_ = nullptr;
    std::map<unsigned, room*>::iterator iterator =  connection_map.find(qconnection->cid_hash_val);
    if (iterator!=connection_map.end()) {
        room_ = iterator->second;
    }
    bool connection_added_to_room = false;
    if (room_ == nullptr) {
        // may be a new connection.
        // so get a waiting room for him
        // search existing waiting rooms.
        for(auto it = waiting_rooms.cbegin();it!=waiting_rooms.cend();it++) {
            room* waiting_room = *it;
            ssize_t old_count = waiting_room->get_playermap_count();
            ssize_t new_count = waiting_room->try_add_connection(qconnection);
            connection_added_to_room = new_count>old_count;
            if (connection_added_to_room) {
                room_ = waiting_room;
                connection_map[qconnection->cid_hash_val] = room_;
                DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "ONConnect - add to map[after add sz:%d] !!! - %0x", connection_map.size(), qconnection->cid_hash_val);
                break;
            }
        }
    } else {
        // check if he can be added back to the same room.
        ssize_t old_count = room_->get_playermap_count();
        ssize_t new_count = room_->try_add_connection(qconnection);
        connection_added_to_room = new_count>old_count;
        if (!connection_added_to_room) {
            // remove from old list
            DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "remove from old hash list");
            connection_map.erase(iterator);
            room_ = nullptr;
        } else {
            DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "ONConnect - add to PREV room of user [m-sz:%d] !!! - %0x", connection_map.size(), qconnection->cid_hash_val);
        }
    }
    // create a new room and add him
    if (!connection_added_to_room) {
        room* waiting_room = create_waiting_room();
        waiting_room->try_add_connection(qconnection);  // no need to check for limit since he is our first user in this room.
        room_ = waiting_room;
        connection_map[qconnection->cid_hash_val] = room_;
        DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "ONConnect - add to hash[after add sz:%d] !!! - %0x", connection_map.size(), qconnection->cid_hash_val);
    }
}

room* roomserver::create_waiting_room() {
    room* room_ = new room(this, roomconfig(2, 4, false));
    waiting_rooms.push_back(room_);
    return room_;
}

void roomserver::OnDestroyConnection(QPeerConnection* qconnection) {
    room* room_ = nullptr;
    std::map<unsigned, room*>::iterator iterator =  connection_map.find(qconnection->cid_hash_val);
    if (iterator!=connection_map.end()) {
        room_ = iterator->second;
    }
    if (room_ == nullptr) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "qconnection not in any room !!! - %0x, [cnt %d]", qconnection->cid_hash_val, connection_map.size());
        return;
    }
    connection_map.erase(iterator);
    
    DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "after-qconnection removed %d, %0x", connection_map.size(), qconnection->cid_hash_val);
    room_->remove_connection(qconnection);
    if (room_->get_state()==room::room_end && room_->get_playermap_count()==0) {
        // delete the room, if the state is in 'END' and player count is zero.
        ssize_t waiting_room_size = waiting_rooms.size();
        waiting_rooms.erase(std::remove(waiting_rooms.begin(), waiting_rooms.end(), room_), waiting_rooms.end());
        DEBUG_ASSERT(__LOGTAG__, (waiting_room_size==waiting_rooms.size()), "check this");   // still in waiting room ???
        ssize_t rooms_size = rooms.size();
        rooms.erase(std::remove(rooms.begin(), rooms.end(), room_), rooms.end());
        DEBUG_ASSERT(__LOGTAG__, (rooms_size>rooms.size()), "check this");   // still in waiting room ???
        GX_DELETE(room_);
        DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "room size %d", rooms.size());
    }
}
