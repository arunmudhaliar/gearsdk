//
//  roomserver.hpp
//  roomserver
//
//  Created by Arun A on 26/10/23.
//

#ifndef roomserver_hpp
#define roomserver_hpp

#include "qnetworkserver.hpp"
#include "room.hpp"

#include <map>
#include <vector>

#undef __LOGTAG__
#define __LOGTAG__ "roomserver"

#define WAITING_ROOM_ZOMBIE_CHECK_TIMER 30.0
#define WAITING_ROOM_ZOMBIE_THRESHOLD 10.0

class roomserver : public qnetworkserver, public interfaceroom {
public:
    inline struct ev_loop * get_netowrk_main_loop() override final {
        return get_mainloop();
    }
protected:
    void on_network_server_end() override final;
    void on_network_server_begin() override final;
    void on_message(ssize_t recv_len, uint8_t* buf, qpeerconnection* qconnection) override final;
    void on_connection(qpeerconnection* qconnection) override final;
    void on_destroy_connection(qpeerconnection* qconnection) override final;
    
    void onroom_create(room*) override;
    void onroom_pre_start(room*) override final;
    void onroom_start(room*) override;
    void onroom_player_added(room*, player*) override;
    virtual void onroom_message(room*, player*, const qstring& msg) = 0;
    void onroom_player_removed(room*, player*) override;
    void onroom_end(room*) override;
    
    // timers
    void on_timer_check_zombie_rooms(qtimer& qtimer_);
    
    room* create_waiting_room();
    
    qtimer_sceduler scheduler;
    std::vector<room*> waiting_rooms;
    std::vector<room*> rooms;
    std::map<unsigned, room*> connection_map;
    ssize_t zombie_rooms = 0;
};

#endif /* roomserver_hpp */
