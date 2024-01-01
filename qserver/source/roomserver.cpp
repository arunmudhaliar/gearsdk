//
//  roomserver.cpp
//  roomserver
//
//  Created by Arun A on 26/10/23.
//

#include "roomserver.hpp"

void roomserver::on_timer_check_zombie_rooms(qtimer& qtimer_) {
    DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "timer_check_zombie_rooms");
    if (waiting_rooms.size()) {
        int zobies = 0;
        for(auto it = waiting_rooms.cbegin();it!=waiting_rooms.cend();it++) {
            room* waiting_room = *it;
            if (waiting_room->since_creation() >= WAITING_ROOM_ZOMBIE_THRESHOLD) {
                waiting_room->print_info();
                zobies++;
            }
        }
        if (zobies>0) {
            DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "[%d] ~~zombies~~", zobies);
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
}
void roomserver::onroom_start(room* room) {
}
void roomserver::onplayer_added(room* room, player* player) {
}
void roomserver::onplayer_removed(room* room, player* player) {
}
void roomserver::onroom_end(room* room) {
}

void roomserver::on_message(ssize_t recv_len, uint8_t* buf, qpeerconnection* qconnection) {
    
}

void roomserver::on_connection(qpeerconnection* qconnection) {
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
