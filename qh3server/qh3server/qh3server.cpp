//
//  qh3server.cpp
//  qh3server
//
//  Created by Arun A on 30/10/23.
//

#include "qh3server.hpp"

qtextfilelogger* qh3server::logger = nullptr;
qstatslogger* qh3server::stats_logger = nullptr;

//void (*cb)(const uint8_t *line, void *argp), void *argp
void qh3server::debug_log(const uint8_t* line, void* argp) {
    UNUSED(argp);
    qh3server* server = (qh3server*)argp;
    if (server!=nullptr && server->is_log_quiche()) {
        DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, (char*)line);
    }
}

void qh3server::flush_egress(struct ev_loop* loop, struct conn_io* conn_io) {
    static uint8_t out[MAX_DATAGRAM_SIZE];

    SendInfo send_info;

    while (1) {
        ssize_t written = quiche_conn_send(conn_io->conn, out, sizeof(out),
            &send_info);

        if (written == QUICHE_ERR_DONE) {
            DEBUG_PRINT(LOG_LEVEL_3, __LOGTAG__, "done writing");
            break;
        }

        if (written < 0) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create packet: %zd", written);
            return;
        }

        ssize_t sent = sendto(conn_io->sock, out, written, 0,
            (struct sockaddr*)&conn_io->peer_addr,
            conn_io->peer_addr_len);
        if (sent != written) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to send");
            return;
        }
        qh3server::get_stats_loggeer()->server_count("flush_egress", sent, "", "", "", "tx", "qh3server", "");
        DEBUG_PRINT(LOG_LEVEL_3, __LOGTAG__, "sent %zd bytes", sent);
    }

    double t = quiche_conn_timeout_as_nanos(conn_io->conn) / 1e9f;
    conn_io->timer.repeat = t;
    ev_timer_again(loop, &conn_io->timer);
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

struct conn_io* qh3server::create_conn(uint8_t* scid, size_t scid_len,
    uint8_t* odcid, size_t odcid_len,
    struct sockaddr* local_addr,
    socklen_t local_addr_len,
    struct sockaddr_storage* peer_addr,
    socklen_t peer_addr_len) {
    //struct conn_io *conn_io = (struct conn_io *)calloc(1, sizeof(*conn_io));
    struct conn_io* new_conn_io = DEBUG_NEW struct conn_io();
    if (new_conn_io == NULL) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to allocate connection IO");
        return NULL;
    }

    if (scid_len != LOCAL_CONN_ID_LEN) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed, scid length too short");
    }

    memcpy(new_conn_io->cid, scid, LOCAL_CONN_ID_LEN);

    Connection* conn = quiche_accept(new_conn_io->cid, LOCAL_CONN_ID_LEN,
        odcid, odcid_len,
        local_addr,
        local_addr_len,
        (struct sockaddr*)peer_addr,
        peer_addr_len,
        config);

    if (conn == NULL) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create connection");
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

    DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "new connection");

    return new_conn_io;
}

/*
 int (*cb)(const uint8_t *name,
   size_t name_len,
   const uint8_t *value,
   size_t value_len,
   void *argp)
 */
void qh3server::parse_header(const qstring& name, const qstring& value, struct conn_io* conn_io) {
    if (name.compare(":path") == 0) {
        DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "got HTTP header: %s=%s",
            name.c_str(), value.c_str());
    }
    else {
        DEBUG_PRINT(LOG_LEVEL_3, __LOGTAG__, "got HTTP header: %s=%s",
            name.c_str(), value.c_str());
    }
    conn_io->http_request->add_or_get_header(name, value);
}

void qh3server::parse(struct conn_io* conn_io) {
    UNUSED(conn_io);
}

int qh3server::for_each_header(const uint8_t* name, size_t name_len,
    const uint8_t* value, size_t value_len,
    void* argp) {
    struct conn_io* conn_io = (struct conn_io*)argp;
    conn_io->bridge->parse_header(qstring(name, name_len), qstring(value, value_len), conn_io);
    return 0;
}

void qh3server::recv_cb(EV_P_ ev_io* w, int revents) {
    UNUSED(revents);
    qh3server* server = (qh3server*)w->data;
    struct connections* conns = server->conns;

    struct conn_io* tmp, * conn_io = NULL;

    static uint8_t buf[65535];
    static uint8_t out[MAX_DATAGRAM_SIZE];

    while (1) {
        struct sockaddr_storage peer_addr;
        socklen_t peer_addr_len = sizeof(peer_addr);
        memset(&peer_addr, 0, peer_addr_len);

        ssize_t read = recvfrom(conns->sock, buf, sizeof(buf), 0,
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

        qh3server::get_stats_loggeer()->server_count("recv_cb", read, "", "", "", "rx", "qh3server", "");
        
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

        int rc = quiche_header_info(buf, read, LOCAL_CONN_ID_LEN, &version,
            &type, scid, &scid_len, dcid, &dcid_len,
            token, &token_len);
        if (rc < 0) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to parse header: %d", rc);
            qh3server::get_stats_loggeer()->server_count("recv_cb", 1, "", "", "", "error", "qh3server", "parse_header_fail");
            return;
        }

        HASH_FIND(hh, conns->h, dcid, dcid_len, conn_io);

        if (conn_io == NULL) {
            if (!quiche_version_is_supported(version)) {
                DEBUG_PRINT(LOG_LEVEL_3, __LOGTAG__, "version negotiation");

                ssize_t written = quiche_negotiate_version(scid, scid_len,
                    dcid, dcid_len,
                    out, sizeof(out));

                if (written < 0) {
                    DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create vneg packet: %zd",
                        written);
                    qh3server::get_stats_loggeer()->server_count("recv_cb", 1, "", "", "", "error", "qh3server", "version_negotiation_fail", "", qstring::format_string("failed to create vneg packet: %zd", written));
                    continue;
                }

                ssize_t sent = sendto(conns->sock, out, written, 0,
                    (struct sockaddr*)&peer_addr,
                    peer_addr_len);
                if (sent != written) {
                    DEBUG_PRINT_ERROR(__LOGTAG__, "failed to send");
                    continue;
                }

                qh3server::get_stats_loggeer()->server_count("recv_cb", sent, "", "", "", "tx", "qh3server", "");
                DEBUG_PRINT(LOG_LEVEL_3, __LOGTAG__, "sent %zd bytes", sent);
                continue;
            }

            if (token_len == 0) {
                DEBUG_PRINT(LOG_LEVEL_3, __LOGTAG__, "stateless retry");

                server->mint_token(dcid, dcid_len, &peer_addr, peer_addr_len,
                    token, &token_len);

                uint8_t new_cid[LOCAL_CONN_ID_LEN];

                if (gen_cid(new_cid, LOCAL_CONN_ID_LEN) == NULL) {
                    continue;
                }

                ssize_t written = quiche_retry(scid, scid_len,
                    dcid, dcid_len,
                    new_cid, LOCAL_CONN_ID_LEN,
                    token, token_len,
                    version, out, sizeof(out));

                if (written < 0) {
                    DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create retry packet: %zd",
                        written);
                    continue;
                }

                ssize_t sent = sendto(conns->sock, out, written, 0,
                    (struct sockaddr*)&peer_addr,
                    peer_addr_len);
                if (sent != written) {
                    DEBUG_PRINT_ERROR(__LOGTAG__, "failed to send");
                    continue;
                }

                qh3server::get_stats_loggeer()->server_count("recv_cb", sent, "", "", "", "tx", "qh3server", "");
                DEBUG_PRINT(LOG_LEVEL_3, __LOGTAG__, "sent %zd bytes", sent);
                continue;
            }


            if (!server->validate_token(token, token_len, &peer_addr, peer_addr_len,
                odcid, &odcid_len)) {
                DEBUG_PRINT_WARN(__LOGTAG__, "invalid address validation token");
                continue;
            }

            conn_io = server->create_conn(dcid, dcid_len, odcid, odcid_len,
                conns->local_addr, conns->local_addr_len,
                &peer_addr, peer_addr_len);

            if (conn_io == NULL) {
                continue;
            }
            qh3server::get_stats_loggeer()->set_total_ram((int)(essentials::get_process_used_mem()));
            qh3server::get_stats_loggeer()->server_count("recv_cb", 1, "", "", "", "", "qh3server", "create_conn_io");
        }

        RecvInfo recv_info = {
            (struct sockaddr*)&peer_addr,
            peer_addr_len,

            conns->local_addr,
            conns->local_addr_len,
        };

        ssize_t done = quiche_conn_recv(conn_io->conn, buf, read, &recv_info);

        if (done < 0) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to process packet: %zd", done);
            qh3server::get_stats_loggeer()->server_count("recv_cb", 1, "", "", "", "error", "qh3server", "process_packet_fail");
            continue;
        }

        DEBUG_PRINT(LOG_LEVEL_3, __LOGTAG__, "recv %zd bytes", done);

        if (quiche_conn_is_established(conn_io->conn)) {
            Event* ev;

            if (conn_io->http3 == NULL) {
                conn_io->http3 = quiche_h3_conn_new_with_transport(conn_io->conn,
                    server->http3_config);
                if (conn_io->http3 == NULL) {
                    qh3server::get_stats_loggeer()->server_count("recv_cb", 1, "", "", "", "error", "qh3server", "http3_conn_fail");
                    DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create HTTP/3 connection");
                    continue;
                }
            }

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
                        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to process headers");
                        qh3server::get_stats_loggeer()->server_count("recv_cb", 1, "", "", "", "error", "qh3server", "process_header_fail");
                    }
                    break;
                }

                case Event_type::Data: {
                    //                        DEBUG_PRINT(LOG_LEVEL_1, __LOGTAG__, "got HTTP req body");
                    //                        conn_io->http_request.clear_payload();
                    for (;;) {
                        ssize_t len = quiche_h3_recv_body(conn_io->http3,
                            conn_io->conn, s,
                            buf, sizeof(buf));
                        if (len <= 0) {
                            break;
                        }
                        conn_io->http_request->set_payload(qstring(buf, len));
                        DEBUG_PRINT(LOG_LEVEL_1, __LOGTAG__, "%.*s", (int)len, buf);
                    }
                    break;
                }

                case Event_type::Finished: {
                    EV_START_RECORD(parse_start_time);
                    conn_io->bridge->parse(conn_io);
                    EV_STOP_RECORD(parse_start_time, __LOGTAG__, "parse-time t:%lu ms", 500);

                    EV_START_RECORD(send_start_time);
                    const conn_io_req_res::payload& payload = conn_io->http_response->get_payload();
                    if (payload.buffer.length() == 0) {
                        DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "no-response. ignoring the request!!!");
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
                        .value_len = conns->quic_alternate_protocol_str.size() - 1,
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
                        DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "custom header %s - %s", it.second->name.c_str(), it.second->value.c_str());
                        additional_header_index++;
                    }
                    quiche_h3_send_response(conn_io->http3, conn_io->conn,
                        s, headers, header_size + conn_io->http_response->headers.size(), false);
                    ssize_t send_len = quiche_h3_send_body(conn_io->http3, conn_io->conn, s,
                        (uint8_t*)payload.buffer.c_str(), payload.buffer.length(),
                        true);
                    GX_DELETE_ARY(headers);
                    EV_STOP_RECORD(send_start_time, __LOGTAG__, "send-time t:%lu ms", 200);
                    if (send_len != (ssize_t)payload.buffer.length()) {
                        DEBUG_PRINT_ERROR(__LOGTAG__, "HTTP response send failure");
                        qh3server::get_stats_loggeer()->server_count("recv_cb", 1, "", "", "", "error", "qh3server", "response_send_fail");
                        break;
                    }
                    DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "sent HTTP response over %" PRId64 " with body %s", s, payload.buffer.c_str());
                }
                                         break;

                case Event_type::Reset:
                    break;

                case Event_type::PriorityUpdate:
                    break;

                case Event_type::GoAway: {
                    DEBUG_PRINT(LOG_LEVEL_1, __LOGTAG__, "got GOAWAY");
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

            DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "connection closed, recv=%zu sent=%zu lost=%zu rtt=%" PRIu64 "ns cwnd=%zu",
                stats.recv, stats.sent, stats.lost, path_stats.rtt, path_stats.cwnd);

            HASH_DELETE(hh, conns->h, conn_io);

            ev_timer_stop(loop, &conn_io->timer);

            quiche_conn_free(conn_io->conn);
            GX_DELETE(conn_io);
        }
    }
}

void qh3server::destroy_connection(struct ev_loop* loop, struct conn_io* conn_io) {
    HASH_DELETE(hh, conns->h, conn_io);
    ev_timer_stop(loop, &conn_io->timer);
    quiche_conn_free(conn_io->conn);
    GX_DELETE(conn_io);
}

void qh3server::timeout_cb(EV_P_ ev_timer* w, int revents) {
    UNUSED(revents);
    struct conn_io* conn_io = (struct conn_io*)w->data;
    quiche_conn_on_timeout(conn_io->conn);

    DEBUG_PRINT(LOG_LEVEL_3, __LOGTAG__, "timeout");

    conn_io->bridge->flush_egress(loop, conn_io);

    if (quiche_conn_is_closed(conn_io->conn)) {
        Stats stats;
        PathStats path_stats;

        quiche_conn_stats(conn_io->conn, &stats);
        quiche_conn_path_stats(conn_io->conn, 0, &path_stats);

        DEBUG_PRINT(LOG_LEVEL_3, __LOGTAG__, "connection closedA, recv=%zu sent=%zu lost=%zu rtt=%" PRIu64 "ns cwnd=%zu",
            stats.recv, stats.sent, stats.lost, path_stats.rtt, path_stats.cwnd);

        conn_io->bridge->destroy_connection(loop, conn_io);
        return;
    }
}

int qh3server::run(const std::string& host, const std::string& port, fs::path& rootDir) {
    const struct addrinfo hints = {
        .ai_family = PF_UNSPEC,
        .ai_socktype = SOCK_DGRAM,
        .ai_protocol = IPPROTO_UDP
    };

    GX_DELETE(logger);
    GX_DELETE(stats_logger);
    logger = DEBUG_NEW qtextfilelogger();
    stats_logger = DEBUG_NEW qstatslogger();

    qh3server::get_file_logger()->start_session("./logs/qh3_logfile", sizeof("./logs/qh3_logfile"));
    qh3server::get_stats_loggeer()->init(essentials::get_sysname(), essentials::get_device_name(), "", 0);
    qh3server::get_stats_loggeer()->start_session("./stats/qh3_statfile", sizeof("./stats/qh3_statfile"));
    quiche_enable_debug_logging(debug_log, this);

    if (is_log_quiche()) {
        DEBUG_PRINT_WARN(__LOGTAG__, "quiche log is enabled. Perfomance may get affected due to excess logs !!!");
    }
    struct addrinfo* local;
    if (getaddrinfo(host.c_str(), port.c_str(), &hints, &local) != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to resolve host");
        return -1;
    }

    int sock = socket(local->ai_family, SOCK_DGRAM, 0);
    if (sock < 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create socket");
        freeaddrinfo(local);
        return -1;
    }

    if (fcntl(sock, F_SETFL, O_NONBLOCK) != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to make socket non-blocking");
        freeaddrinfo(local);
        return -1;
    }

    if (bind(sock, local->ai_addr, local->ai_addrlen) < 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to connect socket");
        freeaddrinfo(local);
        return -1;
    }

    config = quiche_config_new(PROTOCOL_VERSION);
    if (config == NULL) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create config");
        freeaddrinfo(local);
        return -1;
    }


    fs::path certFile(rootDir / "cert.crt");
    fs::path keyFile(rootDir / "cert.key");
    DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "cert file %s, key file %s", certFile.c_str(), keyFile.c_str());
    int res_crt_load = quiche_config_load_cert_chain_from_pem_file(config, certFile.c_str());
    if (res_crt_load != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "CERT load error - %s", certFile.c_str());
    }
    int res_key_load = quiche_config_load_priv_key_from_pem_file(config, keyFile.c_str());
    if (res_key_load != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "KEY load error - %s", keyFile.c_str());
    }

    quiche_config_set_application_protos(config,
        (uint8_t*)QUICHE_H3_APPLICATION_PROTOCOL,
        sizeof(QUICHE_H3_APPLICATION_PROTOCOL) - 1);

    quiche_config_set_max_idle_timeout(config, 5000);
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
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create HTTP/3 config");
        freeaddrinfo(local);
        return -1;
    }

    struct connections c;
    c.sock = sock;
    c.h = NULL;
    c.local_addr = local->ai_addr;
    c.local_addr_len = local->ai_addrlen;
    c.server_port = port;
    c.quic_alternate_protocol_str = "quic:" + port;

    conns = &c;

    ev_io watcher;

    mainloop = ev_default_loop(0);

    ev_io_init(&watcher, recv_cb, sock, EV_READ);
    ev_io_start(mainloop, &watcher);
    watcher.data = this;

    on_run_started();
    ev_loop(mainloop, 0);
    on_run_end();

    // destroy connections
    struct conn_io* tmp, * conn_io = NULL;
    HASH_ITER(hh, conns->h, conn_io, tmp) {
        flush_egress(mainloop, conn_io);

        //if (quiche_conn_is_closed(conn_io->conn)) {
        Stats stats;
        PathStats path_stats;

        quiche_conn_stats(conn_io->conn, &stats);
        quiche_conn_path_stats(conn_io->conn, 0, &path_stats);

        DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "connection force closed, recv=%zu sent=%zu lost=%zu rtt=%" PRIu64 "ns cwnd=%zu",
            stats.recv, stats.sent, stats.lost, path_stats.rtt, path_stats.cwnd);

        HASH_DELETE(hh, conns->h, conn_io);

        ev_timer_stop(mainloop, &conn_io->timer);

        quiche_conn_free(conn_io->conn);
        GX_DELETE(conn_io);
        //}
    }
    //

    freeaddrinfo(local);

    quiche_h3_config_free(http3_config);

    quiche_config_free(config);

    qh3server::get_stats_loggeer()->end_session();
    qh3server::get_file_logger()->end_session();

    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "waiting for services to finish !!!");
    struct ev_loop* wait_loop = ev_loop_new();
    qtimer_sceduler wait_scheduler;
    wait_scheduler.set_ev_lopp(wait_loop);
    wait_scheduler.schedule_repeat_timer([wait_loop](qtimer& timer) {
        int service_shutdown_cnt = 0;
        if (qh3server::get_stats_loggeer()->config.finished) {
            DEBUG_PRINT_IMPORTANT(__LOGTAG__, "stats service finished !!!");
            service_shutdown_cnt++;
        }
        if (qh3server::get_file_logger()->config.finished) {
            DEBUG_PRINT_IMPORTANT(__LOGTAG__, "logger service finished !!!");
            service_shutdown_cnt++;
        }
        if (service_shutdown_cnt >= 2) {
            ev_break(wait_loop, EVBREAK_ONE);
        }
        }, 3);
    ev_run(wait_loop, 0);

    GX_DELETE(logger);
    GX_DELETE(stats_logger);
    return 0;
}
