//
//  QNetworkServer.cpp
//  NetworkServer
//
//  Created by Arun A on 12/10/23.
//

#include "QNetworkServer.hpp"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

#include <fcntl.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <ev.h>
#include <uthash.h>

#include <quiche.h>

QPeerConnection::QPeerConnection(MQPeerConnectionBridge* bridge, uint8_t *scid, size_t scid_len, int sock) :
bridge(bridge),
sock(sock)
{
    if (scid_len != LOCAL_CONN_ID_LEN) {
        DEBUG_PRINT_WARN(__LOGTAG__, "failed, scid length too short");
    }

    memcpy(cid, scid, LOCAL_CONN_ID_LEN);
}

QPeerConnection::~QPeerConnection() {
    ev_timer_stop(bridge->GetMainLoop(), &timer);
    if (conn) {
        quiche_conn_free(conn);
    }
}

void QPeerConnection::SendMessage(const std::string& buffer, bool flush) {
    SendMessage(buffer.c_str(), buffer.size(), flush);
}

void QPeerConnection::SendMessage(const char *buf, size_t buflen, bool flush) {
    if (!quiche_conn_is_established(conn)) {
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "Cant send !!!, connection not established - ", (char*)buf);
        return;
    }
    
    uint64_t s = 0;
    StreamIter *writable = quiche_conn_writable(conn);

    while (quiche_stream_iter_next(writable, &s)) {
        DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "stream %" PRIu64 " is writable", s);

        ssize_t sent_len = quiche_conn_stream_send(conn, s, (uint8_t *) buf,
                                buflen, false);
        if (sent_len!=buflen) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "send failure %d", sent_len);
            break;
        }
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "--------->>>>>>>>>>>[%d] %s", s, (char*)buf);
        break;
    }

    quiche_stream_iter_free(writable);
    if (flush) {
        bridge->FlushEgress(bridge->GetMainLoop(), this);
    }
}

void QNetworkServer::debug_log(const uint8_t *line, void *argp) {
    DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "%s", (char*)line);
}
void QNetworkServer::mint_token(const uint8_t *dcid, size_t dcid_len,
                       struct sockaddr_storage *addr, socklen_t addr_len,
                       uint8_t *token, size_t *token_len) {
    memcpy(token, "quiche", sizeof("quiche") - 1);
    memcpy(token + sizeof("quiche") - 1, addr, addr_len);
    memcpy(token + sizeof("quiche") - 1 + addr_len, dcid, dcid_len);

    *token_len = sizeof("quiche") - 1 + addr_len + dcid_len;
}

bool QNetworkServer::validate_token(const uint8_t *token, size_t token_len,
                           struct sockaddr_storage *addr, socklen_t addr_len,
                           uint8_t *odcid, size_t *odcid_len) {
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

uint8_t *QNetworkServer::gen_cid(uint8_t *cid, size_t cid_len) {
    int rng = open("/dev/urandom", O_RDONLY);
    if (rng < 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to open /dev/urandom");
        return nullptr;
    }

    ssize_t rand_len = read(rng, cid, cid_len);
    if (rand_len < 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create connection ID");
        return nullptr;
    }

    return cid;
}

QPeerConnection *QNetworkServer::create_conn(uint8_t *scid, size_t scid_len,
                                   uint8_t *odcid, size_t odcid_len,
                                   struct sockaddr *local_addr,
                                   socklen_t local_addr_len,
                                   struct sockaddr_storage *peer_addr,
                                   socklen_t peer_addr_len)
{
    QPeerConnection* qconnection = new QPeerConnection(this, scid, scid_len, conns->sock);
    if (qconnection == nullptr) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to allocate qconnection");
        GX_DELETE(qconnection);
        return nullptr;
    }

    Connection *conn = quiche_accept(qconnection->cid, LOCAL_CONN_ID_LEN,
                                      odcid, odcid_len,
                                      local_addr,
                                      local_addr_len,
                                      (struct sockaddr *) peer_addr,
                                      peer_addr_len,
                                      config);

    if (conn == nullptr) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create connection");
        GX_DELETE(qconnection);
        return nullptr;
    }

    qconnection->conn = conn;

    memcpy(&qconnection->peer_addr, peer_addr, peer_addr_len);
    qconnection->peer_addr_len = peer_addr_len;
    
    ev_init(&qconnection->timer, timeout_cb);
    qconnection->timer.data = qconnection;

    HASH_ADD(hh, conns->h, cid, LOCAL_CONN_ID_LEN, qconnection);

    qconnection->bridge->OnConnection(qconnection);

    return qconnection;
}

void QNetworkServer::OnConnection(QPeerConnection* qconnection) {
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "++++++++++<<<<<<<<<<< new connection");
}

void QNetworkServer::OnMessage(ssize_t recv_len, uint8_t* buf, QPeerConnection* qconnection) {
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

    uint8_t* copybuf = new uint8_t[recv_len+1];
    memcpy(copybuf, buf, recv_len);
    copybuf[recv_len] = '\0';
    if (getnameinfo((struct sockaddr *) &qconnection->peer_addr, qconnection->peer_addr_len, hbuf, sizeof(hbuf), sbuf,
                    sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
        DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "---------<<<<<<<<<<< %s [len %d], host : %s, serv : %s", copybuf, recv_len, hbuf, sbuf);
    } else {
        DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "---------<<<<<<<<<<< %s [len %d]", copybuf, recv_len);
    }
    GX_DELETE_ARY(copybuf);
    
    qconnection->SendMessage("HELLO from server", true);
//    qconnection->SendMessage("hello12 from server", true);
//    qconnection->SendMessage("hello123 from server", true);
//    qconnection->SendMessage("hello1234 from server", true);
}

void QNetworkServer::FlushEgress(struct ev_loop *loop, QPeerConnection* qconnection) {
    static uint8_t out[MAX_DATAGRAM_SIZE];

    SendInfo send_info;

    while (1) {
        ssize_t written = quiche_conn_send(qconnection->conn, out, sizeof(out),
                                           &send_info);

        if (written == QUICHE_ERR_DONE) {
            DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "done writing");
            break;
        }

        if (written < 0) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create packet: %zd", written);
            return;
        }

        ssize_t sent = sendto(qconnection->sock, out, written, 0,
                              (struct sockaddr *) &send_info.to,
                              send_info.to_len);

        if (sent != written) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to send");
            return;
        }

        DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "sent %zd bytes", sent);
    }

    uint64_t timeout_in_nanos = quiche_conn_timeout_as_nanos(qconnection->conn);
    double t = (double)timeout_in_nanos / 1e9f;
    qconnection->timer.repeat = t<0.00001f ? 1.0f : t;
    ev_timer_again(loop, &qconnection->timer);
    DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "qconnection->timer.repeat %f - %" PRIu64 "", t, timeout_in_nanos);
}

void QNetworkServer::DestroyConnection(struct ev_loop *loop, QPeerConnection* qconnection) {
    OnDestroyConnection(qconnection);
    HASH_DELETE(hh, conns->h, qconnection);
    GX_DELETE(qconnection);
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "Connection destroyed !!!");
}

void QNetworkServer::OnDestroyConnection(QPeerConnection* qconnection) {
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "Connection about to destroy !!!");
}

void QNetworkServer::timeout_cb(EV_P_ ev_timer *w, int revents) {
    QPeerConnection* qconnection = (QPeerConnection*)w->data;
        
    quiche_conn_on_timeout(qconnection->conn);

    DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "timeout !!!");
    
    qconnection->bridge->FlushEgress(loop, qconnection);

    if (quiche_conn_is_closed(qconnection->conn)) {
        Stats stats;
        PathStats path_stats;

        quiche_conn_stats(qconnection->conn, &stats);
        quiche_conn_path_stats(qconnection->conn, 0, &path_stats);

        DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "connection closed, recv=%zu sent=%zu lost=%zu rtt=%" PRIu64 "ns cwnd=%zu\n",
                stats.recv, stats.sent, stats.lost, path_stats.rtt, path_stats.cwnd);

        qconnection->bridge->DestroyConnection(loop, qconnection);
        return;
    } /*else {
        
        // force close here
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "Force close connection !!!");
        qconnection->bridge->DestroyConnection(loop, qconnection);
        return;
    }*/
}

void QNetworkServer::Recv_cb(EV_P_ ev_io *w, int revents) {
    QPeerConnection* qconnection = nullptr;
    QPeerConnection* tmp = nullptr;
    
    static uint8_t buf[65535];
    static uint8_t out[MAX_DATAGRAM_SIZE];

    while (1) {
        struct sockaddr_storage peer_addr;
        socklen_t peer_addr_len = sizeof(peer_addr);
        memset(&peer_addr, 0, peer_addr_len);

        ssize_t read = recvfrom(conns->sock, buf, sizeof(buf), 0,
                                (struct sockaddr *) &peer_addr,
                                &peer_addr_len);

        if (read < 0) {
            if ((errno == EWOULDBLOCK) || (errno == EAGAIN)) {
                DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "recv would block");
                break;
            }

            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to read");
            return;
        }

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
            continue;
        }

        HASH_FIND(hh, conns->h, dcid, dcid_len, qconnection);

        if (qconnection == nullptr) {
            if (!quiche_version_is_supported(version)) {
                DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "version negotiation");

                ssize_t written = quiche_negotiate_version(scid, scid_len,
                                                           dcid, dcid_len,
                                                           out, sizeof(out));

                if (written < 0) {
                    DEBUG_PRINT_WARN(__LOGTAG__, "failed to create vneg packet: %zd",
                            written);
                    continue;
                }

                ssize_t sent = sendto(conns->sock, out, written, 0,
                                      (struct sockaddr *) &peer_addr,
                                      peer_addr_len);
                if (sent != written) {
                    DEBUG_PRINT_ERROR(__LOGTAG__, "failed to send");
                    continue;
                }

                DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "sent %zd bytes", sent);
                continue;
            }

            if (token_len == 0) {
                DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "stateless retry");

                mint_token(dcid, dcid_len, &peer_addr, peer_addr_len,
                           token, &token_len);

                uint8_t new_cid[LOCAL_CONN_ID_LEN];

                if (gen_cid(new_cid, LOCAL_CONN_ID_LEN) == nullptr) {
                    continue;
                }

                ssize_t written = quiche_retry(scid, scid_len,
                                               dcid, dcid_len,
                                               new_cid, LOCAL_CONN_ID_LEN,
                                               token, token_len,
                                               version, out, sizeof(out));

                if (written < 0) {
                    DEBUG_PRINT_WARN(__LOGTAG__, "failed to create retry packet: %zd",
                            written);
                    continue;
                }

                ssize_t sent = sendto(conns->sock, out, written, 0,
                                      (struct sockaddr *) &peer_addr,
                                      peer_addr_len);
                if (sent != written) {
                    DEBUG_PRINT_ERROR(__LOGTAG__, "failed to send");
                    continue;
                }

                DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "sent %zd bytes", sent);
                continue;
            }


            if (!validate_token(token, token_len, &peer_addr, peer_addr_len,
                               odcid, &odcid_len)) {
                DEBUG_PRINT_WARN(__LOGTAG__, "invalid address validation token");
                continue;
            }

            qconnection = create_conn(dcid, dcid_len, odcid, odcid_len,
                                  conns->local_addr, conns->local_addr_len,
                                  &peer_addr, peer_addr_len);

            if (qconnection == nullptr) {
                continue;
            }
        }

        RecvInfo recv_info = {
            (struct sockaddr *)&peer_addr,
            peer_addr_len,

            conns->local_addr,
            conns->local_addr_len,
        };
        
        ssize_t done = quiche_conn_recv(qconnection->conn, buf, read, &recv_info);

        if (done < 0) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to process packet: %zd", done);
            continue;
        }

        DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "recv %zd bytes", done);

        if (quiche_conn_is_established(qconnection->conn)) {
            uint64_t s = 0;

            StreamIter *readable = quiche_conn_readable(qconnection->conn);

            while (quiche_stream_iter_next(readable, &s)) {
                DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "stream %" PRIu64 " is readable", s);

                bool fin = false;
                ssize_t recv_len = quiche_conn_stream_recv(qconnection->conn, s,
                                                           buf, sizeof(buf),
                                                           &fin);
                if (recv_len < 0) {
                    break;
                }

                if (fin /*|| true*/) {
                    static const char *resp = "byez\n";
                    ssize_t bye_sent_len = quiche_conn_stream_send(qconnection->conn, s, (uint8_t *) resp,
                                            5, true);
                    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "fin received, sending 'byez'");
                    if (bye_sent_len!=5) {
                        DEBUG_PRINT_ERROR(__LOGTAG__, "sending 'byez' failed !!!");
                    }
                }
                
//                DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "\n\nREACHED ---> %s", buf);
                qconnection->bridge->OnMessage(recv_len, buf, qconnection);
            }

            quiche_stream_iter_free(readable);
        }
    }

    HASH_ITER(hh, conns->h, qconnection, tmp) {
        FlushEgress(loop, qconnection);

        if (quiche_conn_is_closed(qconnection->conn)) {
            Stats stats;
            PathStats path_stats;

            quiche_conn_stats(qconnection->conn, &stats);
            quiche_conn_path_stats(qconnection->conn, 0, &path_stats);

            DEBUG_PRINT_IMPORTANT(__LOGTAG__, "connection closed, recv=%zu sent=%zu lost=%zu rtt=%" PRIu64 "ns cwnd=%zu",
                    stats.recv, stats.sent, stats.lost, path_stats.rtt, path_stats.cwnd);

            DestroyConnection(loop, qconnection);
        }
    }
}

void QNetworkServer::BroadCastMessage(const std::string& buffer, bool flush) {
    QPeerConnection* qconnection = nullptr;
    QPeerConnection* tmp = nullptr;
    HASH_ITER(hh, conns->h, qconnection, tmp) {
        if (quiche_conn_is_established(qconnection->conn)) {
            qconnection->SendMessage(buffer, flush);
        }
    }
}

void QNetworkServer::recv_cb(EV_P_ ev_io *w, int revents) {
    QNetworkServer* server = (QNetworkServer*)w->data;
    server->Recv_cb(loop, w, revents);
}

int QNetworkServer::run(std::string host, std::string port, fs::path rootDir) {
    const struct addrinfo hints = {
        .ai_family = PF_UNSPEC,
        .ai_socktype = SOCK_DGRAM,
        .ai_protocol = IPPROTO_UDP
    };

      quiche_enable_debug_logging(debug_log, nullptr);

    struct addrinfo *local;
    if (getaddrinfo(host.c_str(), port.c_str(), &hints, &local) != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to resolve host");
        return -1;
    }

    int sock = socket(local->ai_family, SOCK_DGRAM, 0);
    if (sock < 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create socket");
        return -1;
    }

    if (fcntl(sock, F_SETFL, O_NONBLOCK) != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to make socket non-blocking");
        return -1;
    }

    if (bind(sock, local->ai_addr, local->ai_addrlen) < 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to connect socket");
        return -1;
    }

    config = quiche_config_new(PROTOCOL_VERSION);
    if (config == nullptr) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create config");
        return -1;
    }

    //std::string rootDir = executablePath.parent_path();
    fs::path certFile("cert.crt");
    fs::path keyFile("cert.key");
    DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "cert file %s", (rootDir / certFile).c_str());
    quiche_config_load_cert_chain_from_pem_file(config, (rootDir / certFile).c_str());
    quiche_config_load_priv_key_from_pem_file(config, (rootDir / keyFile).c_str());

    quiche_config_set_application_protos(config,
        (uint8_t *) "\x0ahq-interop\x05hq-29\x05hq-28\x05hq-27\x08http/0.9", 38);

    quiche_config_set_max_idle_timeout(config, 30000);
    quiche_config_set_max_recv_udp_payload_size(config, MAX_DATAGRAM_SIZE);
    quiche_config_set_max_send_udp_payload_size(config, MAX_DATAGRAM_SIZE);
    quiche_config_set_initial_max_data(config, 10000000);
    quiche_config_set_initial_max_stream_data_bidi_local(config, 1000000);
    quiche_config_set_initial_max_stream_data_bidi_remote(config, 1000000);
    quiche_config_set_initial_max_streams_bidi(config, 100);
    quiche_config_set_cc_algorithm(config, Reno);

    struct connections c;
    c.sock = sock;
    c.h = nullptr;
    c.local_addr = local->ai_addr;
    c.local_addr_len = local->ai_addrlen;

    conns = &c;

    ev_io watcher;

    mainloop = ev_default_loop(0);

    ev_io_init(&watcher, recv_cb, sock, EV_READ);
    ev_io_start(mainloop, &watcher);
    watcher.data = this;

    ev_loop(mainloop, 0);

    freeaddrinfo(local);

    quiche_config_free(config);

    return 0;
}
