//
//  qh3server.cpp
//  qh3server
//
//  Created by Arun A on 30/10/23.
//

#include "qh3server.hpp"

qh3server::~qh3server() {
    DEBUG_PRINT_IMPORTANT2(logtag.c_str(), "qh3server destroyed !!!");
}

void qh3server::debug_log(const uint8_t* line, void* argp) {
    UNUSED(argp);
    qh3server* server = (qh3server*)argp;
    if (server!=nullptr && server->is_log_quiche()) {
        DEBUG_PRINT(LOG_LEVEL_0, server->logtag.c_str(), (char*)line);
    }
}

ssize_t qh3server::flush_egress(struct ev_loop* loop, struct conn_io_qh3* conn_io) {
    const char* const_logtag = logtag.c_str();
    const bool via_router = relay_through_router_info && relay_through_router_info->serialised_buffer.length()>=ORIGINAL_CLIENT_ADDR_SZ;
    SendInfo send_info;
    ssize_t total_bytes_sent = 0;
    while (1) {
        ssize_t written = quiche_conn_send(conn_io->conn, out, sizeof(out),
            &send_info);

        if (written == QUICHE_ERR_DONE) {
            DEBUG_PRINT(LOG_LEVEL_4, const_logtag, "done writing");
            break;
        }

        if (written < 0) {
            DEBUG_PRINT_ERROR(const_logtag, "failed to create packet: %zd", written);
            return -1;
        }

        // if relay through router
        if (via_router) {
            memcpy((void*)&out[written], (void*)conn_io->original_client_serialised_buffer.c_str(), ORIGINAL_CLIENT_ADDR_SZ);
            written+=ORIGINAL_CLIENT_ADDR_SZ;
        }
        //
        
        ssize_t sent = sendto(conn_io->sock, out, written, 0,
            (struct sockaddr*)&conn_io->peer_addr,
            conn_io->peer_addr_len);
#if LOG_LEVEL >= LOG_LEVEL_4
        char name[INET6_ADDRSTRLEN];
        char port[10];
        getnameinfo((struct sockaddr*)&conn_io->peer_addr, sizeof(struct sockaddr), name, sizeof(name), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
        DEBUG_PRINT(LOG_LEVEL_0, const_logtag, "send to %s:%s bytes:%d", name, port, sent);
#endif
        if (sent != written) {
            char name[INET6_ADDRSTRLEN];
            char port[10];
            getnameinfo((struct sockaddr*)&conn_io->peer_addr, sizeof(struct sockaddr), name, sizeof(name), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
            DEBUG_PRINT_ERROR(const_logtag, "ERROR (flush_egress) sending to %s:%s", name, port);
            DEBUG_PRINT_ERROR(const_logtag, "failed to send - flush_egress %d<>%d", sent, written);
            return -1;
        }

        qh3server::get_stats_loggeer()->server_count("flush_egress", sent, "", "", "", "tx", "qh3server", "", port_id.c_str());
        DEBUG_PRINT(LOG_LEVEL_4, const_logtag, "sent %zd bytes", sent);
        //return sent;
        total_bytes_sent+=sent;
    }

    double t = quiche_conn_timeout_as_nanos(conn_io->conn) / 1e9f;
    conn_io->timer.repeat = t;
    ev_timer_again(loop, &conn_io->timer);
    return total_bytes_sent;
}

void qh3server::mint_token(const uint8_t* dcid, size_t dcid_len,
    struct sockaddr_storage* addr, socklen_t addr_len,
    uint8_t* token, size_t* token_len) {
    memcpy(token, "quiche", sizeof("quiche") - 1);
    memcpy(token + sizeof("quiche") - 1, addr, addr_len);
    memcpy(token + sizeof("quiche") - 1 + addr_len, dcid, dcid_len);

    *token_len = sizeof("quiche") - 1 + addr_len + dcid_len;
}

bool qh3server::validate_token(const uint8_t* token, size_t token_len,
    struct sockaddr_storage* addr, socklen_t addr_len,
    uint8_t* odcid, size_t* odcid_len) {
    if ((token_len < sizeof("quiche") - 1) ||
        memcmp(token, "quiche", sizeof("quiche") - 1)) {
        return false;
    }

    token += sizeof("quiche") - 1;
    token_len -= sizeof("quiche") - 1;

    if ((token_len < addr_len) || memcmp(token, addr, addr_len)) {
        return false;
    }

    token += addr_len;
    token_len -= addr_len;

    if (*odcid_len < token_len) {
        return false;
    }

    memcpy(odcid, token, token_len);
    *odcid_len = token_len;

    return true;
}

uint8_t* qh3server::gen_cid(uint8_t* cid, size_t cid_len) {
    int rng = open("/dev/urandom", O_RDONLY);
    if (rng < 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to open /dev/urandom");
        return NULL;
    }

    ssize_t rand_len = read(rng, cid, cid_len);
    if (rand_len < 0) {
        close(rng);
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create connection ID");
        return NULL;
    }
    close(rng);
    return cid;
}

struct conn_io_qh3* qh3server::create_conn(uint8_t* scid, size_t scid_len,
    uint8_t* odcid, size_t odcid_len,
    struct sockaddr* local_addr,
    socklen_t local_addr_len,
    struct sockaddr_storage* peer_addr,
    socklen_t peer_addr_len,
    struct sockaddr_storage* peer_original_client_addr) {
    const char* const_logtag = logtag.c_str();
    struct conn_io_qh3* new_conn_io = DEBUG_NEW struct conn_io_qh3();
    if (new_conn_io == NULL) {
        DEBUG_PRINT_ERROR(const_logtag, "failed to allocate connection IO");
        return NULL;
    }
    new_conn_io->creation_time = ev_now(mainloop);
    
    if (scid_len != LOCAL_CONN_ID_LEN) {
        DEBUG_PRINT_ERROR(const_logtag, "failed, scid length too short");
    }

    memcpy(new_conn_io->cid, scid, LOCAL_CONN_ID_LEN);

    Connection* conn = quiche_accept(new_conn_io->cid, LOCAL_CONN_ID_LEN,
        odcid, odcid_len,
        local_addr,
        local_addr_len,
        (struct sockaddr*)peer_original_client_addr,
        peer_addr_len,
        config);

    if (conn == NULL) {
        DEBUG_PRINT_ERROR(const_logtag, "failed to create connection");
        return NULL;
    }

    new_conn_io->sock = conns->sock;
    new_conn_io->conn = conn;
    new_conn_io->bridge = this;

    memcpy(&new_conn_io->peer_addr, peer_addr, peer_addr_len);
    new_conn_io->peer_addr_len = peer_addr_len;
    
    ev_init(&new_conn_io->timer, timeout_cb);
    new_conn_io->timer.data = new_conn_io;

    HASH_ADD(hh, conns->h, cid, LOCAL_CONN_ID_LEN, new_conn_io);

    DEBUG_PRINT(LOG_LEVEL_4, const_logtag, "new connection");

    return new_conn_io;
}

void qh3server::parse_header(const qstring& name, const qstring& value, struct conn_io_qh3* conn_io) {
    const char* const_logtag = logtag.c_str();
    if (name.compare(":path") == 0) {
        DEBUG_PRINT(LOG_LEVEL_4, const_logtag, "got HTTP header: %s=%s",
            name.c_str(), value.c_str());
    }
    else {
        DEBUG_PRINT(LOG_LEVEL_4, const_logtag, "got HTTP header: %s=%s",
            name.c_str(), value.c_str());
    }
    conn_io->http_request->add_or_get_header(name, value);
}

void qh3server::parse(struct conn_io_qh3* conn_io) {
    UNUSED(conn_io);
}

int qh3server::for_each_header(const uint8_t* name, size_t name_len,
    const uint8_t* value, size_t value_len,
    void* argp) {
    struct conn_io_qh3* conn_io = (struct conn_io_qh3*)argp;
    conn_io->bridge->parse_header(qstring(name, name_len), qstring(value, value_len), conn_io);
    return 0;
}

void qh3server::recv_cb(EV_P_ ev_io* w, int revents) {
    UNUSED(revents);
    qh3server* server = (qh3server*)w->data;
    struct connections* conns = server->conns;
    struct conn_io_qh3* tmp, * conn_io = NULL;
    const char* const_logtag = server->logtag.c_str();
    const char* port_id_cstr = server->port_id.c_str();
    const bool via_router = server->relay_through_router_info && server->relay_through_router_info->serialised_buffer.length()>=ORIGINAL_CLIENT_ADDR_SZ;
    while (1) {
        struct sockaddr_storage peer_addr;
        struct sockaddr_storage peer_original_client_addr;
        socklen_t peer_addr_len = sizeof(peer_addr);
        memset(&peer_addr, 0, peer_addr_len);
        qstring original_client_serialised_buffer;
        
        ssize_t read = recvfrom(conns->sock, server->buf, sizeof(buf), 0,
            (struct sockaddr*)&peer_addr,
            &peer_addr_len);

        if (read < 0) {
            if ((errno == EWOULDBLOCK) || (errno == EAGAIN)) {
                DEBUG_PRINT(LOG_LEVEL_4, const_logtag, "recv would block");
                break;
            }

            DEBUG_PRINT_ERROR(const_logtag, "failed to read");
            return;
        }

        server->get_stats_loggeer()->server_count("recv_cb", read, "", "", "", "rx", "qh3server", "", port_id_cstr);
        
#if LOG_LEVEL >= LOG_LEVEL_4
        char name[INET6_ADDRSTRLEN];
        char port[10];
        getnameinfo((struct sockaddr*)&peer_addr, sizeof(struct sockaddr), name, sizeof(name), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
        DEBUG_PRINT(LOG_LEVEL_0, const_logtag, "peer addr %s:%s read:%d", name, port, read);
#endif
        
        // if relay through router
        if (server->relay_through_router_info) {
            memset(&peer_original_client_addr, 0, peer_addr_len);
            read = read - peer_addr_len;    // remove the client info
            struct sockaddr* client_info = (struct sockaddr*)&peer_original_client_addr;
            memcpy((void*)client_info, (void*)&server->buf[read], peer_addr_len);

            // serialize the original client address for later use
            qaddress original_client_address((struct sockaddr*)&peer_original_client_addr);
            original_client_address.serialise(original_client_serialised_buffer);
            
            // update the peer address port (return port)
            essentials::update_port((struct sockaddr*)&peer_addr, server->relay_through_router_info->port_return);
#if LOG_LEVEL >= LOG_LEVEL_4
            DEBUG_PRINT(LOG_LEVEL_0, const_logtag, "crc of orinal-client addr (last %d bytes) = 0x%x", peer_addr_len, essentials::get_crc(&server->buf[read], peer_addr_len));
            getnameinfo((struct sockaddr*)&peer_original_client_addr, sizeof(struct sockaddr), name, sizeof(name), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
            DEBUG_PRINT_IMPORTANT2(const_logtag, "original-client-address %s:%s", name, port);
            getnameinfo((struct sockaddr*)&peer_addr, sizeof(struct sockaddr), name, sizeof(name), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
            DEBUG_PRINT_IMPORTANT2(const_logtag, "modified-peer address %s:%s", name, port);
#endif
        } else {
            memcpy(&peer_original_client_addr, &peer_addr, peer_addr_len);
        }
        //
        
        uint8_t type;
        uint32_t version;

        uint8_t scid[MAX_CID_LEN];
        size_t scid_len = sizeof(scid);

        uint8_t dcid[MAX_CID_LEN];
        size_t dcid_len = sizeof(dcid);

        uint8_t odcid[MAX_CID_LEN];
        size_t odcid_len = sizeof(odcid);

        uint8_t token[MAX_TOKEN_LEN];
        size_t token_len = sizeof(token);

        int rc = quiche_header_info(server->buf, read, LOCAL_CONN_ID_LEN, &version,
            &type, scid, &scid_len, dcid, &dcid_len,
            token, &token_len);
        if (rc < 0) {
            DEBUG_PRINT_ERROR(const_logtag, "failed to parse header: %d", rc);
            server->get_stats_loggeer()->server_count("recv_cb", 1, "", "", "", "error", "qh3server", "parse_header_fail", port_id_cstr);
            return;
        }
        
        HASH_FIND(hh, conns->h, dcid, dcid_len, conn_io);

        if (conn_io == NULL) {
            if (!quiche_version_is_supported(version)) {
                DEBUG_PRINT(LOG_LEVEL_4, const_logtag, "version negotiation");

                ssize_t written = quiche_negotiate_version(scid, scid_len,
                    dcid, dcid_len,
                    server->out, sizeof(server->out));

                if (written < 0) {
                    DEBUG_PRINT_ERROR(const_logtag, "failed to create vneg packet: %zd",
                        written);
                    server->get_stats_loggeer()->server_count("recv_cb", 1, "", "", "", "error", "qh3server", "version_negotiation_fail", port_id_cstr, qstring::format_string("failed to create vneg packet: %zd", written));
                    continue;
                }

                // if relay through router
                if (via_router) {
                    memcpy((void*)&server->out[written], (void*)original_client_serialised_buffer.c_str(), ORIGINAL_CLIENT_ADDR_SZ);
                    written+=ORIGINAL_CLIENT_ADDR_SZ;
                }
                //
                ssize_t sent = sendto(conns->sock, server->out, written, 0,
                    (struct sockaddr*)&peer_addr,
                    peer_addr_len);
                
#if LOG_LEVEL >= LOG_LEVEL_4
                char name[INET6_ADDRSTRLEN];
                char port[10];
                getnameinfo((struct sockaddr*)&peer_addr, sizeof(struct sockaddr), name, sizeof(name), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
                DEBUG_PRINT(LOG_LEVEL_0, const_logtag, "send to %s:%s bytes:%d", name, port, sent);
#endif
                if (sent != written) {
                    char name[INET6_ADDRSTRLEN];
                    char port[10];
                    getnameinfo((struct sockaddr*)&peer_addr, sizeof(struct sockaddr), name, sizeof(name), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
                    DEBUG_PRINT_ERROR(const_logtag, "ERROR (conn_io == NULL) sending to %s:%s", name, port);
                    DEBUG_PRINT_ERROR(const_logtag, "failed to send - recv_cb (conn_io == NULL) %d<>%d", sent, written);
                    continue;
                }

                server->get_stats_loggeer()->server_count("recv_cb", sent, "", "", "", "tx", "qh3server", "", port_id_cstr);
                DEBUG_PRINT(LOG_LEVEL_4, const_logtag, "sent %zd bytes", sent);
                continue;
            }

            if (token_len == 0) {
                DEBUG_PRINT(LOG_LEVEL_4, const_logtag, "stateless retry");

                server->mint_token(dcid, dcid_len, &peer_original_client_addr, peer_addr_len,
                    token, &token_len);

                uint8_t new_cid[LOCAL_CONN_ID_LEN];

                if (gen_cid(new_cid, LOCAL_CONN_ID_LEN) == NULL) {
                    continue;
                }

                ssize_t written = quiche_retry(scid, scid_len,
                    dcid, dcid_len,
                    new_cid, LOCAL_CONN_ID_LEN,
                    token, token_len,
                    version, server->out, sizeof(server->out));

                if (written < 0) {
                    DEBUG_PRINT_ERROR(const_logtag, "failed to create retry packet: %zd",
                        written);
                    continue;
                }

                // if relay through router
                if (via_router) {
                    memcpy((void*)&server->out[written], (void*)original_client_serialised_buffer.c_str(), ORIGINAL_CLIENT_ADDR_SZ);
                    written+=ORIGINAL_CLIENT_ADDR_SZ;
                }
                //
                ssize_t sent = sendto(conns->sock, server->out, written, 0,
                    (struct sockaddr*)&peer_addr,
                    peer_addr_len);
                
#if LOG_LEVEL >= LOG_LEVEL_4
                char name[INET6_ADDRSTRLEN];
                char port[10];
                getnameinfo((struct sockaddr*)&peer_addr, sizeof(struct sockaddr), name, sizeof(name), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
                DEBUG_PRINT(LOG_LEVEL_0, const_logtag, "send to %s:%s bytes:%d", name, port, sent);
#endif

                if (sent != written) {
                    char name[INET6_ADDRSTRLEN];
                    char port[10];
                    getnameinfo((struct sockaddr*)&peer_addr, sizeof(struct sockaddr), name, sizeof(name), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
                    DEBUG_PRINT_ERROR(const_logtag, "ERROR sending to %s:%s", name, port);
                    DEBUG_PRINT_ERROR(const_logtag, "failed to send %d<>%d", sent, written);
                    continue;
                }


                server->get_stats_loggeer()->server_count("recv_cb", sent, "", "", "", "tx", "qh3server", "", port_id_cstr);
                DEBUG_PRINT(LOG_LEVEL_4, const_logtag, "sent %zd bytes", sent);
                continue;
            }


            if (!server->validate_token(token, token_len, &peer_original_client_addr, peer_addr_len,
                odcid, &odcid_len)) {
                DEBUG_PRINT_WARN(const_logtag, "invalid address validation token");
                continue;
            }

            conn_io = server->create_conn(dcid, dcid_len, odcid, odcid_len,
                conns->local_addr, conns->local_addr_len,
                &peer_addr, peer_addr_len, &peer_original_client_addr);

            if (conn_io == NULL) {
                continue;
            }
            // cache the original client adress for later use. (flush_engress)
            conn_io->original_client_serialised_buffer.bin_copy((const uint8_t*)original_client_serialised_buffer.c_str(), original_client_serialised_buffer.length());
            
            server->get_stats_loggeer()->set_total_ram((int)(essentials::get_process_used_mem()));
            server->get_stats_loggeer()->server_count("recv_cb", 1, "", "", "", "", "qh3server", "create_conn_io", port_id_cstr);
        }

        RecvInfo recv_info = {
            (struct sockaddr*)&peer_addr,
            peer_addr_len,

            conns->local_addr,
            conns->local_addr_len,
        };

        ssize_t done = quiche_conn_recv(conn_io->conn, server->buf, read, &recv_info);

        if (done < 0) {
            DEBUG_PRINT_ERROR(const_logtag, "failed to process packet: %zd", done);
            server->get_stats_loggeer()->server_count("recv_cb", 1, "", "", "", "error", "qh3server", "process_packet_fail", port_id_cstr);
            continue;
        }

        DEBUG_PRINT(LOG_LEVEL_4, const_logtag, "recv %zd bytes", done);

        if (quiche_conn_is_established(conn_io->conn)) {
            Event* ev;

            if (conn_io->http3 == NULL) {
                conn_io->http3 = quiche_h3_conn_new_with_transport(conn_io->conn,
                    server->http3_config);
                if (conn_io->http3 == NULL) {
                    server->get_stats_loggeer()->server_count("recv_cb", 1, "", "", "", "error", "qh3server", "http3_conn_fail", port_id_cstr);
                    DEBUG_PRINT_ERROR(const_logtag, "failed to create HTTP/3 connection");
                    continue;
                }
            }

            // pending
            const conn_io_req_res::payload& payload = conn_io->http_response->get_payload();
            if (conn_io->total_sent_bytes < (ssize_t)payload.buffer.length()) {
                server->send_in_chunks(conn_io);
                if (conn_io->total_sent_bytes==payload.buffer.length()) {
                    DEBUG_PRINT(LOG_LEVEL_4, __LOGTAG__, "FINISH Stream sending .... [%d] [%d]", conn_io->total_sent_bytes, payload.buffer.length());
                }
            }
            //
            
            while (1) {
                int64_t s = quiche_h3_conn_poll(conn_io->http3,
                    conn_io->conn,
                    (const struct Event**)&ev);

                if (s < 0) {
                    break;
                }

                switch (quiche_h3_event_type(ev)) {
                case Event_type::Headers: {
                    int rc = quiche_h3_event_for_each_header((const struct Event*)ev,
                        for_each_header,
                        conn_io);

                    if (rc != 0) {
                        DEBUG_PRINT_ERROR(const_logtag, "failed to process headers");
                        server->get_stats_loggeer()->server_count("recv_cb", 1, "", "", "", "error", "qh3server", "process_header_fail", port_id_cstr);
                    }
                    break;
                }

                case Event_type::Data: {
                    //                        DEBUG_PRINT(LOG_LEVEL_1, __LOGTAG__, "got HTTP req body");
                    //                        conn_io->http_request.clear_payload();
                    for (;;) {
                        ssize_t len = quiche_h3_recv_body(conn_io->http3,
                            conn_io->conn, s,
                            server->buf, sizeof(server->buf));
                        if (len <= 0) {
                            break;
                        }
                        conn_io->http_request->set_payload(qstring(server->buf, len));
                        DEBUG_PRINT(LOG_LEVEL_4, const_logtag, "%.*s", (int)len, server->buf);
                    }
                    break;
                }

                case Event_type::Finished: {
                    EV_START_RECORD(parse_start_time);
                    conn_io->bridge->parse(conn_io);
                    EV_STOP_RECORD(parse_start_time, const_logtag, "parse-time t:%lu ms", 200);

                    EV_START_RECORD(send_start_time);
                    if (payload.buffer.length() == 0) {
                        DEBUG_PRINT(LOG_LEVEL_4, const_logtag, "no-response. ignoring the request!!!");
                        conn_io->http_response->set_payload(qstring("{}", strlen("{}")));
                    }
                    const qstring& content_length_data = qstring::format_string("%d", (int)payload.buffer.length());
                    const qstring& crc = payload.get_crc_string();

                    int header_size = 5;
                    conn_io_req_res::header* status_header = conn_io->http_response->get_header(":status");
                    Header* headers = DEBUG_NEW Header[header_size + conn_io->http_response->headers.size()];
                    headers[0] = {
                        .name = (uint8_t*)":status",
                        .name_len = sizeof(":status") - 1,

                        .value = status_header ? (uint8_t*)status_header->value.c_str() : (uint8_t*)"200",
                        .value_len = status_header ? status_header->value.length() : sizeof("200") - 1,
                    };
                    headers[1] = {
                        .name = (uint8_t*)"Alternate-Protocol",
                        .name_len = sizeof("Alternate-Protocol") - 1,

                        .value = (uint8_t*)conns->quic_alternate_protocol_str.c_str(),
                        .value_len = conns->quic_alternate_protocol_str.length() - 1,
                    };

                    headers[2] = {
                        .name = (uint8_t*)"server",
                        .name_len = sizeof("server") - 1,

                        .value = (uint8_t*)"quiche",
                        .value_len = sizeof("quiche") - 1,
                    };
                    headers[3] = {
                        .name = (uint8_t*)"content-length",
                        .name_len = sizeof("content-length") - 1,

                        .value = (uint8_t*)content_length_data.c_str(),
                        .value_len = content_length_data.length(),
                    };
                    headers[4] = {
                        .name = (uint8_t*)"crc",
                        .name_len = sizeof("crc") - 1,

                        .value = (uint8_t*)crc.c_str(),
                        .value_len = crc.length(),
                    };

                    int additional_header_index = 0;
                    for (auto it : conn_io->http_response->headers) {
                        headers[header_size + additional_header_index] = {
                            .name = (uint8_t*)it.second->name.c_str(),
                            .name_len = it.second->name.length(),

                            .value = (uint8_t*)it.second->value.c_str(),
                            .value_len = it.second->value.length(),
                        };
                        DEBUG_PRINT(LOG_LEVEL_4, const_logtag, "custom header %s - %s", it.second->name.c_str(), it.second->value.c_str());
                        additional_header_index++;
                    }
                    quiche_h3_send_response(conn_io->http3, conn_io->conn,
                        s, headers, header_size + conn_io->http_response->headers.size(), false);
                    GX_DELETE_ARY(headers);
                    
                    // payload
                    conn_io->total_sent_bytes = 0;  // reset the total bytes sent over network
                    ssize_t bytes_to_send = payload.buffer.length();
                    if (bytes_to_send < SEND_CHUNK_SIZE) {    // if small chunk then try issue in one go.
                        ssize_t sent = quiche_h3_send_body(conn_io->http3, conn_io->conn, s,
                            (uint8_t*)payload.buffer.c_str(), bytes_to_send,
                            true);
                        if (sent < 0) {
                            break;
                        }
                        conn_io->total_sent_bytes+=sent;
                        if (conn_io->total_sent_bytes != (ssize_t)payload.buffer.length()) {
                            DEBUG_PRINT_ERROR(const_logtag, "HTTP response send failure %d<>%d", conn_io->total_sent_bytes, payload.buffer.length());
                            server->get_stats_loggeer()->server_count("recv_cb", 1, "", conn_io->total_sent_bytes, (ssize_t)payload.buffer.length(), "error", "qh3server", "response_send_fail", port_id_cstr);
                            break;
                        }
                    } else {
                        conn_io->stream_id = s;
                        server->send_in_chunks(conn_io);
                        DEBUG_PRINT(LOG_LEVEL_4, __LOGTAG__, "START Stream sending .... [%d] [%d]", conn_io->total_sent_bytes, payload.buffer.length());
                        if (conn_io->total_sent_bytes < (ssize_t)payload.buffer.length()) {
                            DEBUG_PRINT(LOG_LEVEL_4, const_logtag, "(Partial) HTTP response send %d<>%d", conn_io->total_sent_bytes, payload.buffer.length());
                        }
                    }
                    
                    EV_STOP_RECORD(send_start_time, const_logtag, "send-time t:%lu ms", 200);
                    DEBUG_PRINT(LOG_LEVEL_4, const_logtag, "sent HTTP response over %" PRId64 " with body %s", s, payload.buffer.c_str());
                }
                    break;

                case Event_type::Reset:
                    break;

                case Event_type::PriorityUpdate:
                    break;

                case Event_type::GoAway: {
                    DEBUG_PRINT(LOG_LEVEL_1, const_logtag, "got GOAWAY");
                    break;
                }
                }

                quiche_h3_event_free(ev);
            }
        }
    }

    HASH_ITER(hh, conns->h, conn_io, tmp) {
        server->flush_egress(loop, conn_io);

        if (quiche_conn_is_closed(conn_io->conn)) {
            Stats stats;
            PathStats path_stats;

            quiche_conn_stats(conn_io->conn, &stats);
            quiche_conn_path_stats(conn_io->conn, 0, &path_stats);

            DEBUG_PRINT(LOG_LEVEL_4, const_logtag, "connection closed, recv=%zu sent=%zu lost=%zu rtt=%" PRIu64 "ns cwnd=%zu",
                stats.recv, stats.sent, stats.lost, path_stats.rtt, path_stats.cwnd);

            HASH_DELETE(hh, conns->h, conn_io);

            ev_timer_stop(loop, &conn_io->timer);

            quiche_conn_free(conn_io->conn);
            GX_DELETE(conn_io);
        }
    }
}

void qh3server::send_in_chunks(struct conn_io_qh3* conn_io) {
    const conn_io_req_res::payload& payload = conn_io->http_response->get_payload();
    size_t chunk_size = SEND_CHUNK_SIZE;
    uint8_t* data = (uint8_t*)payload.buffer.c_str();
    size_t start_index = conn_io->total_sent_bytes;
    ssize_t total_payload_size = payload.buffer.length();
    
     // Send the data in chunks
    for (size_t offset = start_index; offset < total_payload_size; offset += chunk_size) {
        size_t remaining = total_payload_size - offset;
        size_t chunk = remaining < chunk_size ? remaining : chunk_size;
        // Send a chunk of the data
        bool fin = offset + chunk >= total_payload_size;
        ssize_t sent = quiche_h3_send_body(conn_io->http3, conn_io->conn, conn_io->stream_id, data + offset, chunk, fin);
        if (sent < 0) {
//            fprintf(stderr, "Error sending body: %zd\n", sent);
            break;
        }
        if (fin) {
            DEBUG_PRINT(LOG_LEVEL_4, __LOGTAG__, "fin");
        }
        conn_io->total_sent_bytes+=sent;
    }
}

void qh3server::destroy_connection(struct ev_loop* loop, struct conn_io_qh3* conn_io) {
    HASH_DELETE(hh, conns->h, conn_io);
    ev_timer_stop(loop, &conn_io->timer);
    quiche_conn_free(conn_io->conn);
    GX_DELETE(conn_io);
}

void qh3server::timeout_cb(EV_P_ ev_timer* w, int revents) {
    UNUSED(revents);
    struct conn_io_qh3* conn_io = (struct conn_io_qh3*)w->data;
    quiche_conn_on_timeout(conn_io->conn);

    DEBUG_PRINT(LOG_LEVEL_4, __LOGTAG__, "timeout");

    conn_io->bridge->flush_egress(loop, conn_io);

    if (quiche_conn_is_closed(conn_io->conn)) {
        Stats stats;
        PathStats path_stats;

        quiche_conn_stats(conn_io->conn, &stats);
        quiche_conn_path_stats(conn_io->conn, 0, &path_stats);

        DEBUG_PRINT(LOG_LEVEL_4, __LOGTAG__, "connection closedA, recv=%zu sent=%zu lost=%zu rtt=%" PRIu64 "ns cwnd=%zu",
            stats.recv, stats.sent, stats.lost, path_stats.rtt, path_stats.cwnd);

        conn_io->bridge->destroy_connection(loop, conn_io);
        return;
    }
}

int qh3server::run(const qstring& host, const qstring& port, fs::path& rootDir, struct addrinfo* router_, uint16_t command_center_feedback_port, uint16_t router_port_return) {
    const struct addrinfo hints = {
        .ai_family = PF_UNSPEC,
        .ai_socktype = SOCK_DGRAM,
        .ai_protocol = IPPROTO_UDP
    };

    host_id = host;
    port_id = port;
    GX_DELETE(relay_through_router_info);
    if (router_ != nullptr) {
        relay_through_router_info = DEBUG_NEW struct routerinfo(router_, router_port_return);
    }
    logtag = qstring::format_string("%s:%s", __LOGTAG__, port.c_str());
    const char* const_logtag = logtag.c_str();
//    quiche_enable_debug_logging(debug_log, this);

    if (is_log_quiche()) {
        DEBUG_PRINT_WARN(const_logtag, "quiche log is enabled. Perfomance may get affected due to excess logs !!!");
    }
    struct addrinfo* local;
    if (getaddrinfo(host.c_str(), port.c_str(), &hints, &local) != 0) {
        DEBUG_PRINT_ERROR(const_logtag, "failed to resolve host - port[%s]", port.c_str());
        return -1;
    }

    int sock = socket(local->ai_family, SOCK_DGRAM, 0);
    if (sock < 0) {
        DEBUG_PRINT_ERROR(const_logtag, "failed to create socket - port[%s]", port.c_str());
        freeaddrinfo(local);
        return -1;
    }

    if (fcntl(sock, F_SETFL, O_NONBLOCK) != 0) {
        DEBUG_PRINT_ERROR(const_logtag, "failed to make socket non-blocking - port[%s]", port.c_str());
        close(sock);    // (amudaliar) : Needed for running as virtual servers. Else new servers wont be able to bind.
        freeaddrinfo(local);
        return -1;
    }

    if (bind(sock, local->ai_addr, local->ai_addrlen) < 0) {
        DEBUG_PRINT_ERROR(const_logtag, "failed to bind socket - port[%s]", port.c_str());
        close(sock);
        freeaddrinfo(local);
        return -1;
    }

    config = quiche_config_new(PROTOCOL_VERSION);
    if (config == NULL) {
        DEBUG_PRINT_ERROR(const_logtag, "failed to create config");
        close(sock);
        freeaddrinfo(local);
        return -1;
    }

    fs::path certFile(rootDir / "cert.crt");
    fs::path keyFile(rootDir / "cert.key");
    DEBUG_PRINT(LOG_LEVEL_2, const_logtag, "cert file %s, key file %s", certFile.c_str(), keyFile.c_str());
    int res_crt_load = quiche_config_load_cert_chain_from_pem_file(config, certFile.c_str());
    if (res_crt_load != 0) {
        DEBUG_PRINT_ERROR(const_logtag, "CERT load error - %s", certFile.c_str());
        close(sock);
        freeaddrinfo(local);
        return -1;
    }
    int res_key_load = quiche_config_load_priv_key_from_pem_file(config, keyFile.c_str());
    if (res_key_load != 0) {
        DEBUG_PRINT_ERROR(const_logtag, "KEY load error - %s", keyFile.c_str());
        close(sock);
        freeaddrinfo(local);
        return -1;
    }

    quiche_config_set_application_protos(config,
        (uint8_t*)QUICHE_H3_APPLICATION_PROTOCOL,
        sizeof(QUICHE_H3_APPLICATION_PROTOCOL) - 1);

    quiche_config_set_max_idle_timeout(config, 25000);
    quiche_config_set_max_recv_udp_payload_size(config, MAX_DATAGRAM_SIZE);
    quiche_config_set_max_send_udp_payload_size(config, MAX_DATAGRAM_SIZE);
    quiche_config_set_initial_max_data(config, 10000000);
    quiche_config_set_initial_max_stream_data_bidi_local(config, 1000000);
    quiche_config_set_initial_max_stream_data_bidi_remote(config, 1000000);
    quiche_config_set_initial_max_stream_data_uni(config, 1000000);
    quiche_config_set_initial_max_streams_bidi(config, 100);
    quiche_config_set_initial_max_streams_uni(config, 100);
    quiche_config_set_disable_active_migration(config, true);
    quiche_config_set_cc_algorithm(config, Reno);

    http3_config = quiche_h3_config_new();
    if (http3_config == NULL) {
        DEBUG_PRINT_ERROR(const_logtag, "failed to create HTTP/3 config");
        close(sock);
        freeaddrinfo(local);
        return -1;
    }

    struct connections c;
    c.sock = sock;
    c.h = NULL;
    c.local_addr = local->ai_addr;
    c.local_addr_len = local->ai_addrlen;
    c.server_port = port;
    c.quic_alternate_protocol_str = qstring("quic:") + port;

    conns = &c;

    ev_io watcher;
    mainloop = ev_loop_new();
    ev_io_init(&watcher, recv_cb, sock, EV_READ);
    ev_io_start(mainloop, &watcher);
    watcher.data = this;

    //
    GX_DELETE(logger);
    GX_DELETE(stats_logger);
    logger = DEBUG_NEW qtextfilelogger();
    stats_logger = DEBUG_NEW qstatslogger();
    
    if (!on_server_pre_init()) {
        DEBUG_PRINT_ERROR(const_logtag, "on_server_pre_init failed !!!, Exiting.");
        GX_DELETE(stats_logger);
        GX_DELETE(logger);
        close(sock);
        freeaddrinfo(local);
        return -1;
    }
    qstring log_path = qstring::format_string("./logs/%s/qh3_logfile", port.c_str());
    qstring stats_path = qstring::format_string("./stats/%s/qh3_statfile", port.c_str());
    
    qh3server::get_file_logger()->start_session(log_path, log_path.length());
    qh3server::get_stats_loggeer()->init(essentials::get_sysname(), essentials::get_device_name(), "", 0);
    qh3server::get_stats_loggeer()->start_session(stats_path, stats_path.length());
    //
    
    //
    qtimer_sceduler close_dangling_connections_scheduler;
    close_dangling_connections_scheduler.set_ev_lopp(mainloop);
    qtimer* dangling_connections_check_timer =  close_dangling_connections_scheduler.schedule_repeat_timer([this, const_logtag](qtimer& timer) {
            int dangling_connections = 0;
            int dangling_with_response = 0;
            int flushed_on_exit = 0;
            struct conn_io_qh3* tmp, * conn_io = NULL;
            HASH_ITER(hh, conns->h, conn_io, tmp) {
                ev_tstamp elapsed = ev_now(mainloop) - conn_io->creation_time;
                if (elapsed > DROP_CONNECTION_AFTER && conn_io->timer.repeat == 0) {    // DROP_CONNECTION_AFTER seconds after connection creation time.
                    if ((true) || !quiche_conn_is_closed(conn_io->conn)) {    // flushing even if the connection is closed.
//                        DEBUG_PRINT(LOG_LEVEL_4, const_logtag, "dangling : try flush : connection is still open");
                        ssize_t sent_bytes = flush_egress(mainloop, conn_io);
                        if (sent_bytes) {
                            DEBUG_PRINT(LOG_LEVEL_4, const_logtag, "dangling : try flush : sent bytes %zd", sent_bytes);
                            flushed_on_exit++;
                        }
                    }
                    
                    if (conn_io->http_response->get_payload().buffer.length()>3) {
                        dangling_with_response++;
                    }
                    DEBUG_PRINT(LOG_LEVEL_4, const_logtag, "closing dangling connection !!!");
                    Stats stats;
                    PathStats path_stats;
                    quiche_conn_stats(conn_io->conn, &stats);
                    quiche_conn_path_stats(conn_io->conn, 0, &path_stats);
                    DEBUG_PRINT(LOG_LEVEL_4, const_logtag,
                                "dangling connection force closed, recv=%zu sent=%zu lost=%zu rtt=%" PRIu64 "ns cwnd=%zu elapsed:%10.2fs",
                        stats.recv, stats.sent, stats.lost, path_stats.rtt, path_stats.cwnd, ev_now(mainloop) - conn_io->creation_time);
                    HASH_DELETE(hh, conns->h, conn_io);
                    ev_timer_stop(mainloop, &conn_io->timer);
                    quiche_conn_free(conn_io->conn);
                    GX_DELETE(conn_io);
                    dangling_connections++;
                }
            }
            if (dangling_connections>0) {
                if (dangling_connections<10) {
                    DEBUG_PRINT(LOG_LEVEL_0, const_logtag, "Force closed %d dangling connections, with response %d. flushed_on_exit(%d)",
                                dangling_connections, dangling_with_response, flushed_on_exit);
                } else if (dangling_connections>=10 && dangling_connections <20) {
                    DEBUG_PRINT_IMPORTANT2(const_logtag, "Force closed %d dangling connections, with response %d. flushed_on_exit(%d)",
                                dangling_connections, dangling_with_response, flushed_on_exit);
                } else if (dangling_connections>=20) {
                    DEBUG_PRINT_WARN(const_logtag, "Force closed %d dangling connections, with response %d. flushed_on_exit(%d)",
                                dangling_connections, dangling_with_response, flushed_on_exit);
                }
            }
        }, 3);
    //
    
    on_run_started();
    ev_loop(mainloop, 0);

    // destroy connections
    struct conn_io_qh3* tmp, * conn_io = NULL;
    int pending_connections = 0;
    HASH_ITER(hh, conns->h, conn_io, tmp) {
        ssize_t sent_bytes = flush_egress(mainloop, conn_io);
        if (sent_bytes) {
            DEBUG_PRINT(LOG_LEVEL_3, const_logtag, "force close --> try flush : sent bytes %zd", sent_bytes);
        }
        if (quiche_conn_is_closed(conn_io->conn)) {
            DEBUG_PRINT(LOG_LEVEL_0, const_logtag, "force close : connection is already closed");
        }
        Stats stats;
        PathStats path_stats;
        quiche_conn_stats(conn_io->conn, &stats);
        quiche_conn_path_stats(conn_io->conn, 0, &path_stats);
        DEBUG_PRINT(LOG_LEVEL_3, const_logtag, "connection force closed, recv=%zu sent=%zu lost=%zu rtt=%" PRIu64 "ns cwnd=%zu",
            stats.recv, stats.sent, stats.lost, path_stats.rtt, path_stats.cwnd);
        HASH_DELETE(hh, conns->h, conn_io);
        ev_timer_stop(mainloop, &conn_io->timer);
        quiche_conn_free(conn_io->conn);
        GX_DELETE(conn_io);
        pending_connections++;
    }
    if (pending_connections>0) {
        DEBUG_PRINT(LOG_LEVEL_0, const_logtag, "Force closed %d pending connections.", pending_connections);
    }
    //

    on_run_end();
    
    close_dangling_connections_scheduler.cancel_and_destroy_timer(dangling_connections_check_timer);
    
    ev_loop_destroy(mainloop);
    
    freeaddrinfo(local);
    
    quiche_h3_config_free(http3_config);
    quiche_config_free(config);

    get_stats_loggeer()->end_session();
    get_file_logger()->end_session();
    
    DEBUG_PRINT_IMPORTANT(const_logtag, "waiting for services to finish !!!");
    struct ev_loop* wait_loop = ev_loop_new();
    qtimer_sceduler wait_scheduler;
    wait_scheduler.set_ev_lopp(wait_loop);
    qtimer* wait_timer = wait_scheduler.schedule_repeat_timer([this, wait_loop, const_logtag, host, sock, command_center_feedback_port](qtimer& timer) {
        int service_shutdown_cnt = 0;
        if (get_stats_loggeer()->config.finished) {
            DEBUG_PRINT_IMPORTANT(const_logtag, "stats service finished !!!");
            service_shutdown_cnt++;
        }
        if (get_file_logger()->config.finished) {
            DEBUG_PRINT_IMPORTANT(const_logtag, "logger service finished !!!");
            service_shutdown_cnt++;
        }
        if (service_shutdown_cnt >= 2) {
            const struct addrinfo hints = {
                .ai_family = PF_UNSPEC,
                .ai_socktype = SOCK_DGRAM,
                .ai_protocol = IPPROTO_UDP
            };
            qstring command_center_feedback_port_str = qstring::format_string("%d", command_center_feedback_port);
            struct addrinfo* cmd_center_feedback_address;
            if (getaddrinfo(host.c_str(), command_center_feedback_port_str.c_str(), &hints, &cmd_center_feedback_address) != 0) {
                DEBUG_PRINT_ERROR(const_logtag, "failed to resolve host - port[%s]", command_center_feedback_port_str.c_str());
                return;
            }
            DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "Sending shutdown-ack to %s:%s", host.c_str(), command_center_feedback_port_str.c_str());
            qstring shut_cmd = qstring::format_string("shut-ack-%s", port_id.c_str());
            ssize_t sent = sendto(sock, shut_cmd.c_str(), shut_cmd.length(), 0,
                                  cmd_center_feedback_address->ai_addr,
                                  cmd_center_feedback_address->ai_addrlen);
            if (sent != shut_cmd.length()) {
                DEBUG_PRINT_ERROR(const_logtag, "ERROR sending shutdown event to command center !!!");
            }
            freeaddrinfo(cmd_center_feedback_address);
            ev_break(wait_loop, EVBREAK_ONE);
        }
    }, 3);
    
    ev_run(wait_loop, 0);
    wait_scheduler.cancel_and_destroy_timer(wait_timer);
    ev_loop_destroy(wait_loop);
    
    close(sock);
    GX_DELETE(relay_through_router_info);
    GX_DELETE(logger);
    GX_DELETE(stats_logger);
    return 0;
}
