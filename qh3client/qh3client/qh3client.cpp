//
//  qh3client.cpp
//  qh3client
//
//  Created by Arun A on 30/10/23.
//

#include "qh3client.hpp"
#include "../../common/sdktypes.hpp"

void qh3client::debug_log(const uint8_t *line, void *argp) {
    fprintf(stderr, "%s\n", line);
}

void qh3client::flush_egress(struct ev_loop *loop, struct conn_io *conn_io) {
    SendInfo send_info;

    while (1) {
        ssize_t written = quiche_conn_send(conn_io->conn, conn_io->out, sizeof(conn_io->out),
                                           &send_info);

        if (written == QUICHE_ERR_DONE) {
            fprintf(stderr, "done writing\n");
            break;
        }

        if (written < 0) {
            fprintf(stderr, "failed to create packet: %zd\n", written);
            return;
        }

        ssize_t sent = sendto(conn_io->sock, conn_io->out, written, 0,
                              (struct sockaddr *) &send_info.to,
                              send_info.to_len);

        if (sent != written) {
            perror("failed to send");
            return;
        }

        fprintf(stderr, "sent %zd bytes\n", sent);
    }

    double t = quiche_conn_timeout_as_nanos(conn_io->conn) / 1e9f;
    conn_io->timer.repeat = t;
    ev_timer_again(loop, &conn_io->timer);
}

int qh3client::for_each_setting(uint64_t identifier, uint64_t value,
                           void *argp) {
    fprintf(stderr, "got HTTP/3 SETTING: %" PRIu64 "=%" PRIu64 "\n",
            identifier, value);

    return 0;
}

/*
 int (*cb)(const uint8_t *name,
   size_t name_len,
   const uint8_t *value,
   size_t value_len,
   void *argp)
 */
int qh3client::for_each_header(const uint8_t *name, size_t name_len,
                           const uint8_t *value, size_t value_len,
                           void *argp) {
    fprintf(stderr, "got HTTP header: %.*s=%.*s\n",
            (int) name_len, name, (int) value_len, value);

    return 0;
}

int64_t qh3client::send_get_http_request(const getorpost_reqdata& data_getorpost_, struct conn_io *conn_io) {
    Header headers_get[] = {
        {
            .name = (uint8_t *) ":method",
            .name_len = sizeof(":method") - 1,

            .value = (uint8_t *) "GET",
            .value_len = sizeof("GET") - 1,
        },

        {
            .name = (uint8_t *) ":scheme",
            .name_len = sizeof(":scheme") - 1,

            .value = (uint8_t *) "https",
            .value_len = sizeof("https") - 1,
        },

        {
            .name = (uint8_t *) ":authority",
            .name_len = sizeof(":authority") - 1,

            .value = (uint8_t *) conn_io->host,
            .value_len = strlen(conn_io->host),
        },

        {
            .name = (uint8_t *) ":path",
            .name_len = sizeof(":path") - 1,

            .value = (uint8_t *) data_getorpost_.path.c_str(),
            .value_len = data_getorpost_.path.size(),
        },

        {
            .name = (uint8_t *) "user-agent",
            .name_len = sizeof("user-agent") - 1,

            .value = (uint8_t *) "quiche",
            .value_len = sizeof("quiche") - 1,
        },
    };

    int64_t stream_id = quiche_h3_send_request(conn_io->http3,
                                               conn_io->conn,
                                               headers_get, 5, true);

    fprintf(stderr, "sent HTTP GET request %" PRId64 "\n", stream_id);
    return stream_id;
}

int64_t qh3client::send_post_http_request(const getorpost_reqdata& data_getorpost_, struct conn_io *conn_io) {
    int number_of_digits = NumberOfDigits((int)data_getorpost_.payload.size());
    char content_length_data[number_of_digits+1];
    snprintf(content_length_data, sizeof(content_length_data), "%d", (int)data_getorpost_.payload.size());
    Header headers_get[] = {
        {
            .name = (uint8_t *) ":method",
            .name_len = sizeof(":method") - 1,

            .value = (uint8_t *) "POST",
            .value_len = sizeof("POST") - 1,
        },

        {
            .name = (uint8_t *) ":scheme",
            .name_len = sizeof(":scheme") - 1,

            .value = (uint8_t *) "https",
            .value_len = sizeof("https") - 1,
        },

        {
            .name = (uint8_t *) ":authority",
            .name_len = sizeof(":authority") - 1,

            .value = (uint8_t *) conn_io->host,
            .value_len = strlen(conn_io->host),
        },

        {
            .name = (uint8_t *) ":path",
            .name_len = sizeof(":path") - 1,

            .value = (uint8_t *) data_getorpost_.path.c_str(),
            .value_len = data_getorpost_.path.size(),
        },

        {
            .name = (uint8_t *) "user-agent",
            .name_len = sizeof("user-agent") - 1,

            .value = (uint8_t *) "quiche",
            .value_len = sizeof("quiche") - 1,
        },
        {
            .name = (uint8_t *) "content-length",
            .name_len = sizeof("content-length") - 1,

            .value = (uint8_t *) content_length_data,
            .value_len = sizeof(content_length_data) - 1,
        },
    };

    int64_t stream_id = quiche_h3_send_request(conn_io->http3,
                                               conn_io->conn,
                                               headers_get, 6, false);
    ssize_t send_len = quiche_h3_send_body(conn_io->http3, conn_io->conn, stream_id,
                                           (u_int8_t*)data_getorpost_.payload.c_str(), data_getorpost_.payload.size(),
                                           true);
    fprintf(stderr, "sent HTTP POST request %" PRId64 " with body %ld\n", stream_id, send_len);
    return stream_id;
}

void qh3client::recv_cb(EV_P_ ev_io *w, int revents) {
    struct conn_io *conn_io = (struct conn_io *)w->data;

    while (1) {
        struct sockaddr_storage peer_addr;
        socklen_t peer_addr_len = sizeof(peer_addr);
        memset(&peer_addr, 0, peer_addr_len);

        ssize_t read = recvfrom(conn_io->sock, conn_io->buf, sizeof(conn_io->buf), 0,
                                (struct sockaddr *) &peer_addr,
                                &peer_addr_len);

        if (read < 0) {
            if ((errno == EWOULDBLOCK) || (errno == EAGAIN)) {
                fprintf(stderr, "recv would block\n");
                break;
            }

            perror("failed to read");
            return;
        }

        RecvInfo recv_info = {
            (struct sockaddr *) &peer_addr,
            peer_addr_len,

            (struct sockaddr *) &conn_io->local_addr,
            conn_io->local_addr_len,
        };

        ssize_t done = quiche_conn_recv(conn_io->conn, conn_io->buf, read, &recv_info);

        if (done < 0) {
            fprintf(stderr, "failed to process packet: %zd\n", done);
            continue;
        }

        fprintf(stderr, "recv %zd bytes\n", done);
    }

    fprintf(stderr, "done reading\n");

    if (quiche_conn_is_closed(conn_io->conn)) {
        fprintf(stderr, "connection closed\n");

        ev_break(EV_A_ EVBREAK_ONE);
        return;
    }

    if (quiche_conn_is_established(conn_io->conn) && !conn_io->req_sent) {
        const uint8_t *app_proto;
        size_t app_proto_len;

        quiche_conn_application_proto(conn_io->conn, &app_proto, &app_proto_len);

        fprintf(stderr, "connection established: %.*s\n",
                (int) app_proto_len, app_proto);

        Config *config = quiche_h3_config_new();
        if (config == NULL) {
            fprintf(stderr, "failed to create HTTP/3 config\n");
            return;
        }

        conn_io->http3 = quiche_h3_conn_new_with_transport(conn_io->conn, config);
        if (conn_io->http3 == NULL) {
            fprintf(stderr, "failed to create HTTP/3 connection\n");
            return;
        }

        quiche_h3_config_free(config);

        const getorpost_reqdata& data_getorpost_ = conn_io->bridge->get_getorpost_http_request();
        if (data_getorpost_.is_postrequest()) {
            conn_io->bridge->send_post_http_request(data_getorpost_, conn_io);
        } else {
            conn_io->bridge->send_get_http_request(data_getorpost_, conn_io);
        }
        conn_io->req_sent = true;
    }

    if (quiche_conn_is_established(conn_io->conn)) {
        Event *ev;

        while (1) {
            int64_t s = quiche_h3_conn_poll(conn_io->http3,
                                            conn_io->conn,
                                            (const struct Event **)&ev);

            if (s < 0) {
                break;
            }

            if (!conn_io->settings_received) {
                int rc = quiche_h3_for_each_setting(conn_io->http3,
                                                    for_each_setting,
                                                    NULL);

                if (rc == 0) {
                    conn_io->settings_received = true;
                }
            }

            switch (quiche_h3_event_type(ev)) {
                case Event_type::Headers: {
                    int rc = quiche_h3_event_for_each_header((const struct Event *)ev, for_each_header,
                                                             NULL);

                    if (rc != 0) {
                        fprintf(stderr, "failed to process headers");
                    }

                    break;
                }

                case Event_type::Data: {
                    for (;;) {
                        ssize_t len = quiche_h3_recv_body(conn_io->http3,
                                                          conn_io->conn, s,
                                                          conn_io->buf, sizeof(conn_io->buf));

                        if (len <= 0) {
                            break;
                        }

                        printf("%.*s", (int) len, conn_io->buf);
                        if (conn_io->response) {
                            conn_io->response->push_back(conn_io_response(conn_io->buf, len));
                        }
                    }

                    conn_io->res_received = true;
                    break;
                }

                case Event_type::Finished:
                    if (quiche_conn_close(conn_io->conn, true, 0, NULL, 0) < 0) {
                        fprintf(stderr, "failed to close connection\n");
                    }
                    break;

                case Event_type::Reset:
                    fprintf(stderr, "request was reset\n");

                    if (quiche_conn_close(conn_io->conn, true, 0, NULL, 0) < 0) {
                        fprintf(stderr, "failed to close connection\n");
                    }
                    break;

                case Event_type::PriorityUpdate:
                    break;

                case Event_type::GoAway: {
                    fprintf(stderr, "got GOAWAY\n");
                    break;
                }
            }

            quiche_h3_event_free(ev);
        }
    }

    conn_io->bridge->flush_egress(loop, conn_io);
}

void qh3client::timeout_cb(EV_P_ ev_timer *w, int revents) {
    struct conn_io *conn_io = (struct conn_io *)w->data;
    quiche_conn_on_timeout(conn_io->conn);

    fprintf(stderr, "timeout\n");

    conn_io->bridge->flush_egress(loop, conn_io);

    if (quiche_conn_is_closed(conn_io->conn)) {
        Stats stats;
        PathStats path_stats;

        quiche_conn_stats(conn_io->conn, &stats);
        quiche_conn_path_stats(conn_io->conn, 0, &path_stats);

        fprintf(stderr, "connection closed, recv=%zu sent=%zu lost=%zu rtt=%" PRIu64 "ns\n",
                stats.recv, stats.sent, stats.lost, path_stats.rtt);

        ev_break(EV_A_ EVBREAK_ONE);
        return;
    }
}

qh3client::qh3client(const std::string& host, const std::string& port) :
host(host),
port(port) {
}

qh3client::~qh3client() {
    GX_DELETE(conn_io);
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "qh3client destroyed");
}

int qh3client::send_request(const getorpost_reqdata& data_get_, std::vector<conn_io_response>* response) {
    this->http_request = data_get_;
    
    const struct addrinfo hints = {
        .ai_family = PF_UNSPEC,
        .ai_socktype = SOCK_DGRAM,
        .ai_protocol = IPPROTO_UDP
    };

//    quiche_enable_debug_logging(debug_log, NULL);

    struct addrinfo *peer;
    if (getaddrinfo(host.c_str(), port.c_str(), &hints, &peer) != 0) {
        perror("failed to resolve host");
        return -1;
    }

    int sock = socket(peer->ai_family, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("failed to create socket");
        return -1;
    }

    if (fcntl(sock, F_SETFL, O_NONBLOCK) != 0) {
        perror("failed to make socket non-blocking");
        return -1;
    }

    Config *config = quiche_config_new(0xbabababa);
    if (config == NULL) {
        fprintf(stderr, "failed to create config\n");
        return -1;
    }

    quiche_config_set_application_protos(config,
        (uint8_t *) QUICHE_H3_APPLICATION_PROTOCOL,
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

    if (getenv("SSLKEYLOGFILE")) {
      quiche_config_log_keys(config);
    }

    // ABC: old config creation here

    uint8_t scid[LOCAL_CONN_ID_LEN];
    int rng = open("/dev/urandom", O_RDONLY);
    if (rng < 0) {
        perror("failed to open /dev/urandom");
        return -1;
    }

    ssize_t rand_len = read(rng, &scid, sizeof(scid));
    if (rand_len < 0) {
        close(rng);
        perror("failed to create connection ID");
        return -1;
    }
    close(rng);

    conn_io = new struct conn_io();
    if (conn_io == NULL) {
        fprintf(stderr, "failed to allocate connection IO\n");
        return -1;
    }
    conn_io->response = response;
    
    conn_io->local_addr_len = sizeof(conn_io->local_addr);
    if (getsockname(sock, (struct sockaddr *)&conn_io->local_addr,
                    &conn_io->local_addr_len) != 0)
    {
        perror("failed to get local address of socket");
        return -1;
    };

    Connection *conn = quiche_connect(host.c_str(), (const uint8_t *) scid, sizeof(scid),
                                       (struct sockaddr *) &conn_io->local_addr,
                                       conn_io->local_addr_len,
                                       peer->ai_addr, peer->ai_addrlen, config);

    if (conn == NULL) {
        fprintf(stderr, "failed to create connection\n");
        return -1;
    }

    conn_io->sock = sock;
    conn_io->conn = conn;
    conn_io->host = host.c_str();
    conn_io->bridge = this;

    ev_io watcher;

    mainloop = ev_loop_new(0);

    ev_io_init(&watcher, recv_cb, conn_io->sock, EV_READ);
    ev_io_start(mainloop, &watcher);
    watcher.data = conn_io;

    ev_init(&conn_io->timer, timeout_cb);
    conn_io->timer.data = conn_io;

    flush_egress(mainloop, conn_io);

    ev_loop(mainloop, 0);

    ev_timer_stop(mainloop, &conn_io->timer);
    ev_io_stop(mainloop, &watcher);
    
    freeaddrinfo(peer);

    if (conn_io->http3) {
        quiche_h3_conn_free(conn_io->http3);
    }

    quiche_conn_free(conn);

    quiche_config_free(config);

    return 0;
}
