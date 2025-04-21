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
#include "serverplugin_helper.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "plugin_gameserver"
namespace gsdk {
namespace server {

class plugin_gameserver_event_listener : public observer_qserver_events {
public:
    ~plugin_gameserver_event_listener() {}
    typedef void (*type_plugin_gameserver_on_pre_start)(qnetworkserver* server);
    typedef void (*type_plugin_gameserver_on_start)(qnetworkserver* server, const char* ip, uint16_t port);
    typedef void (*type_plugin_gameserver_on_stop)(qnetworkserver* server);
    typedef void (*type_plugin_gameserver_on_error)(qnetworkserver* server, int error_code);
    
    // room event cb
    typedef void (*type_plugin_gameserver_on_room_event_create)(qnetworkserver* server, int room, class room* room_ptr);
    typedef void (*type_plugin_gameserver_on_room_event_start)(qnetworkserver* server, int room, class room* room_ptr);
    typedef void (*type_plugin_gameserver_on_room_event_player_added)(qnetworkserver* server, int room, class room* room_ptr, const char* pid, unsigned cid_hash);
    typedef void (*type_plugin_gameserver_on_room_event_message)(qnetworkserver* server, int room, class room* room_ptr, const char* pid, unsigned cid_hash, const char* msg);
    typedef void (*type_plugin_gameserver_on_room_event_player_removed)(qnetworkserver* server, int room, class room* room_ptr, const char* pid, unsigned cid_hash);
    typedef void (*type_plugin_gameserver_on_room_event_end)(qnetworkserver* server, int room, class room* room_ptr);
    typedef void (*type_plugin_gameserver_on_room_event_destroy)(qnetworkserver* server, int room, class room* room_ptr);
    typedef void (*type_plugin_gameserver_on_room_event_countdown_to_start)(qnetworkserver* server, int room, class room* room_ptr, int count, int max_count);
    typedef void (*type_plugin_gameserver_on_room_event_countdown_cancelled)(qnetworkserver* server, int room, class room* room_ptr);
    
    plugin_gameserver_event_listener(type_plugin_gameserver_on_pre_start pre_start_cb, type_plugin_gameserver_on_start start_cb, type_plugin_gameserver_on_stop stop_cb, type_plugin_gameserver_on_error error_cb,
                                   type_plugin_gameserver_on_room_event_create room_event_create_cb,
                                   type_plugin_gameserver_on_room_event_start room_event_start_cb,
                                   type_plugin_gameserver_on_room_event_player_added room_event_player_added_cb,
                                   type_plugin_gameserver_on_room_event_message room_event_message_cb,
                                   type_plugin_gameserver_on_room_event_player_removed room_player_removed_cb,
                                   type_plugin_gameserver_on_room_event_end room_event_end_cb,
                                     type_plugin_gameserver_on_room_event_destroy room_event_destroy_cb,
                                   type_plugin_gameserver_on_room_event_countdown_to_start room_event_countdown_to_start_cb,
                                   type_plugin_gameserver_on_room_event_countdown_cancelled room_event_countdown_cancelled_cb)
    : cb_on_server_pre_start(pre_start_cb), cb_on_server_start(start_cb), cb_on_server_stop(stop_cb), cb_on_server_error(error_cb),
    cb_room_event_create(room_event_create_cb), cb_room_event_start(room_event_start_cb), cb_room_event_player_added(room_event_player_added_cb), cb_room_event_message(room_event_message_cb), cb_room_player_removed(room_player_removed_cb), cb_room_event_end(room_event_end_cb), cb_room_event_destroy(room_event_destroy_cb), cb_room_event_countdown_to_start(room_event_countdown_to_start_cb), cb_room_event_countdown_cancelled(room_event_countdown_cancelled_cb) {}

protected:
    void on_server_pre_start(qnetworkserver* server) override final;
    void on_server_start(qnetworkserver* server, const char* ip, uint16_t port) override final;
    void on_server_stop(qnetworkserver* server) override final;
    void on_server_error(qnetworkserver* server, int error_code) override final;

    void room_event_create(qnetworkserver* server, int room, class room* room_ptr) override final;
    void room_event_start(qnetworkserver* server, int room, class room* room_ptr) override final;
    void room_event_player_added(qnetworkserver* server, int room, class room* room_ptr, const qstring& pid, unsigned cid_hash) override final;
    void room_event_message(qnetworkserver* server, int room, class room* room_ptr, const qstring& pid, unsigned cid_hash, const qstring& msg) override final;
    void room_event_player_removed(qnetworkserver* server, int room, class room* room_ptr, const qstring& pid, unsigned cid_hash) override final;
    void room_event_end(qnetworkserver* server, int room, class room* room_ptr) override final;
    void room_event_destroy(qnetworkserver* server, int room, class room* room_ptr) override final;
    void room_event_countdown_to_start(qnetworkserver* server, int room, class room* room_ptr, int count, int max_count) override final;
    void room_event_countdown_cancelled(qnetworkserver* server, int room, class room* room_ptr) override final;
    
private:
    type_plugin_gameserver_on_pre_start cb_on_server_pre_start = nullptr;
    type_plugin_gameserver_on_start cb_on_server_start = nullptr;
    type_plugin_gameserver_on_stop cb_on_server_stop = nullptr;
    type_plugin_gameserver_on_error cb_on_server_error = nullptr;
    
    type_plugin_gameserver_on_room_event_create cb_room_event_create = nullptr;
    type_plugin_gameserver_on_room_event_start cb_room_event_start = nullptr;
    type_plugin_gameserver_on_room_event_player_added cb_room_event_player_added = nullptr;
    type_plugin_gameserver_on_room_event_message cb_room_event_message = nullptr;
    type_plugin_gameserver_on_room_event_player_removed cb_room_player_removed = nullptr;
    type_plugin_gameserver_on_room_event_end cb_room_event_end = nullptr;
    type_plugin_gameserver_on_room_event_end cb_room_event_destroy = nullptr;
    type_plugin_gameserver_on_room_event_countdown_to_start cb_room_event_countdown_to_start = nullptr;
    type_plugin_gameserver_on_room_event_countdown_cancelled cb_room_event_countdown_cancelled = nullptr;
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
    void onroom_destroy() override;
    bool can_allow_reconnection(unsigned cid_hash) override;
};

// MARK: -
struct st_stateful_gameserver_response_packet {
    st_stateful_gameserver_response_packet(short type, qnetworkserver* server, class room* room_ptr, unsigned cid_hash, const char* msg, size_t len) {
        this->type = type;
        this->server = server;
        this->room_ptr = room_ptr;
        this->cid_hash = cid_hash;
        this->message.bin_copy((const uint8_t*)msg, len);
    }
    short type = -1;
    qnetworkserver* server = nullptr;
    class room* room_ptr = nullptr;
    unsigned cid_hash = 0;
    qstring message;
};

// MARK: -
class plugin_gameserver : public roomserver {
public:
    plugin_gameserver();
    virtual ~plugin_gameserver();
    
    async_ev_notifier<struct st_stateful_gameserver_response_packet>& get_ev_notifier()   { return ev_notifier; }
    
protected:
    void on_network_server_init() override;
    room* create_room(const msg_room_config* room_config_msg) override;
    
    async_ev_notifier<struct st_stateful_gameserver_response_packet> ev_notifier;
    static void notify_server_async_cb(EV_P_ ev_async *w, int revents);
};

extern "C" {
EXPORT int spawn_game_server(const char* server_address, const char* redis_address, const char* zk_uri, const char* root_dir, const char* inf_file, const char* app_id, plugin_gameserver_event_listener::type_plugin_gameserver_on_pre_start pre_start_cb, plugin_gameserver_event_listener::type_plugin_gameserver_on_start start_cb, plugin_gameserver_event_listener::type_plugin_gameserver_on_stop stop_cb,
                         plugin_gameserver_event_listener::type_plugin_gameserver_on_error error_cb,
                         plugin_gameserver_event_listener::type_plugin_gameserver_on_room_event_create room_event_create_cb,
                         plugin_gameserver_event_listener::type_plugin_gameserver_on_room_event_start room_event_start_cb,
                         plugin_gameserver_event_listener::type_plugin_gameserver_on_room_event_player_added room_event_player_added_cb,
                         plugin_gameserver_event_listener::type_plugin_gameserver_on_room_event_message room_event_message_cb,
                         plugin_gameserver_event_listener::type_plugin_gameserver_on_room_event_player_removed room_player_removed_cb,
                         plugin_gameserver_event_listener::type_plugin_gameserver_on_room_event_end room_event_end_cb,
    plugin_gameserver_event_listener::type_plugin_gameserver_on_room_event_destroy room_event_destroy_cb,
                         plugin_gameserver_event_listener::type_plugin_gameserver_on_room_event_countdown_to_start room_event_countdown_to_start_cb,
                         plugin_gameserver_event_listener::type_plugin_gameserver_on_room_event_countdown_cancelled room_event_countdown_cancelled_cb,
                         void* user_arg = nullptr
                         );
void* spawn_game_server_internal(void* data);
EXPORT uint64_t qserver_logfile(qnetworkserver*, qlogfile::log_lvls lvl, qcustomlogger::elog_type type, const char* tag, const char* pid, const char* roomid, const char* message);
EXPORT size_t qserver_stats_count(qnetworkserver* server, const char* counter, long count_val, const char* session, const char* pid, const char* version = "", const char* epic = "", const char* myth = "", const char* legend = "",
                    const char* story = "", const char* message = "");
EXPORT bool room_broadcast_except(qnetworkserver* server, class room* room_ptr, unsigned cid_hash, const char* msg, unsigned length);
EXPORT void room_broadcast(qnetworkserver* server, class room* room_ptr, const char* msg, unsigned length);
EXPORT bool room_send_to(qnetworkserver* server, class room* room_ptr, unsigned cid_hash, const char* msg, unsigned length);
}
}}
#endif /* plugin_gameserver_hpp */
