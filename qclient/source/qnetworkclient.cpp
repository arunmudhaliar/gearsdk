//
//  qnetworkclient.cpp
//  networkclient
//
//  Created by Arun A on 12/10/23.
//

#include "qnetworkclient.hpp"

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
#include <algorithm>

using namespace client;

int qnetworkclient::connectionID = 0;

// MARK: - QConnection
conn_io_client::conn_io_client(bridge_qcommand* bridge, int id) : bridge(bridge),
id(id) {
}

conn_io_client::conn_io_client(bridge_qcommand* bridge, Config* config, int id) : bridge(bridge),
config(config),
id(id) {
}

conn_io_client::~conn_io_client() {
    Release();
}

void conn_io_client::Release() {
    if (peer) {
        freeaddrinfo(peer);
        peer = nullptr;
    }
    ev_timer_stop(bridge->getmainloop(), &timer);
    ev_timer_stop(bridge->getmainloop(), &sendTimer);

    watcher.data = nullptr;
    ev_io_stop(bridge->getmainloop(), &watcher);
    ev_break(bridge->getmainloop(), EVBREAK_ONE);
    if (conn) {
        quiche_conn_free(conn);
        conn = nullptr;
    }
    
    for (auto it = sendBuffer.cbegin(); it != sendBuffer.cend(); it++) {
        qdata* sd = *it;
        GX_DELETE(sd);
    }
    sendBuffer.clear();
}

int conn_io_client::Connect(qstring host, qstring port) {
    const struct addrinfo hints = {
        .ai_family = PF_UNSPEC,
        .ai_socktype = SOCK_DGRAM,
        .ai_protocol = IPPROTO_UDP };

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

    uint8_t scid[Q_LOCAL_CONN_ID_LEN];
    int rng = open("/dev/urandom", O_RDONLY);
    if (rng < 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to open /dev/urandom");
        return -1;
    }

    ssize_t rand_len = read(rng, &scid, sizeof(scid));
    if (rand_len < 0) {
        close(rng);
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create connection ID");
        return -1;
    }
    close(rng);

    local_addr_len = sizeof(local_addr);
    if (getsockname(sock, (struct sockaddr*)&local_addr,
        &local_addr_len) != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to get local address of socket");
        return -1;
    };

    conn = quiche_connect(host.c_str(), (const uint8_t*)scid, sizeof(scid),
        (struct sockaddr*)&local_addr,
        local_addr_len,
        peer->ai_addr, peer->ai_addrlen, config);

    if (conn == NULL) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create connection");
        return -1;
    }

    return 0;
}

int conn_io_client::ConnectionActive() {
    if (!conn) {
        return -1;
    }
    if (!quiche_conn_is_established(conn)) {
        return -2;
    }

    if (quiche_conn_is_closed(conn)) {
        return -3;
    }

    return 0;
}

ssize_t conn_io_client::SendMessage(const qstring& buffer, bool fin) {
    return SendMessage(buffer.c_str(), buffer.length(), fin);
}

ssize_t conn_io_client::SendMessage(const char* buf, size_t buflen, bool fin) {
    int conn_active = ConnectionActive();
    if (conn_active < 0) {
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "Cant send !!!, conn is null or not active = %d", conn_active);
        return conn_active;
    }

    if (!quiche_conn_is_established(conn)) {
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "Cant send !!!, connection not established - %s", (char*)buf);
        return -2;
    }

    if (quiche_conn_is_closed(conn)) {
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "Cant send !!!, connection closed - %s", (char*)buf);
        return -3;
    }

    ssize_t result = -4;
    uint64_t s = 0;
    StreamIter* writable = quiche_conn_writable(conn);
    while (quiche_stream_iter_next(writable, &s)) {
        //        DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "stream %" PRIu64 " is writable", s);
        ssize_t sent_len = quiche_conn_stream_send(conn, s, (uint8_t*)buf,
            buflen, fin);
        if (sent_len != buflen) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "send failure %d", sent_len);
            break;
        }
        // DEBUG_PRINT_IMPORTANT(__LOGTAG__, "--------->>>>>>>>>>>[%d] %s", s, (char*)buf);
        result = sent_len;
        break;
    }
    quiche_stream_iter_free(writable);

    return result;
}

void qnetworkclient::setstate(CON_STATE state) {
    if (this->state >= state) {
        DEBUG_PRINT_WARN(__LOGTAG__, "QConnection state >= state, this->state %d, incoming state %d", this->state, state);
    }
    this->state = state;
}

void qnetworkclient::debug_log(const uint8_t* line, void* argp) {
    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "%s", (char*)line);
}

void qnetworkclient::flushegress(struct ev_loop* loop, conn_io_client* qconnection) {
    SendInfo send_info;
    while (true) {
        ssize_t written = quiche_conn_send(qconnection->conn, qconnection->egress_out, sizeof(qconnection->egress_out),
            &send_info);

        if (written == QUICHE_ERR_DONE) {
            DEBUG_PRINT(LOG_LEVEL_4, __LOGTAG__, "done writing");
            break;
        }

        if (written < 0) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create packet: %zd", written);
            return;
        }

        ssize_t sent = sendto(qconnection->sock, qconnection->egress_out, written, 0,
            (struct sockaddr*)&send_info.to,
            send_info.to_len);

        if (sent != written) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to send");
            return;
        }

        DEBUG_PRINT(LOG_LEVEL_4, __LOGTAG__, "sent %zd bytes", sent);
    }

    uint64_t timeout_in_nanos = quiche_conn_timeout_as_nanos(qconnection->conn);
    double t = (double)timeout_in_nanos / 1e9f;
    qconnection->timer.repeat = t;
    ev_timer_again(loop, &qconnection->timer);
    DEBUG_PRINT(LOG_LEVEL_5, __LOGTAG__, "qconnection->timer.repeat %f - %" PRIu64 "", t, timeout_in_nanos);
}

void qnetworkclient::event_connect(conn_io_client* qconnection) {
    setstate(STATE_CONNECT);
    onconnect(qconnection);
}

void qnetworkclient::event_msg_received(ssize_t recv_len, uint8_t* buf, conn_io_client* qconnection) {
    onmessage(recv_len, buf, qconnection);
}

void qnetworkclient::event_close(conn_io_client* qconnection) {
    if (isclosed()) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "Connection already closed, Event_Close returning...");
        return;
    }
    setstate(STATE_CLOSE);
    onclose(qconnection);
}

void qnetworkclient::forcerelease() {
    if (release_connection(getmainloop(), qclient_connection) == 0) {
        GX_DELETE(qclient_connection);
    }
}

int qnetworkclient::release_connection(struct ev_loop* loop, conn_io_client* qconnection) {
    int retVal = 0;
    if (qconnection != nullptr) {
        qconnection->Release();
        onreleaseconnection(qconnection);
        GX_DELETE(qclient_connection);
        DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "Connection released !!!");
    }
    else {
        retVal = -1;
        DEBUG_PRINT_ERROR(__LOGTAG__, "Already destroyed.. Ignoring...");
    }
    return retVal;
}

int qnetworkclient::close() {
#if USE_PTHREAD
    // lock
    DEBUG_ASSERT(__LOGTAG__, (close_mutex.tryLock(__FUNCTION__) == 0), __FUNCTION__);
    close_mutex.conditionalWait(__FUNCTION__);
    close_mutex.unLock();
    // block close
    send_mutex.block(__FUNCTION__);
    sendloop_mutex.block(__FUNCTION__);
#endif
    
    if (state == CON_STATE::STATE_CONNECT) {
        if (qclient_connection) {
            int conActive = qclient_connection->ConnectionActive();
            if (conActive == 0) {
                const uint8_t bye[] = "Bye\r\n";
                qclient_connection->sendBuffer.push_back(DEBUG_NEW qdata((uint8_t*)bye, sizeof(bye), true));
            }
        }
    }
#if USE_PTHREAD
    sendloop_mutex.unBlock(__FUNCTION__);
    send_mutex.unBlock(__FUNCTION__);
#endif
    return 0;
}

int qnetworkclient::sendMessage(const uint8_t* buffer, ssize_t size, bool flush) {
#if USE_PTHREAD
    // lock
    DEBUG_ASSERT(__LOGTAG__, (send_mutex.tryLock(__FUNCTION__) == 0), __FUNCTION__);
    send_mutex.conditionalWait(__FUNCTION__);
    send_mutex.unLock();
    // block
    close_mutex.block(__FUNCTION__);
    sendloop_mutex.block(__FUNCTION__);
#endif
    
    if (qclient_connection) {
        qclient_connection->sendBuffer.push_back(DEBUG_NEW qdata(buffer, size));
    }
    
#if USE_PTHREAD
    sendloop_mutex.unBlock(__FUNCTION__);
    close_mutex.unBlock(__FUNCTION__);
#endif
    return 0;
}

int qnetworkclient::sendMessage(const qstring& buffer, bool flush) {
    return sendMessage((uint8_t*)buffer.c_str(), buffer.length(), flush);
}

void qnetworkclient::recv_cb(EV_P_ ev_io* w, int revents) {
    conn_io_client* qconnection_ = (conn_io_client*)w->data;
    if (qconnection_->conn == nullptr) {
        return;
    }

    while (true) {
        struct sockaddr_storage peer_addr;
        socklen_t peer_addr_len = sizeof(peer_addr);
        memset(&peer_addr, 0, peer_addr_len);

        ssize_t read = recvfrom(qconnection_->sock, qconnection_->recv_buf, sizeof(qconnection_->recv_buf), 0,
            (struct sockaddr*)&peer_addr,
            &peer_addr_len);

        if (read < 0) {
            if ((errno == EWOULDBLOCK) || (errno == EAGAIN)) {
                DEBUG_PRINT(LOG_LEVEL_4, __LOGTAG__, "recv would block");
                break;
            }

            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to read");
            return;
        }

        RecvInfo recv_info = {
            (struct sockaddr*)&peer_addr,
            peer_addr_len,

            (struct sockaddr*)&qconnection_->local_addr,
            qconnection_->local_addr_len,
        };

        ssize_t done = quiche_conn_recv(qconnection_->conn, qconnection_->recv_buf, read, &recv_info);

        if (done < 0) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to process packet");
            continue;
        }

        DEBUG_PRINT(LOG_LEVEL_4, __LOGTAG__, "recv %zd bytes", done);
    }

    DEBUG_PRINT(LOG_LEVEL_4, __LOGTAG__, "done reading");

    if (quiche_conn_is_established(qconnection_->conn) && qconnection_->bridge->getstate() == CON_STATE::STATE_OPEN) {
        const uint8_t* app_proto;
        size_t app_proto_len;

        quiche_conn_application_proto(qconnection_->conn, &app_proto, &app_proto_len);

        DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "connection established: %.*s",
            (int)app_proto_len, app_proto);
        qconnection_->bridge->event_connect(qconnection_);

        const static uint8_t hi[] = "Hi";
        if (quiche_conn_stream_send(qconnection_->conn, 4, hi, sizeof(hi), false) < 0) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to send Hi request");
            return;
        }
        DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "sent Hi request");
    }

    if (quiche_conn_is_established(qconnection_->conn)) {
        uint64_t s = 0;
        StreamIter* readable = quiche_conn_readable(qconnection_->conn);
        while (quiche_stream_iter_next(readable, &s)) {
            DEBUG_PRINT(LOG_LEVEL_4, __LOGTAG__, "stream %" PRIu64 " is readable", s);

            bool fin = false;
            ssize_t recv_len = quiche_conn_stream_recv(qconnection_->conn, s,
                qconnection_->recv_buf, sizeof(qconnection_->recv_buf),
                &fin);
            if (recv_len < 0) {
                break;
            }

            if (fin) {
                if (quiche_conn_close(qconnection_->conn, true, 0, NULL, 0) < 0) {
                    DEBUG_PRINT_ERROR(__LOGTAG__, "failed to close connection");
                }
                DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "fin received, closing...");
            }
            qconnection_->bridge->event_msg_received(recv_len, qconnection_->recv_buf, qconnection_);
        }
        quiche_stream_iter_free(readable);
    }

    if (qconnection_->conn) {
        qconnection_->bridge->flushegress(loop, qconnection_);
    }
}

void qnetworkclient::send_cb(EV_P_ ev_timer* w, int revents) {
    conn_io_client* qconnection_ = (conn_io_client*)w->data;
#if USE_PTHREAD
    // lock
    DEBUG_ASSERT(__LOGTAG__, (qconnection_->bridge->get_sendloop_mutex()->tryLock(__FUNCTION__) == 0), __FUNCTION__);
    qconnection_->bridge->get_sendloop_mutex()->conditionalWait(__FUNCTION__);
    qconnection_->bridge->get_sendloop_mutex()->unLock();
    // block
    qconnection_->bridge->het_close_mutex()->block(__FUNCTION__);
    qconnection_->bridge->get_send_mutex()->block(__FUNCTION__);
#endif
    
    if (qconnection_->bridge->getstate() == CON_STATE::STATE_CONNECT) {
        std::vector<qdata*> successfullySent;
        for (auto it = qconnection_->sendBuffer.cbegin(); it != qconnection_->sendBuffer.cend(); it++) {
            qdata* sd = *it;
            ssize_t send_res = qconnection_->SendMessage((const char*)sd->data, sd->size, sd->fin);
            if (sd->size != send_res) {
                DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "send_cb failed for %s, err %d", sd->data, send_res);
            }
            else {
                successfullySent.push_back(sd);
                qconnection_->bridge->flushegress(qconnection_->bridge->getmainloop(), qconnection_);
            }
        }

        for (auto it = successfullySent.cbegin(); it != successfullySent.cend(); it++) {
            qdata* fd = *it;
            int oldSz = (int)qconnection_->sendBuffer.size();
            qconnection_->sendBuffer.erase(std::remove(qconnection_->sendBuffer.begin(), qconnection_->sendBuffer.end(), fd), qconnection_->sendBuffer.end());
            if (oldSz != qconnection_->sendBuffer.size()) {
                GX_DELETE(fd);
            }
        }
    }

#if USE_PTHREAD
    qconnection_->bridge->get_send_mutex()->unBlock(__FUNCTION__);
    qconnection_->bridge->het_close_mutex()->unBlock(__FUNCTION__);
#endif
}

void qnetworkclient::timeout_cb(EV_P_ ev_timer* w, int revents) {
    conn_io_client* qconnection_ = (conn_io_client*)w->data;
    if (qconnection_->conn == nullptr) {
        ev_break(EV_A_ EVBREAK_ONE);
        return;
    }

    DEBUG_PRINT(LOG_LEVEL_4, __LOGTAG__, "timeout");

    quiche_conn_on_timeout(qconnection_->conn);
    qconnection_->bridge->flushegress(loop, qconnection_);

    if (quiche_conn_is_closed(qconnection_->conn)) {
        Stats stats;
        PathStats path_stats;

        quiche_conn_stats(qconnection_->conn, &stats);
        quiche_conn_path_stats(qconnection_->conn, 0, &path_stats);

        DEBUG_PRINT(LOG_LEVEL_4, __LOGTAG__, "connection closed, recv=%zu sent=%zu lost=%zu rtt=%" PRIu64 "ns",
            stats.recv, stats.sent, stats.lost, path_stats.rtt);
        qconnection_->bridge->event_close(qconnection_);
        ev_break(EV_A_ EVBREAK_ONE);
        return;
    }
    else {
        if (quiche_conn_is_established(qconnection_->conn)) {
            DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "connection not closed");
        }
    }
}

void qnetworkclient::onconnect(conn_io_client* qconnection) {
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "########## CONNECTED ########## - %d", qconnection->id);
}

void qnetworkclient::onclose(conn_io_client* qconnection) {
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "########## CLOSED ########## - %d", qconnection->id);
}

void qnetworkclient::onmessage(ssize_t recv_len, uint8_t* buf, conn_io_client* qconnection) {
    uint8_t* copybuf = DEBUG_NEW uint8_t[recv_len + 1];
    memcpy(copybuf, buf, recv_len);
    copybuf[recv_len] = '\0';
    DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "---------<<<<<<<<<<< %s [len:%d]", copybuf, recv_len);
    GX_DELETE_ARY(copybuf);
}

void qnetworkclient::onreleaseconnection(conn_io_client* qconnection) {
    DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "Connection about to release !!!");
}

qnetworkclient::qnetworkclient() {
#if USE_PTHREAD
    DEBUG_ASSERT(__LOGTAG__, (run_mutex.init("run") == 0), "qnetworkclient Constructor - CHECK !!!");
    DEBUG_ASSERT(__LOGTAG__, (send_mutex.init("send") == 0), "qnetworkclient Constructor - CHECK !!!");
    DEBUG_ASSERT(__LOGTAG__, (sendloop_mutex.init("sendLoop") == 0), "qnetworkclient Constructor - CHECK !!!");
    DEBUG_ASSERT(__LOGTAG__, (close_mutex.init("close") == 0), "qnetworkclient Constructor - CHECK !!!");
    DEBUG_ASSERT(__LOGTAG__, (runconfig_mutex.init("runConfig") == 0), "qnetworkclient Constructor - CHECK !!!");
#endif
    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "qnetworkclient created !!!");
}

qnetworkclient::~qnetworkclient() {
    release_connection(mainloop, qclient_connection);
    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "qnetworkclient destroyed !!!");
}

void* qnetworkclient::run_internal(void* data) {
    RunConfig* runConfig = (RunConfig*)data;
    qstring host = runConfig->host;
    qstring port = runConfig->port;
    qnetworkclient* thiz = runConfig->thiz;
#if USE_PTHREAD
    if (thiz->run_mutex.tryLock(__FUNCTION__) != 0) {
        runConfig->finished = true;
        runConfig->pthread_returnValue = -1;
        pthread_exit(&runConfig->pthread_returnValue);
    }
#endif

    //    quiche_enable_debug_logging(debug_log, nullptr);

    Config* config = quiche_config_new(0xbabababa);
    if (config == NULL) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create config");
        runConfig->pthread_returnValue = -1;
        runConfig->finished = true;
#if USE_PTHREAD
        DEBUG_ASSERT(__LOGTAG__, (thiz->run_mutex.unLock() == 0), "CHECK !!!");
        pthread_exit(&runConfig->pthread_returnValue);
#else
        return nullptr;
#endif
    }

    quiche_config_set_application_protos(config,
        (uint8_t*)"\x0ahq-interop\x05hq-29\x05hq-28\x05hq-27\x08http/0.9", 38);

    quiche_config_set_max_idle_timeout(config, 30000);
    quiche_config_set_max_recv_udp_payload_size(config, Q_MAX_DATAGRAM_SIZE);
    quiche_config_set_max_send_udp_payload_size(config, Q_MAX_DATAGRAM_SIZE);
    quiche_config_set_initial_max_data(config, 10000000);
    quiche_config_set_initial_max_stream_data_bidi_local(config, 1000000);
    quiche_config_set_initial_max_stream_data_uni(config, 1000000);
    //    quiche_config_set_initial_max_stream_data_bidi_remote(config, 1000000);
    quiche_config_set_initial_max_streams_bidi(config, 100);
    quiche_config_set_initial_max_streams_uni(config, 100);
    quiche_config_set_disable_active_migration(config, true);

    if (getenv("SSLKEYLOGFILE")) {
        quiche_config_log_keys(config);
    }

#if USE_PTHREAD
    thiz->sendloop_mutex.block(__FUNCTION__);
    thiz->close_mutex.block(__FUNCTION__);
    thiz->send_mutex.block(__FUNCTION__);
#endif

    thiz->qclient_connection = DEBUG_NEW conn_io_client(thiz, config, runConfig->id);
    if (thiz->qclient_connection == NULL) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create qconnection");
        runConfig->pthread_returnValue = -1;
        runConfig->finished = true;
#if USE_PTHREAD
        DEBUG_ASSERT(__LOGTAG__, (thiz->run_mutex.unLock() == 0), "CHECK !!!");
        pthread_exit(&runConfig->pthread_returnValue);
#else
        return nullptr;
#endif
    }
    thiz->qclient_connection->Connect(host, port);

    thiz->mainloop = ev_loop_new(0);

    ev_io_init(&thiz->qclient_connection->watcher, recv_cb, thiz->qclient_connection->sock, EV_READ);
    ev_io_start(thiz->mainloop, &thiz->qclient_connection->watcher);
    thiz->qclient_connection->watcher.data = thiz->qclient_connection;

    ev_init(&thiz->qclient_connection->timer, timeout_cb);
    thiz->qclient_connection->timer.data = thiz->qclient_connection;

    ev_init(&thiz->qclient_connection->sendTimer, send_cb);
    thiz->qclient_connection->sendTimer.data = thiz->qclient_connection;
    thiz->qclient_connection->sendTimer.repeat = 0.2f;
    ev_timer_again(thiz->mainloop, &thiz->qclient_connection->sendTimer);
    //    ev_timer_start(thiz->mainloop, &thiz->qclientConnection->sendTimer);

    thiz->flushegress(thiz->mainloop, thiz->qclient_connection);

#if USE_PTHREAD
    thiz->send_mutex.unBlock(__FUNCTION__);
    thiz->close_mutex.unBlock(__FUNCTION__);
    thiz->sendloop_mutex.unBlock(__FUNCTION__);
#endif
    ev_loop(thiz->mainloop, 0);

    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "run_internal loop released !!!");
    thiz->release_connection(thiz->mainloop, thiz->qclient_connection);

    ev_loop_destroy(thiz->mainloop);
    
    quiche_config_free(config);

#if USE_PTHREAD
    DEBUG_ASSERT(__LOGTAG__, (thiz->get_runconfigmutex().tryLock(__FUNCTION__) == 0), __FUNCTION__);
    runConfig->pthread_returnValue = 0;
    runConfig->finished = true;
    DEBUG_ASSERT(__LOGTAG__, (thiz->get_runconfigmutex().unLock() == 0), __FUNCTION__);
    DEBUG_ASSERT(__LOGTAG__, (thiz->run_mutex.unLock() == 0), "CHECK !!!");
    pthread_exit(0);
#else
    runConfig->pthread_returnValue = 0;
    runConfig->finished = true;
    return nullptr;
#endif
}

bool qnetworkclient::is_runfinished() {
#if USE_PTHREAD
    DEBUG_ASSERT(__LOGTAG__, (runconfig_mutex.tryLock(__FUNCTION__) == 0), __FUNCTION__);
    bool retVal = runConfig.finished;
    DEBUG_ASSERT(__LOGTAG__, (runconfig_mutex.unLock() == 0), __FUNCTION__);
#else
    bool retVal = runConfig.finished;
#endif
    return retVal;
}

int qnetworkclient::run(qstring host, qstring port) {
#if USE_PTHREAD
    DEBUG_ASSERT(__LOGTAG__, (runconfig_mutex.tryLock(__FUNCTION__) == 0), __FUNCTION__);
#endif
    runConfig.host = host;
    runConfig.port = port;
    runConfig.thiz = this;
    runConfig.finished = false;
    runConfig.id = qnetworkclient::connectionID++;
#if USE_PTHREAD
    DEBUG_ASSERT(__LOGTAG__, (runconfig_mutex.unLock() == 0), __FUNCTION__);
    if (pthread_create(&run_thread_id, nullptr, qnetworkclient::run_internal, (void*)&runConfig) < 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "could not create thread: %s - %d", strerror(errno), errno);
        return -1;
    }
#else
    qnetworkclient::run_internal((void*)&runConfig);
#endif
    return 0;
}
