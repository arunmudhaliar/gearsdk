//
//  QNetworkClient.cpp
//  NetworkClient
//
//  Created by Arun A on 12/10/23.
//

#include "QNetworkClient.hpp"

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

#include <pthread.h>

QConnection::QConnection(MQCommandBridge* bridge, Config *config) :
bridge(bridge),
config(config) {
}

QConnection::~QConnection() {
    if (peer) {
        freeaddrinfo(peer);
        peer = nullptr;
    }
    ev_timer_stop(bridge->GetMainLoop(), &timer);
    if (conn) {
        quiche_conn_free(conn);
        conn = nullptr;
    }
    bridge = nullptr;
    config = nullptr;
}

void QConnection::SetState(CON_STATE state) {
    if (this->state>=state) {
        DEBUG_PRINT_WARN(__LOGTAG__, "QConnection state >= state, this->state %d, incoming state %d", this->state, state);
    }
    this->state = state;
}

int QConnection::Connect(std::string host, std::string port) {
    const struct addrinfo hints = {
        .ai_family = PF_UNSPEC,
        .ai_socktype = SOCK_DGRAM,
        .ai_protocol = IPPROTO_UDP
    };
    
    if (peer) {
        freeaddrinfo(peer);
        peer = nullptr;
    }
    if (getaddrinfo(host.c_str(), port.c_str(), &hints, &peer) != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to resolve host");
        return -1;
    }

    sock = socket(peer->ai_family, SOCK_DGRAM, 0);
    if (sock < 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create socket");
        return -1;
    }

    if (fcntl(sock, F_SETFL, O_NONBLOCK) != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to make socket non-blocking");
        return -1;
    }
    
    uint8_t scid[LOCAL_CONN_ID_LEN];
    int rng = open("/dev/urandom", O_RDONLY);
    if (rng < 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to open /dev/urandom");
        return -1;
    }

    ssize_t rand_len = read(rng, &scid, sizeof(scid));
    if (rand_len < 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create connection ID");
        return -1;
    }

    local_addr_len = sizeof(local_addr);
    if (getsockname(sock, (struct sockaddr *)&local_addr,
                    &local_addr_len) != 0)
    {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to get local address of socket");
        return -1;
    };

    conn = quiche_connect(host.c_str(), (const uint8_t *) scid, sizeof(scid),
                                       (struct sockaddr *) &local_addr,
                                       local_addr_len,
                                       peer->ai_addr, peer->ai_addrlen, config);

    if (conn == NULL) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create connection");
        return -1;
    }
    
    return 0;
}

int QConnection::ConnectionActive() {
    if (!quiche_conn_is_established(conn)) {
        return -1;
    }

    if (quiche_conn_is_closed(conn)) {
        return -2;
    }
    
    return 0;
}

int QConnection::SendMessage(const std::string& buffer, bool flush) {
    return SendMessage(buffer.c_str(), buffer.size(), flush);
}

int QConnection::SendMessage(const char *buf, size_t buflen, bool flush) {
    if (state == STATE_CLOSE) {
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "Cant send !!!, connection in CLOSE state. May be in destruction event !!!", (char*)buf);
        return -1;
    }
    if (!quiche_conn_is_established(conn)) {
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "Cant send !!!, connection not established - ", (char*)buf);
        return -2;
    }
    
    if (quiche_conn_is_closed(conn)) {
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "Cant send !!!, connection closed - ", (char*)buf);
        return -3;
    }
    
    uint64_t s = 0;
    StreamIter *writable = quiche_conn_writable(conn);

    int result = -4;
    while (quiche_stream_iter_next(writable, &s)) {
        DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "stream %" PRIu64 " is writable", s);

        ssize_t sent_len = quiche_conn_stream_send(conn, s, (uint8_t *) buf,
                                buflen, false);
        if (sent_len!=buflen) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "send failure %d", sent_len);
            break;
        }
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "--------->>>>>>>>>>> %s", (char*)buf);
        result = 0;
        break;
    }

    quiche_stream_iter_free(writable);
    if (flush) {
        bridge->FlushEgress(bridge->GetMainLoop(), this);
    }
    return result;
}

void QNetworkClient::debug_log(const uint8_t *line, void *argp) {
    // DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "%s", (char*)line);
}

void QNetworkClient::FlushEgress(struct ev_loop *loop, QConnection* qconnection) {
    SendInfo send_info;

    while (1) {
        ssize_t written = quiche_conn_send(qconnection->conn, qconnection->egress_out, sizeof(qconnection->egress_out),
                                           &send_info);

        if (written == QUICHE_ERR_DONE) {
            DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "done writing");
            break;
        }

        if (written < 0) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create packet: %zd", written);
            return;
        }

        ssize_t sent = sendto(qconnection->sock, qconnection->egress_out, written, 0,
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
    qconnection->timer.repeat = t;
    ev_timer_again(loop, &qconnection->timer);
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "qconnection->timer.repeat %f - %" PRIu64 "", t, timeout_in_nanos);
}

void QNetworkClient::Event_Connect(QConnection* qconnection) {
//    qconnection->SetState(QConnection::STATE_CONNECTING);
//    if(pthread_mutex_trylock(&conn_mutex)!=0) {
//        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to acuire lock(conn_mutex), Connect returning...");
//        return;
//    }
    qconnection->SetState(QConnection::STATE_CONNECT);
    OnConnect(qconnection);
//    if (pthread_mutex_unlock(&conn_mutex) != 0) {
//        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to unlock conn_mutex, Connect !!!");
//    }
}

void QNetworkClient::Event_MsgReceived(ssize_t recv_len, uint8_t* buf, QConnection* qconnection) {
//    if(pthread_mutex_trylock(&conn_mutex)!=0) {
//        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to acuire lock(conn_mutex), MessageReceived returning...");
//        return;
//    }
    
//    if(pthread_mutex_trylock(&recv_mutex)!=0) {
//        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to acuire lock(recv_mutex), MessageReceived returning...");
//        return;
//    }
    OnMessage(recv_len, buf, qconnection);
//    if (pthread_mutex_unlock(&recv_mutex) != 0) {
//        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to unlock recv_mutex, MessageReceived !!!");
//    }
    
//    if (pthread_mutex_unlock(&conn_mutex) != 0) {
//        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to unlock conn_mutex, MessageReceived !!!");
//    }
}

void QNetworkClient::Event_Close(QConnection* qconnection) {
//    if(pthread_mutex_trylock(&conn_mutex)!=0) {
//        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to acuire lock(conn_mutex), Close returning...");
//        return;
//    }
    if (qconnection->IsClosed()) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "Connection already closed, Event_Close returning...");
        return;
    }
    qconnection->SetState(QConnection::STATE_CLOSE);
    
    OnClose(qconnection);
//    if (pthread_mutex_unlock(&conn_mutex) != 0) {
//        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to unlock conn_mutex, Close !!!");
//    }
}

void QNetworkClient::DestroyConnection(struct ev_loop *loop, QConnection* qconnection) {
//    if(pthread_mutex_trylock(&conn_mutex)!=0) {
//        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to acuire lock(conn_mutex), DestroyConnection returning...");
//        return;
//    }
    OnDestroyConnection(qconnection);
    GX_DELETE(qconnection);
    
//    if (pthread_mutex_unlock(&conn_mutex) != 0) {
//        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to unlock conn_mutex, DestroyConnection !!!");
//    }
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "Connection destroyed !!!");
}

int QNetworkClient::Close() {
    if (qclientConnection == nullptr) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "qclientConnection == null, Close returning...");
        return -9;
    }
    int conActive =  qclientConnection->ConnectionActive();
    if (conActive<0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "connection not active %d, Close returning...", conActive);
        return conActive;
    }
    
    if(pthread_mutex_trylock(&close_mutex)!=0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to acuire lock(close_mutex), SendMessage returning...");
        return -8;
    }
    
    if (qclientConnection) {
//        qclientConnection->br(buffer.c_str(), buffer.size(), flush);
        if (quiche_conn_close(qclientConnection->conn, true, 0, NULL, 0) < 0) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to close connection - Close");
        }
    }
    
    if (pthread_mutex_unlock(&close_mutex) != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to unlock close_mutex, Close !!!");
    }
    
    return 0;
}

int QNetworkClient::SendMessage(const std::string& buffer, bool flush) {
//    if(pthread_mutex_trylock(&conn_mutex)!=0) {
//        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to acuire lock(conn_mutex), SendMessage returning...");
//        return -1;
//    }
    
    if(pthread_mutex_trylock(&send_mutex)!=0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to acuire lock(send_mutex), SendMessage returning...");
        return -9;
    }
    
    if (qclientConnection) {
        qclientConnection->SendMessage(buffer.c_str(), buffer.size(), flush);
    }
    
    if (pthread_mutex_unlock(&send_mutex) != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to unlock send_mutex, SendMessage !!!");
    }
    
//    if (pthread_mutex_unlock(&conn_mutex) != 0) {
//        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to unlock conn_mutex, SendMessage !!!");
//    }
    
    return 0;
}

void QNetworkClient::recv_cb(EV_P_ ev_io *w, int revents) {
    QConnection *qconnection = (QConnection *)w->data;

    while (1) {
        struct sockaddr_storage peer_addr;
        socklen_t peer_addr_len = sizeof(peer_addr);
        memset(&peer_addr, 0, peer_addr_len);

        ssize_t read = recvfrom(qconnection->sock, qconnection->recv_buf, sizeof(qconnection->recv_buf), 0,
                                (struct sockaddr *) &peer_addr,
                                &peer_addr_len);

        if (read < 0) {
            if ((errno == EWOULDBLOCK) || (errno == EAGAIN)) {
                DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "recv would block");
                break;
            }

            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to read");
            return;
        }

        RecvInfo recv_info = {
            (struct sockaddr *) &peer_addr,
            peer_addr_len,

            (struct sockaddr *) &qconnection->local_addr,
            qconnection->local_addr_len,
        };

        ssize_t done = quiche_conn_recv(qconnection->conn, qconnection->recv_buf, read, &recv_info);

        if (done < 0) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to process packet\n");
            continue;
        }

        DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "recv %zd bytes", done);
    }

    DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "done reading");

    if (quiche_conn_is_closed(qconnection->conn)) {
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "connection closed");
        qconnection->bridge->Event_Close(qconnection);
        ev_break(EV_A_ EVBREAK_ONE);
        return;
    }

    if (quiche_conn_is_established(qconnection->conn) && qconnection->IsOpen()) {
        const uint8_t *app_proto;
        size_t app_proto_len;

        quiche_conn_application_proto(qconnection->conn, &app_proto, &app_proto_len);

        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "connection established: %.*s",
                (int) app_proto_len, app_proto);
        qconnection->bridge->Event_Connect(qconnection);

        const static uint8_t hi[] = "Hi\r\n";
        if (quiche_conn_stream_send(qconnection->conn, 4, hi, sizeof(hi), false) < 0) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to send Hi request");
            return;
        }

        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "sent Hi request");
    }

    if (quiche_conn_is_established(qconnection->conn)) {
        uint64_t s = 0;

        StreamIter *readable = quiche_conn_readable(qconnection->conn);

        while (quiche_stream_iter_next(readable, &s)) {
            DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "stream %" PRIu64 " is readable", s);

            bool fin = false;
            ssize_t recv_len = quiche_conn_stream_recv(qconnection->conn, s,
                                                       qconnection->recv_buf, sizeof(qconnection->recv_buf),
                                                       &fin);
            if (recv_len < 0) {
                break;
            }

            if (fin) {
                if (quiche_conn_close(qconnection->conn, true, 0, NULL, 0) < 0) {
                    DEBUG_PRINT_ERROR(__LOGTAG__, "failed to close connection");
                }
                DEBUG_PRINT_IMPORTANT(__LOGTAG__, "fin received, closing...");
            }
            qconnection->bridge->Event_MsgReceived(recv_len, qconnection->recv_buf, qconnection);
        }

        quiche_stream_iter_free(readable);
    }

    qconnection->bridge->FlushEgress(loop, qconnection);
}

void QNetworkClient::timeout_cb(EV_P_ ev_timer *w, int revents) {
    QConnection *qconnection = (QConnection *)w->data;
    quiche_conn_on_timeout(qconnection->conn);

    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "timeout");

    qconnection->bridge->FlushEgress(loop, qconnection);

    if (quiche_conn_is_closed(qconnection->conn)) {
        Stats stats;
        PathStats path_stats;

        quiche_conn_stats(qconnection->conn, &stats);
        quiche_conn_path_stats(qconnection->conn, 0, &path_stats);

        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "connection closed, recv=%zu sent=%zu lost=%zu rtt=%" PRIu64 "ns",
                stats.recv, stats.sent, stats.lost, path_stats.rtt);
        qconnection->bridge->Event_Close(qconnection);
        ev_break(EV_A_ EVBREAK_ONE);
        return;
    } else {
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "connection not closed");
    }
}

void QNetworkClient::OnConnect(QConnection* qconnection) {
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "########## CONNECTED ##########");
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "########## CONNECTED ##########");
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "########## CONNECTED ##########");
}

void QNetworkClient::OnClose(QConnection* qconnection) {
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "########## CLOSED ##########");
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "########## CLOSED ##########");
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "########## CLOSED ##########");
}

void QNetworkClient::OnMessage(ssize_t recv_len, uint8_t* buf, QConnection* qconnection) {
    uint8_t* copybuf = new uint8_t[recv_len+1];
    memcpy(copybuf, buf, recv_len);
    copybuf[recv_len] = '\0';
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "---------<<<<<<<<<<< %s [len%d]", copybuf, recv_len);
    GX_DELETE_ARY(copybuf);
}

void QNetworkClient::OnDestroyConnection(QConnection* qconnection) {
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "Connection about to destroy !!!");
}


QNetworkClient::QNetworkClient() {
    if (pthread_mutex_init(&run_mutex, nullptr) != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "run_mutex init has failed: %s - %d", strerror (errno), errno);
    } else {
        DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "run_mutex init");
    }
    
//    if (pthread_mutex_init(&conn_mutex, nullptr) != 0) {
//        DEBUG_PRINT_ERROR(__LOGTAG__, "conn_mutex init has failed: %s - %d", strerror (errno), errno);
//    } else {
//        DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "conn_mutex init");
//    }
    
//    if (pthread_mutex_init(&recv_mutex, nullptr) != 0) {
//        DEBUG_PRINT_ERROR(__LOGTAG__, "recv_mutex init has failed: %s - %d", strerror (errno), errno);
//    } else {
//        DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "recv_mutex init");
//    }
    
    if (pthread_mutex_init(&send_mutex, nullptr) != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "send_mutex init has failed: %s - %d", strerror (errno), errno);
    } else {
        DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "send_mutex init");
    }
    
    if (pthread_mutex_init(&close_mutex, nullptr) != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "close_mutex init has failed: %s - %d", strerror (errno), errno);
    } else {
        DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "close_mutex init");
    }
}

QNetworkClient::~QNetworkClient() {
    pthread_mutex_destroy(&close_mutex);
//    pthread_mutex_destroy(&recv_mutex);
    pthread_mutex_destroy(&send_mutex);
//    pthread_mutex_destroy(&conn_mutex);
    pthread_mutex_destroy(&run_mutex);
}

void* QNetworkClient::run_internal(void* data) {
    RunConfig* runConfig = (RunConfig*)data;
    std::string host = runConfig->host;
    std::string port = runConfig->port;
    QNetworkClient* thiz = runConfig->thiz;
    
    if(pthread_mutex_trylock(&thiz->run_mutex)!=0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to acuire lock(run_mutex), returning...");
        runConfig->pthread_returnValue = -1;
        pthread_exit(&runConfig->pthread_returnValue);
    }
    
    quiche_enable_debug_logging(debug_log, NULL);

    Config *config = quiche_config_new(0xbabababa);
    if (config == NULL) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create config");
        runConfig->pthread_returnValue = -1;
        if (pthread_mutex_unlock(&thiz->run_mutex) != 0) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to unlock mutex !!!");
        }
        pthread_mutex_destroy(&thiz->run_mutex);
        pthread_exit(&runConfig->pthread_returnValue);
    }

    quiche_config_set_application_protos(config,
        (uint8_t *) "\x0ahq-interop\x05hq-29\x05hq-28\x05hq-27\x08http/0.9", 38);

    quiche_config_set_max_idle_timeout(config, 10000);
    quiche_config_set_max_recv_udp_payload_size(config, MAX_DATAGRAM_SIZE);
    quiche_config_set_max_send_udp_payload_size(config, MAX_DATAGRAM_SIZE);
    quiche_config_set_initial_max_data(config, 10000000);
    quiche_config_set_initial_max_stream_data_bidi_local(config, 1000000);
    quiche_config_set_initial_max_stream_data_uni(config, 1000000);
    quiche_config_set_initial_max_streams_bidi(config, 100);
    quiche_config_set_initial_max_streams_uni(config, 100);
    quiche_config_set_disable_active_migration(config, true);

    if (getenv("SSLKEYLOGFILE")) {
      quiche_config_log_keys(config);
    }

    thiz->qclientConnection = new QConnection(thiz, config);
    if (thiz->qclientConnection == NULL) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create qconnection");
        runConfig->pthread_returnValue = -1;
        if (pthread_mutex_unlock(&thiz->run_mutex) != 0) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to unlock mutex !!!");
        }
        pthread_mutex_destroy(&thiz->run_mutex);
        pthread_exit(&runConfig->pthread_returnValue);
    }
    thiz->qclientConnection->Connect(host, port);
    
    ev_io watcher;
    thiz->mainloop = ev_loop_new(0);

    ev_io_init(&watcher, recv_cb, thiz->qclientConnection->sock, EV_READ);
    ev_io_start(thiz->mainloop, &watcher);
    watcher.data = thiz->qclientConnection;

    ev_init(&thiz->qclientConnection->timer, timeout_cb);
    thiz->qclientConnection->timer.data = thiz->qclientConnection;

    thiz->FlushEgress(thiz->mainloop, thiz->qclientConnection);

    ev_loop(thiz->mainloop, 0);

    thiz->DestroyConnection(thiz->mainloop, thiz->qclientConnection);
    
    quiche_config_free(config);
    
    if (pthread_mutex_unlock(&thiz->run_mutex) != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to unlock mutex !!!");
    }
    pthread_exit(0);
}

int QNetworkClient::run(std::string host, std::string port) {
    runConfig.host = host;
    runConfig.port = port;
    runConfig.thiz = this;
        
    if( pthread_create( &run_thread_id, nullptr,  QNetworkClient::run_internal, (void*)&runConfig) < 0)
    {
        DEBUG_PRINT_ERROR(__LOGTAG__, "could not create thread: %s - %d", strerror (errno), errno);
        return -1;
    }
    return 0;
}
