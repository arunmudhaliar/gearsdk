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
#include "../../networkcommon/source/message.hpp"

#include <map>
#include <vector>

#undef __LOGTAG__
#define __LOGTAG__ "roomserver"

#define WAITING_ROOM_ZOMBIE_CHECK_TIMER 30.0
#define WAITING_ROOM_ZOMBIE_THRESHOLD 10.0

// MARK: -
class roomserver : public qnetworkserver, public roomserver_interface {
public:
    inline struct ev_loop * get_netowrk_main_loop() override final {
        return get_mainloop();
    }
protected:
    void on_network_server_begin() override final;
    void on_network_server_init() override;
    void on_network_server_end() override final;
    void onconnection_message(ssize_t recv_len, uint8_t* buf, conn_io* qconnection) override final;
    void onconnection_connect(conn_io* qconnection) override final;
    void onconnection_connected(conn_io* qconnection) override final;
    void onconnection_destroy(conn_io* qconnection) override final;
    void on_qhiredis_async_key_expired(const qstring&) override;
    
    void onroom_pre_start(room*) override final;
    virtual room* create_room(const msg_room_config* room_config_msg) = 0;
    
    // timers
    void on_timer_check_zombie_rooms(qtimer& qtimer_);
    
    room* create_waiting_room(const msg_room_config* room_config);
    
    qtimer_sceduler scheduler;
    std::vector<room*> waiting_rooms;
    std::vector<room*> rooms;
    std::map<unsigned, room*> connection_map;
    std::map<unsigned, ev_tstamp> new_connections;
    ssize_t zombie_rooms = 0;
    
    message_parser msg_parser;
    qtimer* waiting_room_check_zombie_timer = nullptr;
    
private:
    enum CONN_FLAGS {
        FLAG_ROOM_CONFIG_RECEIVED = (1<<0)
    };
    
    void do_process_roomjoin(conn_io* qconnection, const msg_room_config& room_config_msg);
};

#endif /* roomserver_hpp */
