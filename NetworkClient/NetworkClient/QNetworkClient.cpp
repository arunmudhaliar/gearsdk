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
#include <algorithm>

int QNetworkClient::connectionID = 0;

#pragma region QConnection
QConnection::QConnection(MQCommandBridge* bridge, int id) :
bridge(bridge),
id(id) {
}

QConnection::QConnection(MQCommandBridge* bridge, Config *config, int id) :
bridge(bridge),
config(config),
id(id) {
}

QConnection::~QConnection() {
    Release();
}

void QConnection::Release() {
    if (peer) {
        freeaddrinfo(peer);
        peer = nullptr;
    }
    ev_timer_stop(bridge->GetMainLoop(), &timer);
    ev_timer_stop(bridge->GetMainLoop(), &sendTimer);
    
    watcher.data = nullptr;
    ev_io_stop(bridge->GetMainLoop(), &watcher);
    ev_break(bridge->GetMainLoop(), EVBREAK_ONE);
    if (conn) {
        quiche_conn_free(conn);
        conn = nullptr;
    }
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
        close(rng);
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create connection ID");
        return -1;
    }
    close(rng);

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

ssize_t QConnection::SendMessage(const std::string& buffer, bool fin) {
    return SendMessage(buffer.c_str(), buffer.size(), fin);
}

ssize_t QConnection::SendMessage(const char *buf, size_t buflen, bool fin) {
    int conn_active = ConnectionActive();
    if (conn_active< 0) {
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "Cant send !!!, conn is null or not active = %d", conn_active);
        return conn_active;
    }
    
    if (!quiche_conn_is_established(conn)) {
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "Cant send !!!, connection not established - ", (char*)buf);
        return -2;
    }
    
    if (quiche_conn_is_closed(conn)) {
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "Cant send !!!, connection closed - ", (char*)buf);
        return -3;
    }
    
    ssize_t result = -4;
    uint64_t s = 0;
    StreamIter *writable = quiche_conn_writable(conn);
    while (quiche_stream_iter_next(writable, &s)) {
//        DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "stream %" PRIu64 " is writable", s);
        ssize_t sent_len = quiche_conn_stream_send(conn, s, (uint8_t *) buf,
                                buflen, fin);
        if (sent_len!=buflen) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "send failure %d", sent_len);
            break;
        }
        //DEBUG_PRINT_IMPORTANT(__LOGTAG__, "--------->>>>>>>>>>>[%d] %s", s, (char*)buf);
        result = sent_len;
        break;
    }
    quiche_stream_iter_free(writable);

    return result;
}
#pragma endregion QConnection



void QNetworkClient::SetState(CON_STATE state) {
    if (this->state>=state) {
        DEBUG_PRINT_WARN(__LOGTAG__, "QConnection state >= state, this->state %d, incoming state %d", this->state, state);
    }
    this->state = state;
}

void QNetworkClient::debug_log(const uint8_t *line, void *argp) {
     DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "%s", (char*)line);
}

void QNetworkClient::FlushEgress(struct ev_loop *loop, QConnection* qconnection) {
    SendInfo send_info;
    while (true) {
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
    DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "qconnection->timer.repeat %f - %" PRIu64 "", t, timeout_in_nanos);
}

void QNetworkClient::Event_Connect(QConnection* qconnection) {
    SetState(STATE_CONNECT);
    OnConnect(qconnection);
}

void QNetworkClient::Event_MsgReceived(ssize_t recv_len, uint8_t* buf, QConnection* qconnection) {
    OnMessage(recv_len, buf, qconnection);
}

void QNetworkClient::Event_Close(QConnection* qconnection) {
    if (IsClosed()) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "Connection already closed, Event_Close returning...");
        return;
    }
    SetState(STATE_CLOSE);
    OnClose(qconnection);
}

void QNetworkClient::ForceRelease() {
    if (ReleaseConnection(GetMainLoop(), qclientConnection)==0){
        GX_DELETE(qclientConnection);
    }
}

int QNetworkClient::ReleaseConnection(struct ev_loop *loop, QConnection* qconnection) {
    int retVal = 0;
    if (qconnection != nullptr) {
        qconnection->Release();
        OnReleaseConnection(qconnection);
        GX_DELETE(qclientConnection);
        DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "Connection released !!!");
    } else {
        retVal = -1;
        DEBUG_PRINT_ERROR(__LOGTAG__, "Already destroyed.. Ignoring...");
    }
    return retVal;
}

int QNetworkClient::Close() {
    //lock
    DEBUG_ASSERT(__LOGTAG__, (closeMutex.tryLock(__FUNCTION__)==0), __FUNCTION__);
    closeMutex.conditionalWait(__FUNCTION__);
    closeMutex.unLock();
    //block close
    sendMutex.block(__FUNCTION__);
    sendLoopMutex.block(__FUNCTION__);
    
    if (state == CON_STATE::STATE_CONNECT) {
        if (qclientConnection) {
            int conActive =  qclientConnection->ConnectionActive();
            if (conActive==0) {
                const uint8_t bye[] = "Bye\r\n";
                qclientConnection->sendBuffer.push_back(new QData((uint8_t*)bye,  sizeof(bye), true));
            }
        }
    }
    sendLoopMutex.unBlock(__FUNCTION__);
    sendMutex.unBlock(__FUNCTION__);
    
    return 0;
}

int QNetworkClient::SendMessage(const std::string& buffer, bool flush) {
    // lock
    DEBUG_ASSERT(__LOGTAG__, (sendMutex.tryLock(__FUNCTION__)==0), __FUNCTION__);
    sendMutex.conditionalWait(__FUNCTION__);
    sendMutex.unLock();
    //block
    closeMutex.block(__FUNCTION__);
    sendLoopMutex.block(__FUNCTION__);
    if (qclientConnection) {
        qclientConnection->sendBuffer.push_back(new QData((uint8_t*)buffer.c_str(),  buffer.size()));
    }
    sendLoopMutex.unBlock(__FUNCTION__);
    closeMutex.unBlock(__FUNCTION__);
    
    return 0;
}

void QNetworkClient::recv_cb(EV_P_ ev_io *w, int revents) {
    QConnection *qconnection = (QConnection *)w->data;
    if (qconnection->conn == nullptr) {
        return;
    }
    
    while (true) {
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
            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to process packet");
            continue;
        }

        DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "recv %zd bytes", done);
    }

    DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "done reading");

    if (quiche_conn_is_established(qconnection->conn) && qconnection->bridge->GetState() == CON_STATE::STATE_OPEN) {
        const uint8_t *app_proto;
        size_t app_proto_len;

        quiche_conn_application_proto(qconnection->conn, &app_proto, &app_proto_len);

        DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "connection established: %.*s",
                (int) app_proto_len, app_proto);
        qconnection->bridge->Event_Connect(qconnection);

        const static uint8_t hi[] = "Hi\r\n";
        if (quiche_conn_stream_send(qconnection->conn, 4, hi, sizeof(hi), false) < 0) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to send Hi request");
            return;
        }
        DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "sent Hi request");
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
                DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "fin received, closing...");
            }
            qconnection->bridge->Event_MsgReceived(recv_len, qconnection->recv_buf, qconnection);
        }
        quiche_stream_iter_free(readable);
    }
    
    if (qconnection->conn) {
        qconnection->bridge->FlushEgress(loop, qconnection);
    }
}

void QNetworkClient::send_cb(EV_P_ ev_timer *w, int revents) {
    QConnection *qconnection = (QConnection *)w->data;
    // lock
    DEBUG_ASSERT(__LOGTAG__, (qconnection->bridge->Get_sendLoopMutex()->tryLock(__FUNCTION__)==0), __FUNCTION__);
    qconnection->bridge->Get_sendLoopMutex()->conditionalWait(__FUNCTION__);
    qconnection->bridge->Get_sendLoopMutex()->unLock();
    //block
    qconnection->bridge->Get_closeMutex()->block(__FUNCTION__);
    qconnection->bridge->Get_sendMutex()->block(__FUNCTION__);
    
    if (qconnection->bridge->GetState() == CON_STATE::STATE_CONNECT) {
        std::vector<QData*> successfullySent;
        for(auto it = qconnection->sendBuffer.cbegin();it!=qconnection->sendBuffer.cend();it++) {
            QData* sd = *it;
            ssize_t send_res = qconnection->SendMessage((const char *)sd->data, sd->size, sd->fin);
            if (sd->size!=send_res) {
                DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "send_cb failed for %s, err %d", sd->data, send_res);
            } else {
                successfullySent.push_back(sd);
                qconnection->bridge->FlushEgress(qconnection->bridge->GetMainLoop(), qconnection);
            }
        }
        
        for(auto it = successfullySent.cbegin();it!=successfullySent.cend();it++) {
            QData* fd = *it;
            int oldSz = (int)qconnection->sendBuffer.size();
            qconnection->sendBuffer.erase(std::remove(qconnection->sendBuffer.begin(), qconnection->sendBuffer.end(), fd), qconnection->sendBuffer.end());
            if(oldSz!=qconnection->sendBuffer.size()) {
                GX_DELETE(fd);
            }
        }
    }

    qconnection->bridge->Get_sendMutex()->unBlock(__FUNCTION__);
    qconnection->bridge->Get_closeMutex()->unBlock(__FUNCTION__);
}

void QNetworkClient::timeout_cb(EV_P_ ev_timer *w, int revents) {
    QConnection *qconnection = (QConnection *)w->data;
    if (qconnection->conn == nullptr) {
        ev_break(EV_A_ EVBREAK_ONE);
        return;
    }
    
    DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "timeout");
    
    quiche_conn_on_timeout(qconnection->conn);
    qconnection->bridge->FlushEgress(loop, qconnection);
    
    if (quiche_conn_is_closed(qconnection->conn)) {
        Stats stats;
        PathStats path_stats;

        quiche_conn_stats(qconnection->conn, &stats);
        quiche_conn_path_stats(qconnection->conn, 0, &path_stats);

        DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "connection closed, recv=%zu sent=%zu lost=%zu rtt=%" PRIu64 "ns",
                stats.recv, stats.sent, stats.lost, path_stats.rtt);
        qconnection->bridge->Event_Close(qconnection);
        ev_break(EV_A_ EVBREAK_ONE);
        return;
    } else {
        DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "connection not closed");
    }
}

void QNetworkClient::OnConnect(QConnection* qconnection) {
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "########## CONNECTED ########## - %d", qconnection->id);
}

void QNetworkClient::OnClose(QConnection* qconnection) {
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "########## CLOSED ########## - %d", qconnection->id);
}

void QNetworkClient::OnMessage(ssize_t recv_len, uint8_t* buf, QConnection* qconnection) {
    uint8_t* copybuf = new uint8_t[recv_len+1];
    memcpy(copybuf, buf, recv_len);
    copybuf[recv_len] = '\0';
    DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "---------<<<<<<<<<<< %s [len:%d]", copybuf, recv_len);
    GX_DELETE_ARY(copybuf);
}

void QNetworkClient::OnReleaseConnection(QConnection* qconnection) {
    DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "Connection about to release !!!");
}


QNetworkClient::QNetworkClient() {
#if USE_PTHREAD
    DEBUG_ASSERT(__LOGTAG__, (runMutex.init("run")==0), "QNetworkClient Constructor - CHECK !!!");
    DEBUG_ASSERT(__LOGTAG__, (sendMutex.init("send")==0), "QNetworkClient Constructor - CHECK !!!");
    DEBUG_ASSERT(__LOGTAG__, (sendLoopMutex.init("sendLoop")==0), "QNetworkClient Constructor - CHECK !!!");
    DEBUG_ASSERT(__LOGTAG__, (closeMutex.init("close")==0), "QNetworkClient Constructor - CHECK !!!");
    DEBUG_ASSERT(__LOGTAG__, (runConfigMutex.init("runConfig")==0), "QNetworkClient Constructor - CHECK !!!");
#endif
}

QNetworkClient::~QNetworkClient() {
    ReleaseConnection(mainloop, qclientConnection);
}

void* QNetworkClient::run_internal(void* data) {
    RunConfig* runConfig = (RunConfig*)data;
    std::string host = runConfig->host;
    std::string port = runConfig->port;
    QNetworkClient* thiz = runConfig->thiz;
#if USE_PTHREAD
    if(thiz->runMutex.tryLock(__FUNCTION__)!=0) {
        runConfig->finished = true;
        runConfig->pthread_returnValue = -1;
        pthread_exit(&runConfig->pthread_returnValue);
    }
#endif
    
//    quiche_enable_debug_logging(debug_log, nullptr);

    Config *config = quiche_config_new(0xbabababa);
    if (config == NULL) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create config");
        runConfig->pthread_returnValue = -1;
        runConfig->finished = true;
#if USE_PTHREAD
        DEBUG_ASSERT(__LOGTAG__, (thiz->runMutex.unLock()==0), "CHECK !!!");
        pthread_exit(&runConfig->pthread_returnValue);
#else
        return nullptr;
#endif
    }

    quiche_config_set_application_protos(config,
        (uint8_t *) "\x0ahq-interop\x05hq-29\x05hq-28\x05hq-27\x08http/0.9", 38);

    quiche_config_set_max_idle_timeout(config, 30000);
    quiche_config_set_max_recv_udp_payload_size(config, MAX_DATAGRAM_SIZE);
    quiche_config_set_max_send_udp_payload_size(config, MAX_DATAGRAM_SIZE);
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

    thiz->sendLoopMutex.block(__FUNCTION__);
    thiz->closeMutex.block(__FUNCTION__);
    thiz->sendMutex.block(__FUNCTION__);
    
    thiz->qclientConnection = new QConnection(thiz, config, runConfig->id);
    if (thiz->qclientConnection == NULL) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create qconnection");
        runConfig->pthread_returnValue = -1;
        runConfig->finished = true;
#if USE_PTHREAD
        DEBUG_ASSERT(__LOGTAG__, (thiz->runMutex.unLock()==0), "CHECK !!!");
        pthread_exit(&runConfig->pthread_returnValue);
#else
        return nullptr;
#endif
    }
    thiz->qclientConnection->Connect(host, port);
    
    thiz->mainloop = ev_loop_new(0);

    ev_io_init(&thiz->qclientConnection->watcher, recv_cb, thiz->qclientConnection->sock, EV_READ);
    ev_io_start(thiz->mainloop, &thiz->qclientConnection->watcher);
    thiz->qclientConnection->watcher.data = thiz->qclientConnection;

    ev_init(&thiz->qclientConnection->timer, timeout_cb);
    thiz->qclientConnection->timer.data = thiz->qclientConnection;

    ev_init(&thiz->qclientConnection->sendTimer, send_cb);
    thiz->qclientConnection->sendTimer.data = thiz->qclientConnection;
    thiz->qclientConnection->sendTimer.repeat = 0.2f;
    ev_timer_again(thiz->mainloop, &thiz->qclientConnection->sendTimer);
//    ev_timer_start(thiz->mainloop, &thiz->qclientConnection->sendTimer);
    
    thiz->FlushEgress(thiz->mainloop, thiz->qclientConnection);

    thiz->sendMutex.unBlock(__FUNCTION__);
    thiz->closeMutex.unBlock(__FUNCTION__);
    thiz->sendLoopMutex.unBlock(__FUNCTION__);
    
    ev_loop(thiz->mainloop, 0);

    thiz->ReleaseConnection(thiz->mainloop, thiz->qclientConnection);
    
    quiche_config_free(config);
        
    DEBUG_ASSERT(__LOGTAG__, (thiz->GetRunConfigMutex().tryLock(__FUNCTION__)==0), __FUNCTION__);
    runConfig->pthread_returnValue = 0;
    runConfig->finished = true;
    DEBUG_ASSERT(__LOGTAG__, (thiz->GetRunConfigMutex().unLock()==0), __FUNCTION__);
#if USE_PTHREAD
    DEBUG_ASSERT(__LOGTAG__, (thiz->runMutex.unLock()==0), "CHECK !!!");
    pthread_exit(0);
#else
    return nullptr;
#endif
}

bool QNetworkClient::IsRunFinished() {
    DEBUG_ASSERT(__LOGTAG__, (runConfigMutex.tryLock(__FUNCTION__)==0), __FUNCTION__);
    bool retVal = runConfig.finished;
    DEBUG_ASSERT(__LOGTAG__, (runConfigMutex.unLock()==0), __FUNCTION__);
    return retVal;
}

int QNetworkClient::run(std::string host, std::string port) {
    DEBUG_ASSERT(__LOGTAG__, (runConfigMutex.tryLock(__FUNCTION__)==0), __FUNCTION__);
    runConfig.host = host;
    runConfig.port = port;
    runConfig.thiz = this;
    runConfig.finished = false;
    runConfig.id = QNetworkClient::connectionID++;
    DEBUG_ASSERT(__LOGTAG__, (runConfigMutex.unLock()==0), __FUNCTION__);
#if USE_PTHREAD
    if( pthread_create( &run_thread_id, nullptr,  QNetworkClient::run_internal, (void*)&runConfig) < 0)
    {
        DEBUG_PRINT_ERROR(__LOGTAG__, "could not create thread: %s - %d", strerror (errno), errno);
        return -1;
    }
#else
    QNetworkClient::run_internal((void*)&runConfig);
#endif
    return 0;
}
