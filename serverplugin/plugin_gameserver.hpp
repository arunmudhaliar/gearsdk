//
//  Copyright 2024 homenet25
//  plugin_gameserver.hpp
//  NetworkServer
//
//  Created by Arun A on 15/10/23.
//

#ifndef plugin_gameserver_hpp
#define plugin_gameserver_hpp

#include "../qserver/source/roomserver.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "plugin_gameserver"
namespace gsdk {
namespace server {

class qplugin_qserver_event_listener : public observer_qserver_events {
public:
    ~qplugin_qserver_event_listener() {}
    typedef void (*type_on_qserver_pre_start)(qnetworkserver* server);
    typedef void (*type_on_qserver_start)(qnetworkserver* server, const char* ip, uint16_t port);
    typedef void (*type_on_qserver_stop)(qnetworkserver* server);
    typedef void (*type_on_qserver_error)(qnetworkserver* server, int error_code);
    
    // room event cb
    typedef void (*type_room_event_create)(void* server, int room);
    typedef void (*type_room_event_start)(void* server, int room);
    typedef void (*type_room_event_player_added)(void* server, int room, const char* pid, unsigned cid_hash);
    typedef void (*type_room_event_message)(void* server, int room, const char* pid, unsigned cid_hash, const char* msg);
    typedef void (*type_room_event_player_removed)(void* server, int room, const char* pid, unsigned cid_hash);
    typedef void (*type_room_event_end)(void* server, int room);
    typedef void (*type_room_event_countdown_to_start)(void* server, int room, int count, int max_count);
    typedef void (*type_room_event_countdown_cancelled)(void* server, int room);
    
    qplugin_qserver_event_listener(type_on_qserver_pre_start pre_start_cb, type_on_qserver_start start_cb, type_on_qserver_stop stop_cb, type_on_qserver_error error_cb,
                                   type_room_event_create room_event_create_cb,
                                   type_room_event_start room_event_start_cb,
                                   type_room_event_player_added room_event_player_added_cb,
                                   type_room_event_message room_event_message_cb,
                                   type_room_event_player_removed room_player_removed_cb,
                                   type_room_event_end room_event_end_cb,
                                   type_room_event_countdown_to_start room_event_countdown_to_start_cb,
                                   type_room_event_countdown_cancelled room_event_countdown_cancelled_cb)
    : cb_on_server_pre_start(pre_start_cb), cb_on_server_start(start_cb), cb_on_server_stop(stop_cb), cb_on_server_error(error_cb),
    cb_room_event_create(room_event_create_cb), cb_room_event_start(room_event_start_cb), cb_room_event_player_added(room_event_player_added_cb), cb_room_event_message(room_event_message_cb), cb_room_player_removed(room_player_removed_cb), cb_room_event_end(room_event_end_cb), cb_room_event_countdown_to_start(room_event_countdown_to_start_cb), cb_room_event_countdown_cancelled(room_event_countdown_cancelled_cb) {}

protected:
    void on_server_pre_start(qnetworkserver*) override final;
    void on_server_start(qnetworkserver*, const char* ip, uint16_t port) override final;
    void on_server_stop(qnetworkserver*) override final;
    void on_server_error(qnetworkserver*, int error_code) override final;

    void room_event_create(void* server, int room) override final;
    void room_event_start(void* server, int room) override final;
    void room_event_player_added(void* server, int room, const qstring& pid, unsigned cid_hash) override final;
    void room_event_message(void* server, int room, const qstring& pid, unsigned cid_hash, const qstring& msg) override final;
    void room_event_player_removed(void* server, int room, const qstring& pid, unsigned cid_hash) override final;
    void room_event_end(void* server, int room) override final;
    void room_event_countdown_to_start(void* server, int room, int count, int max_count) override final;
    void room_event_countdown_cancelled(void* server, int room) override final;
    
private:
    type_on_qserver_pre_start cb_on_server_pre_start = nullptr;
    type_on_qserver_start cb_on_server_start = nullptr;
    type_on_qserver_stop cb_on_server_stop = nullptr;
    type_on_qserver_error cb_on_server_error = nullptr;
    
    type_room_event_create cb_room_event_create = nullptr;
    type_room_event_start cb_room_event_start = nullptr;
    type_room_event_player_added cb_room_event_player_added = nullptr;
    type_room_event_message cb_room_event_message = nullptr;
    type_room_event_player_removed cb_room_player_removed = nullptr;
    type_room_event_end cb_room_event_end = nullptr;
    type_room_event_countdown_to_start cb_room_event_countdown_to_start = nullptr;
    type_room_event_countdown_cancelled cb_room_event_countdown_cancelled = nullptr;
};

// MARK: -
class plugin_game_room : public room {
public:
    plugin_game_room(roomserver_interface*, const roomconfig& room_config);
    virtual ~plugin_game_room();
    
protected:
    void onroom_create() override;
    void onroom_start() override;
    void onroom_player_added(player* p) override;
    void onroom_message(player* p, const qstring& msg) override;
    void onroom_player_removed(player* p) override;
    void onroom_end() override;
    bool can_allow_reconnection(unsigned cid_hash) override;
};

// MARK: -
class plugin_gameserver : public roomserver {
public:
    plugin_gameserver(const qstring& zk_uri);
    virtual ~plugin_gameserver();
    
protected:
    void on_network_server_init() override;
    room* create_room(const msg_room_config* room_config_msg) override;
};

extern "C" {
EXPORT int spawn_qserver(const char* server_address, const char* redis_address, const char* zk_uri, const char* root_dir, const char* app_id, qplugin_qserver_event_listener::type_on_qserver_pre_start pre_start_cb, qplugin_qserver_event_listener::type_on_qserver_start start_cb, qplugin_qserver_event_listener::type_on_qserver_stop stop_cb,
                         qplugin_qserver_event_listener::type_on_qserver_error error_cb,
                         qplugin_qserver_event_listener::type_room_event_create room_event_create_cb,
                         qplugin_qserver_event_listener::type_room_event_start room_event_start_cb,
                         qplugin_qserver_event_listener::type_room_event_player_added room_event_player_added_cb,
                         qplugin_qserver_event_listener::type_room_event_message room_event_message_cb,
                         qplugin_qserver_event_listener::type_room_event_player_removed room_player_removed_cb,
                         qplugin_qserver_event_listener::type_room_event_end room_event_end_cb,
                         qplugin_qserver_event_listener::type_room_event_countdown_to_start room_event_countdown_to_start_cb,
                         qplugin_qserver_event_listener::type_room_event_countdown_cancelled room_event_countdown_cancelled_cb);
void* spawn_qserver_internal(void* data);
EXPORT uint64_t qserver_logfile(qnetworkserver*, qlogfile::log_lvls lvl, qcustomlogger::elog_type type, const char* tag, const char* pid, const char* roomid, const char* message);
EXPORT size_t qserver_stats_count(qnetworkserver* server, const char* counter, long count_val, const char* session, const char* pid, const char* version = "", const char* epic = "", const char* myth = "", const char* legend = "",
                    const char* story = "", const char* message = "");
}
}}
#endif /* plugin_gameserver_hpp */
