//
//  http3_command_server.cpp
//  qh3server
//
//  Created by Arun A on 31/10/23.
//

#include "http3_command_server.hpp"
#include "../../common/gxcrc32.h"

http3_command_server::http3_command_server(const qstring& redis_ip, int redis_port, bridge_command_center* bridge_) : bridge(bridge_) {
    hiredis = DEBUG_NEW qhiredis();
    hiredis->connect_redis(redis_ip, redis_port);
}

http3_command_server::~http3_command_server() {
    GX_DELETE(hiredis);
}

void http3_command_server::on_run_started() {
    hiredis->set_value("command_center", qstring::format_string("%s:%s", host_id.c_str(), port_id.c_str()));
}

void http3_command_server::on_run_end() {
    
}

bool http3_command_server::is_log_quiche() {
    if (hiredis == nullptr) {
        return false;
    }
    qstring is_log_quiche;
    if (hiredis->get_value("is_log_quiche", is_log_quiche)==0) {
        return is_log_quiche=="true";
    }
    return false;
}

void http3_command_server::parse_header(const qstring& name, const qstring& value, struct conn_io *conn_io) {
    qh3server::parse_header(name, value, conn_io);
}

void http3_command_server::parse(struct conn_io *conn_io) {
    const char* const_logtag = logtag.c_str();
    const char* port_id_cstr = port_id.c_str();
    conn_io_req_res::header* path_header = conn_io->http_request->get_header(":path");
    if (path_header == nullptr) {
        DEBUG_PRINT_ERROR(const_logtag, "path_header == null, returning. !!!");
        qh3server::get_stats_loggeer()->server_count("parse", 1, "", "", "", "error", "http3_command_server", "", port_id_cstr, "path_not_found");
        return;
    }
    
    if (path_header->value.length()<=1) {
        DEBUG_PRINT_WARN(const_logtag, "path is very short - %s, returning. !!!", path_header->value.c_str());
        qh3server::get_stats_loggeer()->server_count("parse", 1, "", "", "", "warn", "http3_command_server", path_header->value.c_str(), port_id_cstr, "short_path");
        return;
    }
    
    // parse paths
    if (path_header->value.compare("/shutdown_test")==0) {
        parse_shutdown_test(path_header, conn_io);
    } if (path_header->value.compare("/shutdown_cmd_center")==0) {
        parse_shutdown_command_center(path_header, conn_io);
    } else if (path_header->value.compare("/whoami")==0) {
        parse_whoami(path_header, conn_io);
    }
}

void http3_command_server::parse_shutdown_command_center(conn_io_req_res::header* path_header, struct conn_io *conn_io) {
    const char* const_logtag = logtag.c_str();
    const char* port_id_cstr = port_id.c_str();
    bool has_crc_header = conn_io->http_request->has_crc_header();
    if (has_crc_header) {
        bool validate = conn_io->http_request->validate();
        assert(validate);
    } else {
        // may be called from a browser
        DEBUG_PRINT_IMPORTANT2(const_logtag, "May be '%s' requested from browser. So crc validation not possible !!!", path_header->value.c_str());
    }
    qh3server::get_stats_loggeer()->server_count("parse", 1, "", "", "", "command", "http3_command_server", has_crc_header ? "" : "no-crc", port_id_cstr, path_header->value.c_str());
    ev_break(conn_io->bridge->get_mainloop(), EVBREAK_ONE);
}
    
void http3_command_server::parse_shutdown_test(conn_io_req_res::header* path_header, struct conn_io *conn_io) {
    const char* const_logtag = logtag.c_str();
    const char* port_id_cstr = port_id.c_str();
    bool has_crc_header = conn_io->http_request->has_crc_header();
    if (has_crc_header) {
        bool validate = conn_io->http_request->validate();
        assert(validate);
    } else {
        // may be called from a browser
        DEBUG_PRINT_IMPORTANT2(const_logtag, "May be '%s' requested from browser. So crc validation not possible !!!", path_header->value.c_str());
    }
    qh3server::get_stats_loggeer()->server_count("parse", 1, "", "", "", "command", "http3_command_server", has_crc_header ? "" : "no-crc", port_id_cstr, path_header->value.c_str());
    send_shutdown_to_all();
    conn_io->http_response->set_payload("{ shutdown-all }");
}

void http3_command_server::parse_whoami(conn_io_req_res::header* path_header, struct conn_io *conn_io) {
    const char* const_logtag = logtag.c_str();
    const char* port_id_cstr = port_id.c_str();
    bool has_crc_header = conn_io->http_request->has_crc_header();
    if (has_crc_header) {
        bool validate = conn_io->http_request->validate();
        if (!validate) {
            qh3server::get_stats_loggeer()->server_count("parse", 1, "", "", "", "error", "http3_command_server", path_header->value.c_str(), port_id_cstr, "crc_fail");
        }
        assert(validate);
    } else {
        // may be called from a browser
        DEBUG_PRINT_IMPORTANT2(const_logtag, "May be '%s' requested from browser. So crc validation not possible !!!", path_header->value.c_str());
    }
    const char* res_string = "{\"name\" : \"http3_command_server\"}";
    conn_io->http_response->set_payload(qstring(res_string, strlen(res_string)));
    qh3server::get_file_logger()->log(qlogfile::level_0, const_logtag, "%s - whoami - %s", path_header->value.c_str(), res_string);
    qh3server::get_stats_loggeer()->server_count("parse", 1, "", "", "", "command", "http3_command_server", has_crc_header ? "" : "no-crc", port_id_cstr, path_header->value.c_str());
}

void http3_command_server::send_shutdown_to_all() {
    conn_io_req_res* req = conn_io_req_res::create("/shutdown_test", "");
    qh3client_helper::send_async_request(host_id.c_str(), "4004", req,
        [this](conn_io_req_res* response) {
            DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "shutdown-return");
        });
}

void http3_command_server::command_feedback_recv_cb(EV_P_ ev_io* w, int revents) {
    UNUSED(revents);
    route* route_client = (route*)w->data;
    bridge_command_center* bridge = (bridge_command_center*)route_client->arg;
    static uint8_t buf_r[65535];
    while (1) {
        struct sockaddr_storage peer_addr;
        socklen_t peer_addr_len = sizeof(peer_addr);
        memset(&peer_addr, 0, peer_addr_len);

        ssize_t read = recvfrom(route_client->bridge_sock, buf_r, sizeof(buf_r), 0,
            (struct sockaddr*)&peer_addr,
            &peer_addr_len);

        if (read < 0) {
            if ((errno == EWOULDBLOCK) || (errno == EAGAIN)) {
                DEBUG_PRINT(LOG_LEVEL_3, __LOGTAG__, "recv would block");
                break;
            }

            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to read");
            return;
        }
        
        qstring cmd(buf_r, read);
        DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "RECEIVED cmd feedback from client - %s !!!", cmd.c_str());
        bridge->cmd_feedback_from_client((struct sockaddr*)&peer_addr, cmd.c_str());
    }
}
