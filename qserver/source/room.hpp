//
//  room.hpp
//  qserver
//
//  Created by Arun A on 28/10/23.
//

#ifndef room_hpp
#define room_hpp

#include "qnetworkserver.hpp"

#include <map>
#include <vector>

#undef __LOGTAG__
#define __LOGTAG__ "room"

struct player {
    player(conn_io* qcon) : qconnection(qcon) {
    }
    ~player() {
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "player destructor");
    }
    conn_io* qconnection;
};

class room;
class roomserver_interface {
public:
    virtual void onroom_pre_start(room* r) = 0;
    virtual struct ev_loop * get_netowrk_main_loop() = 0;
};

class room_interface {
    virtual void onroom_create()= 0;
    virtual void onroom_start() = 0;
    virtual void onroom_player_added(player* p) = 0;
    virtual void onroom_message(player* p, const qstring& msg) = 0;
    virtual void onroom_player_removed(player* p) = 0;
    virtual void onroom_end() = 0;
};

struct roomconfig {
    roomconfig(const roomconfig& room_config) :
    min_players(room_config.min_players),
    max_players(room_config.max_players),
    allow_join_after_start(room_config.allow_join_after_start) {
    }
    roomconfig(int min_players, int max_players, bool allow_join_after_start) :
        min_players(min_players),
        max_players(max_players),
        allow_join_after_start(allow_join_after_start) {
    }
    const int min_players = 1;
    const int max_players = 1;
    const bool allow_join_after_start = false;
};

class room : public room_interface {
private:
    room() : room_config(roomconfig(1, 1, false)), creation_time(0) {
    }
    
public:
    enum states {
        room_uninitialised,
        room_waiting,
        room_start,
        room_end
    };
    room(roomserver_interface*, const roomconfig& room_config);
    virtual ~room();
    
    ssize_t try_add_connection(conn_io* qconnection);
    ssize_t remove_connection(conn_io* qconnection);
    player* get_player(conn_io* qconnection);
    inline bool is_min_capacity_reached() { return playermap.size()>=room_config.min_players; }
    inline bool is_max_capacity_reached() { return playermap.size()>=room_config.max_players; }
    inline ssize_t get_playermap_count() { return playermap.size(); }
    states get_state() { return state; }
    const qstring& get_state_string();
    bool is_state(states state) { return this->state==state; }
    void kick_all_except(conn_io* qconnection);
    void print_info();
    ev_tstamp since_creation();
    
    std::map<conn_io*, player*> playermap;
    const int room_id = 0;
    const ev_tstamp creation_time;
    
    void pass_message_to_room(player* p, const qstring& msg);
    
    void broadcast(const qstring& msg);
    void broadcast_except(player* p, const qstring& msg);
    void sendto(player* p, const qstring& msg);
    
protected:
    void onroom_create() override;
    void onroom_start() override;
    void onroom_player_added(player* p) override;
    void onroom_message(player* p, const qstring& msg) override;
    void onroom_player_removed(player* p) override;
    void onroom_end() override;
    
private:
    void set_state(states state);
    void on_state_change(states prev_state);
    
    const roomconfig room_config;
    states state = room_uninitialised;
    roomserver_interface* roomserverinterface;
    static int room_id_counter;
    qstring states_string[4] = {"uninitialised", "waiting", "start", "end" };
};

#endif /* room_hpp */
