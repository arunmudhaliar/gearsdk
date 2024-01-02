//
//  roomserver.cpp
//  roomserver
//
//  Created by Arun A on 26/10/23.
//

#include "roomserver.hpp"

void roomserver::on_timer_check_zombie_rooms(qtimer& qtimer_) {
    if (waiting_rooms.size()) {
        std::vector<room*> zombies;
        for(auto it = waiting_rooms.cbegin();it!=waiting_rooms.cend();it++) {
            room* waiting_room = *it;
            if (waiting_room->since_creation() >= WAITING_ROOM_ZOMBIE_THRESHOLD) {
                zombies.push_back(waiting_room);
            }
        }
        if (zombie_rooms!=zombies.size()) {
            DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "[%d] - zombies", zombies.size());
            for(auto it = zombies.cbegin();it!=zombies.cend();it++) {
                room* zombie = *it;
                zombie->print_info();
            }
            
            zombie_rooms = zombies.size();
        }
    }
}

void roomserver::on_network_server_begin() {
    scheduler.set_ev_lopp(get_mainloop());
    type_qtimer_cb timeout_callback = std::bind(&roomserver::on_timer_check_zombie_rooms, this, std::placeholders::_1);
    scheduler.schedule_repeat_timer(timeout_callback, WAITING_ROOM_ZOMBIE_CHECK_TIMER);
    DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "start");
}

void roomserver::on_network_server_end() {
    DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "end");
}

void roomserver::onroom_pre_start(room* r) {
    // check and delete the room from waiting list
    int oldSz = (int)waiting_rooms.size();
    waiting_rooms.erase(std::remove(waiting_rooms.begin(), waiting_rooms.end(), r), waiting_rooms.end());
    if(oldSz<=waiting_rooms.size()) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "onroom_pre_start: coudn't find the room in waiting rooms list. CHECK !!!");
        return;
    }
    
    if (std::find(rooms.begin(), rooms.end(), r) == rooms.end()) {
        rooms.push_back(r);
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "room : removed from waiting list and pushed to rooms list %d", r->room_index);
    } else {
        DEBUG_PRINT_ERROR(__LOGTAG__, "coudn't add the room to rooms list (Duplicate). CHECK !!!");
    }
}

void roomserver::onroom_create(room* r) {
}
void roomserver::onroom_start(room* r) {
}
void roomserver::onroom_player_added(room* r, player* p) {
}
void roomserver::onroom_player_removed(room* r, player* p) {
}
void roomserver::onroom_end(room* room) {
}

void roomserver::on_message(ssize_t recv_len, uint8_t* buf, qpeerconnection* qconnection) {
    // check if he was part of any active room.
    room* room_ = nullptr;
    std::map<unsigned, room*>::iterator iterator =  connection_map.find(qconnection->cid_hash_val);
    if (iterator!=connection_map.end()) {
        room_ = iterator->second;
    }
    if (room_ == nullptr) {
        DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "on_message returning !!!, connection %0x not part of any room.", qconnection->cid_hash_val);
        //connection was not part of any room so far
        return;
    }
    player* player_ = room_->get_player(qconnection);
    if (player_ == nullptr) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "player not found in the room !!!");
        return;
    }
    onroom_message(room_, player_, qstring(buf, recv_len));
}

void roomserver::on_connection(qpeerconnection* qconnection) {
    DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "on_connection: incoming connection %0x", qconnection->cid_hash_val);
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
                if (room_->get_state()==room::states::room_waiting) {
                    DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "on_connection: add to waiting room, map[after add sz:%d] !!! - connection %0x", connection_map.size(), qconnection->cid_hash_val);
                } else if (room_->get_state()>=room::states::room_start) {
                    DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "on_connection: added to room, map[after add sz:%d] !!! - connection %0x", connection_map.size(), qconnection->cid_hash_val);
                }
                break;
            }
        }
    } else {
        // check if he still in the room or not
        player* already_in_room = room_->get_player(qconnection);
        if (already_in_room) {
            DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "on_connection: already part of PREV room %d of user [m-sz:%d] !!! - connection %0x, returning.", room_->room_index, qconnection->cid_hash_val, connection_map.size(), qconnection->cid_hash_val);
            return;
        }
        // check if he can be added back to the same room.
        ssize_t old_count = room_->get_playermap_count();
        ssize_t new_count = room_->try_add_connection(qconnection);
        if (new_count==-2)  {   // already part of the room
            DEBUG_PRINT_ERROR(__LOGTAG__, "on_connection: this can not happen !!!, returning.");
            return;
        }
        connection_added_to_room = new_count>old_count;
        if (!connection_added_to_room ) {
            // remove from old list
            DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "on_connection: can't be added to his prev room, remove him from old hash list");
            connection_map.erase(iterator);
            room_ = nullptr;
        } else {
            DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "on_connection: add player to PREV room of user [m-sz:%d] !!! - connection %0x", connection_map.size(), qconnection->cid_hash_val);
        }
    }
    // create a new room and add him
    if (!connection_added_to_room) {
        room* waiting_room = create_waiting_room();
        waiting_room->try_add_connection(qconnection);  // no need to check for limit since he is our first user in this room.
        room_ = waiting_room;
        connection_map[qconnection->cid_hash_val] = room_;
        DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "on_connection: add to hash[after add sz:%d] !!! - %0x", connection_map.size(), qconnection->cid_hash_val);
    }
}

room* roomserver::create_waiting_room() {
    room* room_ = DEBUG_NEW room(this, roomconfig(2, 4, false));
    waiting_rooms.push_back(room_);
    return room_;
}

void roomserver::on_destroy_connection(qpeerconnection* qconnection) {
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
