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
void qplugin_qserver_event_listener::on_server_pre_start(qnetworkserver* server) {
    if (cb_on_server_pre_start) {
        cb_on_server_pre_start(server);
    }
}

void qplugin_qserver_event_listener::on_server_start(qnetworkserver* server, const char* ip, uint16_t port) {
    if (cb_on_server_start) {
        cb_on_server_start(server, ip, port);
    }
}

void qplugin_qserver_event_listener::on_server_stop(qnetworkserver* server) {
    if (cb_on_server_stop) {
        cb_on_server_stop(server);
    }
}

void qplugin_qserver_event_listener::on_server_error(qnetworkserver* server, int error_code) {
    if (cb_on_server_error) {
        cb_on_server_error(server, error_code);
    }
}

void qplugin_qserver_event_listener::room_event_create(void* server, int room) {
    if (cb_room_event_create) {
        cb_room_event_create(server, room);
    }
}
void qplugin_qserver_event_listener::room_event_start(void* server, int room) {
    if (cb_room_event_start) {
        cb_room_event_start(server, room);
    }
}
void qplugin_qserver_event_listener::room_event_player_added(void* server, int room, const qstring& pid, unsigned cid_hash) {
    if (cb_room_event_player_added) {
        cb_room_event_player_added(server, room, pid.c_str(), cid_hash);
    }
}
void qplugin_qserver_event_listener::room_event_message(void* server, int room, const qstring& pid, unsigned cid_hash, const qstring& msg) {
    if (cb_room_event_message) {
        cb_room_event_message(server, room, pid.c_str(), cid_hash, msg.c_str());
    }
}
void qplugin_qserver_event_listener::room_event_player_removed(void* server, int room, const qstring& pid, unsigned cid_hash) {
    if (cb_room_player_removed) {
        cb_room_player_removed(server, room, pid.c_str(), cid_hash);
    }
}
void qplugin_qserver_event_listener::room_event_end(void* server, int room) {
    if (cb_room_event_end) {
        cb_room_event_end(server, room);
    }
}
void qplugin_qserver_event_listener::room_event_countdown_to_start(void* server, int room, int count, int max_count) {
    if (cb_room_event_countdown_to_start) {
        cb_room_event_countdown_to_start(server, room, count, max_count);
    }
}
void qplugin_qserver_event_listener::room_event_countdown_cancelled(void* server, int room) {
    if (cb_room_event_countdown_cancelled) {
        cb_room_event_countdown_cancelled(server, room);
    }
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
    debug_print(LOG_LEVEL_3, __LOGTAG__, "room %d: received '%.*s' from player %0x", ROOM_ID, msg.length(), msg.c_str(), p->qconnection->cid_hash_val);
    // passing to other clients
    broadcast_except(p, msg);
}
void plugin_game_room::onroom_player_removed(player* p) {
    room::onroom_player_removed(p);
}
void plugin_game_room::onroom_end() {
    room::onroom_end();
}
bool plugin_game_room::can_allow_reconnection(unsigned cid_hash) {
    return true;
}

// MARK: - plugin_gameserver
//----------------------------------------------------------------------------
//---------------------------------plugin_gameserver--------------------------
//----------------------------------------------------------------------------
plugin_gameserver::plugin_gameserver(const qstring& zk_uri) : roomserver(zk_uri) {}

plugin_gameserver::~plugin_gameserver() {}

void plugin_gameserver::on_network_server_init() {
    roomserver::on_network_server_init();
    debug_print_important2(__LOGTAG__, "plugin_gameserver::init");
}

room* plugin_gameserver::create_room(const msg_room_config* room_config_msg) {
    return DEBUG_NEW plugin_game_room(this, roomconfig(room_config_msg));
}

EXPORT int gsdk::server::spawn_qserver(const char* server_address, const char* redis_address, const char* zk_uri, const char* root_dir, const char* app_id, qplugin_qserver_event_listener::type_on_qserver_pre_start pre_start_cb, qplugin_qserver_event_listener::type_on_qserver_start start_cb, qplugin_qserver_event_listener::type_on_qserver_stop stop_cb,
    qplugin_qserver_event_listener::type_on_qserver_error error_cb,
                                       qplugin_qserver_event_listener::type_room_event_create room_event_create_cb,
                                       qplugin_qserver_event_listener::type_room_event_start room_event_start_cb,
                                       qplugin_qserver_event_listener::type_room_event_player_added room_event_player_added_cb,
                                       qplugin_qserver_event_listener::type_room_event_message room_event_message_cb,
                                       qplugin_qserver_event_listener::type_room_event_player_removed room_player_removed_cb,
                                       qplugin_qserver_event_listener::type_room_event_end room_event_end_cb,
                                       qplugin_qserver_event_listener::type_room_event_countdown_to_start room_event_countdown_to_start_cb,
                                       qplugin_qserver_event_listener::type_room_event_countdown_cancelled room_event_countdown_cancelled_cb) {
    qaddress server_addr(server_address);
    qaddress redis_addr(redis_address);
    struct qnetworkserver::runserverconfig* config = new struct qnetworkserver::runserverconfig();
    config->host = server_addr.ip;
    config->port = qstring::format_string("%d", server_addr.port);
    config->redis_ip = redis_addr.ip;
    config->redis_port = redis_addr.port;
    config->zk_uri = zk_uri;
    config->root_dir = fs::path(root_dir);
    config->app_id = app_id;

    qplugin_qserver_event_listener* listener = DEBUG_NEW qplugin_qserver_event_listener(
                                                                                        pre_start_cb, start_cb, stop_cb, error_cb,
                                                                                        room_event_create_cb,
                                                                                        room_event_start_cb,
                                                                                        room_event_player_added_cb,
                                                                                        room_event_message_cb,
                                                                                        room_player_removed_cb,
                                                                                        room_event_end_cb,
                                                                                        room_event_countdown_to_start_cb,
                                                                                        room_event_countdown_cancelled_cb);
    std::tuple<qnetworkserver::runserverconfig*, qplugin_qserver_event_listener*>* tuple_in = DEBUG_NEW std::tuple<qnetworkserver::runserverconfig*, qplugin_qserver_event_listener*>(config, listener);
    if (pthread_create(&config->run_thread_id, nullptr, gsdk::server::spawn_qserver_internal, (void*)tuple_in) < 0) {
        debug_print_error(__LOGTAG__, "spawn_qserver - could not create thread: %s - %d", strerror(errno), errno);
        GX_DELETE(tuple_in);
        GX_DELETE(config);
        GX_DELETE(listener);
        return 1;
    }
    return 0;
}

void* gsdk::server::spawn_qserver_internal(void* data) {
    std::tuple<qnetworkserver::runserverconfig*, qplugin_qserver_event_listener*>* tuple_in = (std::tuple<qnetworkserver::runserverconfig*, qplugin_qserver_event_listener*>*) data;
    qnetworkserver::runserverconfig* config = (qnetworkserver::runserverconfig*)std::get<0>(*tuple_in);
    qplugin_qserver_event_listener* listener = (qplugin_qserver_event_listener*)std::get<1>(*tuple_in);
    plugin_gameserver server(config->zk_uri);
    server.run(config->host, config->port, config->root_dir, config->redis_ip, config->redis_port, config->app_id, listener);
    GX_DELETE(tuple_in);
    GX_DELETE(config);
    GX_DELETE(listener);
    pthread_exit(0);
}

EXPORT uint64_t gsdk::server::qserver_logfile(qnetworkserver* server, qlogfile::log_lvls lvl, qcustomlogger::elog_type type, const char* tag, const char* pid, const char* roomid, const char* message) {
    return server->get_file_logger()->log(lvl, type, tag, pid, roomid, message);
}

EXPORT size_t gsdk::server::qserver_stats_count(qnetworkserver* server, const char* counter, long count_val, const char* session, const char* pid, const char* version, const char* epic, const char* myth, const char* legend,
                           const char* story, const char* message) {
    return server->get_stats_loggeer()->server_count(counter, count_val, session, pid, version, epic, myth, legend, story, message);
}
