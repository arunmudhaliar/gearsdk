//
//  Copyright 2024 homenet25
//  plugin_gameserver.cpp
//  NetworkServer
//
//  Created by Arun A on 15/10/23.
//

#include "plugin_gameserver.hpp"

using namespace gsdk::server;

// MARK: - qplugin_qserver_event_listener
void plugin_gameserver_event_listener::on_server_pre_start(qnetworkserver* server) {
    if (cb_on_server_pre_start) {
        cb_on_server_pre_start(server);
    }
}

void plugin_gameserver_event_listener::on_server_start(qnetworkserver* server, const char* ip, uint16_t port) {
    if (cb_on_server_start) {
        cb_on_server_start(server, ip, port);
    }
}

void plugin_gameserver_event_listener::on_server_stop(qnetworkserver* server) {
    if (cb_on_server_stop) {
        cb_on_server_stop(server);
    }
}

void plugin_gameserver_event_listener::on_server_error(qnetworkserver* server, int error_code) {
    if (cb_on_server_error) {
        cb_on_server_error(server, error_code);
    }
}

void plugin_gameserver_event_listener::room_event_create(qnetworkserver* server, int room, class room* room_ptr) {
    if (cb_room_event_create) {
        cb_room_event_create(server, room, room_ptr);
    }
}
void plugin_gameserver_event_listener::room_event_start(qnetworkserver* server, int room, class room* room_ptr) {
    if (cb_room_event_start) {
        cb_room_event_start(server, room, room_ptr);
    }
}
void plugin_gameserver_event_listener::room_event_player_added(qnetworkserver* server, int room, class room* room_ptr, const qstring& pid, unsigned cid_hash) {
    if (cb_room_event_player_added) {
        cb_room_event_player_added(server, room, room_ptr, pid.c_str(), cid_hash);
    }
}
void plugin_gameserver_event_listener::room_event_message(qnetworkserver* server, int room, class room* room_ptr, const qstring& pid, unsigned cid_hash, const qstring& msg) {
    if (cb_room_event_message) {
        cb_room_event_message(server, room, room_ptr, pid.c_str(), cid_hash, msg.length(), (const uint8_t*)msg.c_str());
    }
}
void plugin_gameserver_event_listener::room_event_player_removed(qnetworkserver* server, int room, class room* room_ptr, const qstring& pid, unsigned cid_hash) {
    if (cb_room_player_removed) {
        cb_room_player_removed(server, room, room_ptr, pid.c_str(), cid_hash);
    }
}
void plugin_gameserver_event_listener::room_event_end(qnetworkserver* server, int room, class room* room_ptr) {
    if (cb_room_event_end) {
        cb_room_event_end(server, room, room_ptr);
    }
}
void plugin_gameserver_event_listener::room_event_destroy(qnetworkserver* server, int room, class room* room_ptr) {
    if (cb_room_event_destroy) {
        cb_room_event_destroy(server, room, room_ptr);
    }
}
void plugin_gameserver_event_listener::room_event_countdown_to_start(qnetworkserver* server, int room, class room* room_ptr, int count, int max_count) {
    if (cb_room_event_countdown_to_start) {
        cb_room_event_countdown_to_start(server, room, room_ptr, count, max_count);
    }
}
void plugin_gameserver_event_listener::room_event_countdown_cancelled(qnetworkserver* server, int room, class room* room_ptr) {
    if (cb_room_event_countdown_cancelled) {
        cb_room_event_countdown_cancelled(server, room, room_ptr);
    }
}

EXPORT bool gsdk::server::room_broadcast_except(qnetworkserver* server, class room* room_ptr, unsigned cid_hash, const char* msg, unsigned length) {
    if (msg == nullptr || length == 0) {
        debug_warn(LOG_LEVEL_0, __LOGTAG__, "msg is empty or null. Ignoring room_broadcast_except");
        return false;
    }
    plugin_gameserver* pserver = static_cast<plugin_gameserver*>(server);
    room_ptr = pserver->try_get_room_if_in_map(room_ptr);
    if (room_ptr == nullptr) {
        debug_warn(LOG_LEVEL_0, __LOGTAG__, "room not present on map. Ignoring room_broadcast_except for message %.*s", length, msg);
        return false;
    }
    
    struct st_stateful_gameserver_response_packet* response_packet = DEBUG_NEW st_stateful_gameserver_response_packet(0, server, room_ptr, cid_hash, msg, length);
    pserver->get_ev_notifier().enqueue_response(response_packet);
    pserver->get_ev_notifier().notify_main_thread();
    return true;
}

EXPORT void gsdk::server::room_broadcast(qnetworkserver* server, class room* room_ptr, const char* msg, unsigned length) {
    if (msg == nullptr || length == 0) {
        debug_warn(LOG_LEVEL_0, __LOGTAG__, "msg is empty or null. Ignoring room_broadcast");
        return;
    }
    plugin_gameserver* pserver = static_cast<plugin_gameserver*>(server);
    room_ptr = pserver->try_get_room_if_in_map(room_ptr);
    if (room_ptr == nullptr) {
        debug_warn(LOG_LEVEL_0, __LOGTAG__, "room not present on map. Ignoring room_broadcast for message %.*s", length, msg);
        return;
    }
    
    struct st_stateful_gameserver_response_packet* response_packet = DEBUG_NEW st_stateful_gameserver_response_packet(1, server, room_ptr, 0, msg, length);
    pserver->get_ev_notifier().enqueue_response(response_packet);
    pserver->get_ev_notifier().notify_main_thread();
}

EXPORT bool gsdk::server::room_send_to(qnetworkserver* server, class room* room_ptr, unsigned cid_hash, const char* msg, unsigned length) {
    if (msg == nullptr || length == 0) {
        debug_warn(LOG_LEVEL_0, __LOGTAG__, "msg is empty or null. Ignoring room_send_to");
        return false;
    }
    plugin_gameserver* pserver = static_cast<plugin_gameserver*>(server);
    room_ptr = pserver->try_get_room_if_in_map(room_ptr);
    if (room_ptr == nullptr) {
        debug_warn(LOG_LEVEL_0, __LOGTAG__, "room not present on map. Ignoring room_send_to for message %.*s", length, msg);
        return false;
    }
    
    struct st_stateful_gameserver_response_packet* response_packet = DEBUG_NEW st_stateful_gameserver_response_packet(2, server, room_ptr, cid_hash, msg, length);
    pserver->get_ev_notifier().enqueue_response(response_packet);
    pserver->get_ev_notifier().notify_main_thread();
    return true;
}
// MARK: - plugin_game_room
plugin_game_room::plugin_game_room(roomserver_interface* interface, const roomconfig& room_config) : room(interface, room_config) {}

plugin_game_room::~plugin_game_room() {}

void plugin_game_room::onroom_create() {
    room::onroom_create();
}
void plugin_game_room::onroom_start() {
    room::onroom_start();
}
void plugin_game_room::onroom_player_added(player* p) {
    room::onroom_player_added(p);
}
void plugin_game_room::onroom_message(player* p, const qstring& msg) {
    room::onroom_message(p, msg);
//    debug_print(LOG_LEVEL_3, __LOGTAG__, "room %d: received '%.*s' from player %0x", ROOM_ID, msg.length(), msg.c_str(), p->qconnection->cid_hash_val);
}
void plugin_game_room::onroom_player_removed(player* p) {
    room::onroom_player_removed(p);
}
void plugin_game_room::onroom_end() {
    room::onroom_end();
}
void plugin_game_room::onroom_destroy() {
    room::onroom_destroy();
}
bool plugin_game_room::can_allow_reconnection(unsigned cid_hash) {
    UNUSED(cid_hash);
    return true;
}

// MARK: - plugin_gameserver
//----------------------------------------------------------------------------
//---------------------------------plugin_gameserver--------------------------
//----------------------------------------------------------------------------
plugin_gameserver::plugin_gameserver() : roomserver() {}

plugin_gameserver::~plugin_gameserver() {
    debug_print(LOG_LEVEL_0, __LOGTAG__, "plugin_gameserver destructor called");
}

void plugin_gameserver::on_network_server_init() {
    roomserver::on_network_server_init();
    ev_notifier.init(get_netowrk_main_loop(), notify_server_async_cb, this);
    debug_print_important2(__LOGTAG__, "plugin_gameserver::init");
}

room* plugin_gameserver::create_room(const msg_room_config* room_config_msg) {
    return DEBUG_NEW plugin_game_room(this, roomconfig(room_config_msg));
}

void plugin_gameserver::notify_server_async_cb(EV_P_ ev_async *w, int revents) {
    UNUSED(loop);
    UNUSED(revents);
    plugin_gameserver* pserver = static_cast<plugin_gameserver*>(w->data);
    while (st_stateful_gameserver_response_packet* response_packet = pserver->get_ev_notifier().dequeue_response()) {
        room* room_ptr = pserver->try_get_room_if_in_map(response_packet->room_ptr);
        if (!room_ptr) {
            GX_DELETE(response_packet);
            continue;
        }
        switch (response_packet->type) {
            case 0: {
                player* p = room_ptr->get_player(response_packet->cid_hash);
                if (p) {
                    room_ptr->broadcast_except(p, response_packet->message);
                }
            }
                break;
            case 1: {
                room_ptr->broadcast(response_packet->message);
            }
                break;
            case 2: {
                player* p = room_ptr->get_player(response_packet->cid_hash);
                if (p) {
                    room_ptr->sendto(p, response_packet->message);
                }
            }
                break;
        }
        GX_DELETE(response_packet);
    }
}

EXPORT int gsdk::server::spawn_game_server(const char* server_address, const char* redis_address, const char* zk_uri, const char* root_dir, const char* inf_file, const char* app_id, plugin_gameserver_event_listener::type_plugin_gameserver_on_pre_start pre_start_cb, plugin_gameserver_event_listener::type_plugin_gameserver_on_start start_cb, plugin_gameserver_event_listener::type_plugin_gameserver_on_stop stop_cb,
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
                                       void* user_arg
                                       ) {
    qaddress server_addr(server_address);
    qaddress redis_addr(redis_address);
    struct st_qserver_config_in* config = new struct st_qserver_config_in();
    config->host = server_addr.ip;
    config->port = qstring::format_string("%d", server_addr.port);
    config->redis_ip = redis_addr.ip;
    config->redis_port = redis_addr.port;
    config->zk_uri = zk_uri;
    config->root_dir = fs::path(root_dir);
    config->inf_file = fs::path(inf_file);
    config->app_id = app_id;
    config->user_arg = user_arg;
    config->print();
    plugin_gameserver_event_listener* listener = DEBUG_NEW plugin_gameserver_event_listener(
                                                                                        pre_start_cb, start_cb, stop_cb, error_cb,
                                                                                        room_event_create_cb,
                                                                                        room_event_start_cb,
                                                                                        room_event_player_added_cb,
                                                                                        room_event_message_cb,
                                                                                        room_player_removed_cb,
                                                                                        room_event_end_cb,
                                                                                        room_event_destroy_cb,
                                                                                        room_event_countdown_to_start_cb,
                                                                                        room_event_countdown_cancelled_cb);
    std::tuple<st_qserver_config_in*, plugin_gameserver_event_listener*>* tuple_in = DEBUG_NEW std::tuple<st_qserver_config_in*, plugin_gameserver_event_listener*>(config, listener);
    if (pthread_create(&config->run_thread_id, nullptr, gsdk::server::spawn_game_server_internal, (void*)tuple_in) < 0) {
        debug_print_error(__LOGTAG__, "spawn_qserver - could not create thread: %s - %d", strerror(errno), errno);
        GX_DELETE(tuple_in);
        GX_DELETE(config);
        GX_DELETE(listener);
        return 1;
    }
    return 0;
}

void* gsdk::server::spawn_game_server_internal(void* data) {
    std::tuple<st_qserver_config_in*, plugin_gameserver_event_listener*>* tuple_in = (std::tuple<st_qserver_config_in*, plugin_gameserver_event_listener*>*) data;
    st_qserver_config_in* config = (st_qserver_config_in*)std::get<0>(*tuple_in);
    plugin_gameserver_event_listener* listener = (plugin_gameserver_event_listener*)std::get<1>(*tuple_in);
    plugin_gameserver server;
    int result = server.run(*config, listener, config->user_arg);
    GX_DELETE(tuple_in);
    GX_DELETE(config);
    GX_DELETE(listener);
    debug_print(LOG_LEVEL_0, __LOGTAG__, "exiting spawn_qserver_internal, result %d", result);
    return nullptr;
}

EXPORT uint64_t gsdk::server::qserver_logfile(qnetworkserver* server, qlogfile::log_lvls lvl, qcustomlogger::elog_type type, const char* tag, const char* pid, const char* roomid, const char* message) {
    return server->get_file_logger()->log(lvl, type, tag, pid, roomid, message);
}

EXPORT size_t gsdk::server::qserver_stats_count(qnetworkserver* server, const char* counter, long count_val, const char* session, const char* pid, const char* version, const char* epic, const char* myth, const char* legend,
                           const char* story, const char* message) {
    return server->get_stats_loggeer()->server_count(counter, count_val, session, pid, version, epic, myth, legend, story, message);
}
