//
//  roomserver.hpp
//  NetworkServer
//
//  Created by Arun A on 26/10/23.
//

#ifndef roomserver_hpp
#define roomserver_hpp

#include "QNetworkServer.hpp"

#include <map>
#include <vector>

#undef __LOGTAG__
#define __LOGTAG__ "roomserver"

class interfacegameserver {
    
};

struct player {
    player(QPeerConnection* qcon) : qconnection(qcon) {
    }
    ~player() {
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "Player destructor");
    }
    QPeerConnection* qconnection;
};

class room;
class interfaceroom {
public:
    virtual void onroom_create(room*)= 0;
    virtual void onroom_pre_start(room*) = 0;
    virtual void onroom_start(room*) = 0;
    virtual void onplayer_added(room*, player*) = 0;
    virtual void onplayer_removed(room*, player*) = 0;
    virtual void onroom_end(room*) = 0;
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

class room {
private:
    room() : room_config(roomconfig(1, 1, false)){}
    
public:
    enum states {
        room_uninitialised,
        room_waiting,
        room_start,
        room_end
    };
    room(interfaceroom*, const roomconfig& room_config);
    ~room();
    
    ssize_t try_add_connection(QPeerConnection* qconnection);
    ssize_t remove_connection(QPeerConnection* qconnection);
    inline bool is_min_capacity_reached() { return playermap.size()>=room_config.min_players; }
    inline bool is_max_capacity_reached() { return playermap.size()>=room_config.max_players; }
    inline ssize_t get_playermap_count() { return playermap.size(); }
    states get_state() { return state; }
    bool is_state(states state) { return this->state==state; }
    void kick_all_except(QPeerConnection* qconnection);
    
    std::map<QPeerConnection*, player*> playermap;
    const int room_index = 0;
    
private:
    void set_state(states state);
    void on_state_change(states prev_state);
    
    const roomconfig room_config;
    states state = room_uninitialised;
    interfaceroom* roominterface;
    static int roomID;
};

class roomserver : public QNetworkServer, public interfaceroom {
protected:
    void OnMessage(ssize_t recv_len, uint8_t* buf, QPeerConnection* qconnection) override;
    void OnConnection(QPeerConnection* qconnection) override;
    void OnDestroyConnection(QPeerConnection* qconnection) override;
    
    void onroom_create(room*) override;
    void onroom_pre_start(room*) override final;
    void onroom_start(room*) override;
    void onplayer_added(room*, player*) override;
    void onplayer_removed(room*, player*) override;
    void onroom_end(room*) override;
    
    room* create_waiting_room();
    
    std::vector<room*> waiting_rooms;
    std::vector<room*> rooms;
    std::map<unsigned, room*> connection_map;
};

#endif /* roomserver_hpp */
