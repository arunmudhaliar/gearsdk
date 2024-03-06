//
//  qnetworkserver.cpp
//  NetworkServer
//
//  Created by Arun A on 12/10/23.
//

#include "qnetworkserver.hpp"

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

// MARK: - conn_io
int qnetworkserver::runID = 0;

conn_io::conn_io(bridge_qpeerconnection *bridge, uint8_t *scid, size_t scid_len, int sock) : bridge(bridge),
                                                                                                             sock(sock)
{
    if (scid_len != Q_LOCAL_CONN_ID_LEN)
    {
        DEBUG_PRINT_WARN(__LOGTAG__, "failed, scid length too short");
    }
    memcpy(cid, scid, Q_LOCAL_CONN_ID_LEN);
    HASH_VALUE(cid, Q_LOCAL_CONN_ID_LEN, cid_hash_val);
}

conn_io::~conn_io()
{
    ev_timer_stop(bridge->get_mainloop(), &timer);
    if (conn)
    {
        quiche_conn_free(conn);
    }
}

void conn_io::sendmessage(const qstring &buffer, bool flush)
{
    sendmessage(buffer.c_str(), buffer.length(), flush);
}

void conn_io::sendmessage(const char *buf, size_t buflen, bool flush)
{
    if (!quiche_conn_is_established(conn))
    {
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "Cant send !!!, connection not established - %s", (char *)buf);
        return;
    }
    bool success = false;
    uint64_t s = 0;
    StreamIter *writable = quiche_conn_writable(conn);
    while (quiche_stream_iter_next(writable, &s))
    {
        if (last_stream_s == s) {
            continue;
        }
        DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "stream %" PRIu64 " is writable", s);
        ssize_t sent_len = quiche_conn_stream_send(conn, s, (uint8_t *)buf,
                                                   buflen, false);
        if (sent_len != buflen)
        {
            DEBUG_PRINT_ERROR(__LOGTAG__, "send failure %d", sent_len);
            break;
        }
        success = true;
        last_stream_s = s;
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "--------->>>>>>>>>>>[%d] %s", s, (char *)buf);
        break;
    }
    
    const uint64_t MAX_SEND_STREAM_TO_TRY = 200;
    uint64_t next_s = last_stream_s;
    while (!success && next_s<MAX_SEND_STREAM_TO_TRY)
    {
        next_s = (next_s+1)+(next_s%2);
        ssize_t sent_len = quiche_conn_stream_send(conn, next_s, (uint8_t *)buf,
                                                   buflen, false);
        if (sent_len == buflen) {
            DEBUG_PRINT_IMPORTANT(__LOGTAG__, "--------->>>>>>>>>>>[%d] %s", next_s, (char *)buf);
            last_stream_s = next_s;
            success = true;
        }
    }
    
    if (!success && next_s>=MAX_SEND_STREAM_TO_TRY) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "Send %s FAILED even after %d tries. Streams not available to send !!!", (char *)buf, next_s);
    }
    quiche_stream_iter_free(writable);
    if (flush)
    {
        bridge->flush_egress(bridge->get_mainloop(), this);
    }
}

void conn_io::close()
{
    if (!quiche_conn_is_established(conn))
    {
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "Cant close !!!, connection not established.");
        return;
    }
    uint64_t s = 0;
    StreamIter *writable = quiche_conn_writable(conn);
    while (quiche_stream_iter_next(writable, &s))
    {
        DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "stream %" PRIu64 " is writable", s);
        const char *byez = "byez\n";
        ssize_t bye_sent_len = quiche_conn_stream_send(conn, s, (uint8_t *)byez,
                                                       5, true);
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "Close, sending 'byez'");
        if (bye_sent_len != 5)
        {
            DEBUG_PRINT_ERROR(__LOGTAG__, "sending 'byez' failed !!!");
        }

        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "--------->>>>>>>>>>>[%d] %s", s, (char *)byez);
        break;
    }
    quiche_stream_iter_free(writable);
    bridge->flush_egress(bridge->get_mainloop(), this);
}

// MARK: - qnetworkserver
void qnetworkserver::debug_log(const uint8_t *line, void *argp)
{
    DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "%s", (char *)line);
}
void qnetworkserver::mint_token(const uint8_t *dcid, size_t dcid_len,
                                struct sockaddr_storage *addr, socklen_t addr_len,
                                uint8_t *token, size_t *token_len)
{
    memcpy(token, "quiche", sizeof("quiche") - 1);
    memcpy(token + sizeof("quiche") - 1, addr, addr_len);
    memcpy(token + sizeof("quiche") - 1 + addr_len, dcid, dcid_len);

    *token_len = sizeof("quiche") - 1 + addr_len + dcid_len;
}

bool qnetworkserver::validate_token(const uint8_t *token, size_t token_len,
                                    struct sockaddr_storage *addr, socklen_t addr_len,
                                    uint8_t *odcid, size_t *odcid_len)
{
    if ((token_len < sizeof("quiche") - 1) ||
        memcmp(token, "quiche", sizeof("quiche") - 1))
    {
        return false;
    }

    token += sizeof("quiche") - 1;
    token_len -= sizeof("quiche") - 1;

    if ((token_len < addr_len) || memcmp(token, addr, addr_len))
    {
        return false;
    }

    token += addr_len;
    token_len -= addr_len;

    if (*odcid_len < token_len)
    {
        return false;
    }

    memcpy(odcid, token, token_len);
    *odcid_len = token_len;

    return true;
}

uint8_t *qnetworkserver::gen_cid(uint8_t *cid, size_t cid_len)
{
    int rng = open("/dev/urandom", O_RDONLY);
    if (rng < 0)
    {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to open /dev/urandom");
        return nullptr;
    }

    ssize_t rand_len = read(rng, cid, cid_len);
    if (rand_len < 0)
    {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create connection ID");
        close(rng);
        return nullptr;
    }

    close(rng);
    return cid;
}

conn_io *qnetworkserver::create_conn(uint8_t *scid, size_t scid_len,
                                             uint8_t *odcid, size_t odcid_len,
                                             struct sockaddr *local_addr,
                                             socklen_t local_addr_len,
                                             struct sockaddr_storage *peer_addr,
                                             socklen_t peer_addr_len)
{
    conn_io *qconnection = DEBUG_NEW conn_io(this, scid, scid_len, conns->sock);
    if (qconnection == nullptr)
    {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to allocate qconnection");
        GX_DELETE(qconnection);
        return nullptr;
    }

    Connection *conn = quiche_accept(qconnection->cid, Q_LOCAL_CONN_ID_LEN,
                                     odcid, odcid_len,
                                     local_addr,
                                     local_addr_len,
                                     (struct sockaddr *)peer_addr,
                                     peer_addr_len,
                                     config);

    if (conn == nullptr)
    {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create connection");
        GX_DELETE(qconnection);
        return nullptr;
    }

    qconnection->conn = conn;

    memcpy(&qconnection->peer_addr, peer_addr, peer_addr_len);
    qconnection->peer_addr_len = peer_addr_len;

    ev_init(&qconnection->timer, timeout_cb);
    qconnection->timer.data = qconnection;

    HASH_ADD(hh, conns->h, cid, Q_LOCAL_CONN_ID_LEN, qconnection);

    qconnection->bridge->onconnection_connect(qconnection);

    return qconnection;
}

void qnetworkserver::onconnection_connect(conn_io *qconnection)
{
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "++++++++++<<<<<<<<<<< new connection");
}
void qnetworkserver::onconnection_connected(conn_io* qconnection)
{
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "++++++++++<<<<<<<<<<< connection established");
}

void qnetworkserver::onconnection_message(ssize_t recv_len, uint8_t *buf, conn_io *qconnection)
{
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

    uint8_t *copybuf = DEBUG_NEW uint8_t[recv_len + 1];
    memcpy(copybuf, buf, recv_len);
    copybuf[recv_len] = '\0';
    if (getnameinfo((struct sockaddr *)&qconnection->peer_addr, qconnection->peer_addr_len, hbuf, sizeof(hbuf), sbuf,
                    sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV) == 0)
    {
        DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "---------<<<<<<<<<<< %s [len %d], host : %s, serv : %s", copybuf, recv_len, hbuf, sbuf);
    }
    else
    {
        DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "---------<<<<<<<<<<< %s [len %d]", copybuf, recv_len);
    }
    GX_DELETE_ARY(copybuf);

    // TODO : Comment this for development.
    /*
    qstring ss = qstring::format_string("HELLO from server-%d", qconnection->itrmsg++);
    qconnection->sendmessage(ss, true);
    */
}

void qnetworkserver::flush_egress(struct ev_loop *loop, conn_io *qconnection)
{
    SendInfo send_info;
    while (true)
    {
        ssize_t written = quiche_conn_send(qconnection->conn, qconnection->egress_out, sizeof(qconnection->egress_out),
                                           &send_info);

        if (written == QUICHE_ERR_DONE)
        {
            DEBUG_PRINT(LOG_LEVEL_4, __LOGTAG__, "done writing");
            break;
        }

        if (written < 0)
        {
            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create packet: %zd", written);
            return;
        }

        ssize_t sent = sendto(qconnection->sock, qconnection->egress_out, written, 0,
                              (struct sockaddr *)&send_info.to,
                              send_info.to_len);

        if (sent != written)
        {
            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to send");
            return;
        }

        DEBUG_PRINT(LOG_LEVEL_4, __LOGTAG__, "sent %zd bytes", sent);
    }

    uint64_t timeout_in_nanos = quiche_conn_timeout_as_nanos(qconnection->conn);
    double t = (double)timeout_in_nanos / 1e9f;
    qconnection->timer.repeat = t < 0.00001f ? 1.0f : t;
    ev_timer_again(loop, &qconnection->timer);
    DEBUG_PRINT(LOG_LEVEL_5, __LOGTAG__, "qconnection->timer.repeat %f - %" PRIu64 "", t, timeout_in_nanos);
}

void qnetworkserver::destroy_connection(struct ev_loop *loop, conn_io *qconnection)
{
    onconnection_destroy(qconnection);
    HASH_DELETE(hh, conns->h, qconnection);
    GX_DELETE(qconnection);
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "Connection destroyed [pending %d]!!!", HASH_CNT(hh, conns->h));
}

void qnetworkserver::onconnection_destroy(conn_io *qconnection)
{
    //    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "Connection about to destroy !!!");
}

void qnetworkserver::on_qhiredis_async_key_expired(const qstring&) {
    
}

void qnetworkserver::timeout_cb(EV_P_ ev_timer *w, int revents)
{
    conn_io *qconnection = (conn_io *)w->data;

    DEBUG_PRINT(LOG_LEVEL_4, __LOGTAG__, "timeout !!!");
    quiche_conn_on_timeout(qconnection->conn);
    qconnection->bridge->flush_egress(loop, qconnection);

    if (quiche_conn_is_closed(qconnection->conn))
    {
        Stats stats;
        PathStats path_stats;

        quiche_conn_stats(qconnection->conn, &stats);
        quiche_conn_path_stats(qconnection->conn, 0, &path_stats);

        DEBUG_PRINT(LOG_LEVEL_4, __LOGTAG__, "connection closed, recv=%zu sent=%zu lost=%zu rtt=%" PRIu64 "ns cwnd=%zu\n",
                    stats.recv, stats.sent, stats.lost, path_stats.rtt, path_stats.cwnd);

        qconnection->bridge->destroy_connection(loop, qconnection);
        return;
    } /*else {

        // force close here
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "Force close connection !!!");
        qconnection->bridge->DestroyConnection(loop, qconnection);
        return;
    }*/
}

void qnetworkserver::recv_cb_internal(EV_P_ ev_io *w, int revents)
{
    conn_io *qconnection = nullptr;
    conn_io *tmp = nullptr;

    while (true)
    {
        struct sockaddr_storage peer_addr;
        socklen_t peer_addr_len = sizeof(peer_addr);
        memset(&peer_addr, 0, peer_addr_len);

        ssize_t read = recvfrom(conns->sock, conns->buf, sizeof(conns->buf), 0,
                                (struct sockaddr *)&peer_addr,
                                &peer_addr_len);

        if (read < 0)
        {
            if ((errno == EWOULDBLOCK) || (errno == EAGAIN))
            {
                DEBUG_PRINT(LOG_LEVEL_4, __LOGTAG__, "recv would block");
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

        int rc = quiche_header_info(conns->buf, read, Q_LOCAL_CONN_ID_LEN, &version,
                                    &type, scid, &scid_len, dcid, &dcid_len,
                                    token, &token_len);
        if (rc < 0)
        {
            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to parse header: %d", rc);
            continue;
        }

        HASH_FIND(hh, conns->h, dcid, dcid_len, qconnection);

        if (qconnection == nullptr)
        {
            if (!quiche_version_is_supported(version))
            {
                DEBUG_PRINT(LOG_LEVEL_4, __LOGTAG__, "version negotiation");

                ssize_t written = quiche_negotiate_version(scid, scid_len,
                                                           dcid, dcid_len,
                                                           conns->out, sizeof(conns->out));

                if (written < 0)
                {
                    DEBUG_PRINT_WARN(__LOGTAG__, "failed to create vneg packet: %zd",
                                     written);
                    continue;
                }

                ssize_t sent = sendto(conns->sock, conns->out, written, 0,
                                      (struct sockaddr *)&peer_addr,
                                      peer_addr_len);
                if (sent != written)
                {
                    DEBUG_PRINT_ERROR(__LOGTAG__, "failed to send");
                    continue;
                }

                DEBUG_PRINT(LOG_LEVEL_4, __LOGTAG__, "sent %zd bytes", sent);
                continue;
            }

            if (token_len == 0)
            {
                DEBUG_PRINT(LOG_LEVEL_4, __LOGTAG__, "stateless retry");

                mint_token(dcid, dcid_len, &peer_addr, peer_addr_len,
                           token, &token_len);

                uint8_t new_cid[Q_LOCAL_CONN_ID_LEN];

                if (gen_cid(new_cid, Q_LOCAL_CONN_ID_LEN) == nullptr)
                {
                    continue;
                }

                ssize_t written = quiche_retry(scid, scid_len,
                                               dcid, dcid_len,
                                               new_cid, Q_LOCAL_CONN_ID_LEN,
                                               token, token_len,
                                               version, conns->out, sizeof(conns->out));

                if (written < 0)
                {
                    DEBUG_PRINT_WARN(__LOGTAG__, "failed to create retry packet: %zd",
                                     written);
                    continue;
                }

                ssize_t sent = sendto(conns->sock, conns->out, written, 0,
                                      (struct sockaddr *)&peer_addr,
                                      peer_addr_len);
                if (sent != written)
                {
                    DEBUG_PRINT_ERROR(__LOGTAG__, "failed to send");
                    continue;
                }

                DEBUG_PRINT(LOG_LEVEL_4, __LOGTAG__, "sent %zd bytes", sent);
                continue;
            }

            if (!validate_token(token, token_len, &peer_addr, peer_addr_len,
                                odcid, &odcid_len))
            {
                DEBUG_PRINT_WARN(__LOGTAG__, "invalid address validation token");
                continue;
            }

            qconnection = create_conn(dcid, dcid_len, odcid, odcid_len,
                                      conns->local_addr, conns->local_addr_len,
                                      &peer_addr, peer_addr_len);

            if (qconnection == nullptr)
            {
                continue;
            }
        }

        RecvInfo recv_info = {
            (struct sockaddr *)&peer_addr,
            peer_addr_len,

            conns->local_addr,
            conns->local_addr_len,
        };

        ssize_t done = quiche_conn_recv(qconnection->conn, conns->buf, read, &recv_info);

        if (done < 0)
        {
            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to process packet: %zd", done);
            continue;
        }

        DEBUG_PRINT(LOG_LEVEL_4, __LOGTAG__, "recv %zd bytes", done);

        if (quiche_conn_is_established(qconnection->conn))
        {
            if (!qconnection->connection_established) {
                qconnection->connection_established = true;
                qconnection->bridge->onconnection_connected(qconnection);
            }
            uint64_t s = 0;
            StreamIter *readable = quiche_conn_readable(qconnection->conn);
            while (quiche_stream_iter_next(readable, &s))
            {
                DEBUG_PRINT(LOG_LEVEL_4, __LOGTAG__, "stream %" PRIu64 " is readable", s);
                bool fin = false;
                ssize_t recv_len = quiche_conn_stream_recv(qconnection->conn, s,
                                                           conns->buf, sizeof(conns->buf),
                                                           &fin);
                if (recv_len < 0)
                {
                    break;
                }
                if (fin)
                {
                    const char *resp = "byez\n";
                    ssize_t bye_sent_len = quiche_conn_stream_send(qconnection->conn, s, (uint8_t *)resp,
                                                                   5, true);
                    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "fin received, sending 'byez' - %0x", qconnection->cid_hash_val);
                    if (bye_sent_len != 5)
                    {
                        DEBUG_PRINT_ERROR(__LOGTAG__, "sending 'byez' failed !!!");
                    }
                }
                //                DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "\n\nREACHED ---> %s", buf);
                qconnection->bridge->onconnection_message(recv_len, conns->buf, qconnection);
            }
            quiche_stream_iter_free(readable);
        }
    }

    HASH_ITER(hh, conns->h, qconnection, tmp)
    {
        flush_egress(loop, qconnection);

        if (quiche_conn_is_closed(qconnection->conn))
        {
            Stats stats;
            PathStats path_stats;

            quiche_conn_stats(qconnection->conn, &stats);
            quiche_conn_path_stats(qconnection->conn, 0, &path_stats);

            DEBUG_PRINT_IMPORTANT(__LOGTAG__, "connection closed, recv=%zu sent=%zu lost=%zu rtt=%" PRIu64 "ns cwnd=%zu",
                                  stats.recv, stats.sent, stats.lost, path_stats.rtt, path_stats.cwnd);

            destroy_connection(loop, qconnection);
        }
    }
}

void qnetworkserver::broadcast_message(const qstring &buffer, bool flush)
{
    conn_io *qconnection = nullptr;
    conn_io *tmp = nullptr;
    HASH_ITER(hh, conns->h, qconnection, tmp)
    {
        if (quiche_conn_is_established(qconnection->conn))
        {
            qconnection->sendmessage(buffer, flush);
        }
    }
}

void qnetworkserver::recv_cb(EV_P_ ev_io *w, int revents)
{
    qnetworkserver *server = (qnetworkserver *)w->data;
    server->recv_cb_internal(loop, w, revents);
}

void qnetworkserver::network_server_begin() {
    on_network_server_begin();
    on_network_server_init();
}
void qnetworkserver::network_server_end() {
    on_network_server_end();
}
void *qnetworkserver::run_internal(void *data)
{
    runserverconfig *runConfig = (runserverconfig *)data;
    qstring host = runConfig->host;
    qstring port = runConfig->port;
    qnetworkserver *thiz = runConfig->thiz;
    thiz->host_id = host;
    thiz->port_id = port;
    
    if (thiz->run_mutex.tryLock(__FUNCTION__) != 0)
    {
        runConfig->finished = true;
        runConfig->pthread_returnValue = -1;
        pthread_exit(&runConfig->pthread_returnValue);
    }
    

    GX_DELETE(thiz->hiredis);
    thiz->hiredis = DEBUG_NEW qhiredis(runConfig->redis_ip, runConfig->redis_port);
    if (thiz->hiredis->connect_redis()!=0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to connect hiredis, Exiting !!!");
        GX_DELETE(thiz->hiredis);
        runConfig->finished = true;
        runConfig->pthread_returnValue = -1;
        pthread_exit(&runConfig->pthread_returnValue);
    }
    
    thiz->hiredis->set_hash_value("gservers", "gameserver", qstring::format_string("%s:%s", host.c_str(), port.c_str()));
    
    const struct addrinfo hints = {
        .ai_family = PF_UNSPEC,
        .ai_socktype = SOCK_DGRAM,
        .ai_protocol = IPPROTO_UDP};

    qstring log_path = qstring::format_string("./glogs/%s/qh3_logfile", port.c_str());
    thiz->logger.start_session(log_path, log_path.length());
//    quiche_enable_debug_logging(debug_log, nullptr);

    struct addrinfo *local;
    if (getaddrinfo(host.c_str(), port.c_str(), &hints, &local) != 0)
    {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to resolve host");
        GX_DELETE(thiz->hiredis);
        runConfig->pthread_returnValue = -1;
        runConfig->finished = true;
        DEBUG_ASSERT(__LOGTAG__, (thiz->run_mutex.unLock() == 0), "CHECK !!!");
        pthread_exit(&runConfig->pthread_returnValue);
    }

    int sock = socket(local->ai_family, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        freeaddrinfo(local);
        GX_DELETE(thiz->hiredis);
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create socket");
        runConfig->pthread_returnValue = -1;
        runConfig->finished = true;
        DEBUG_ASSERT(__LOGTAG__, (thiz->run_mutex.unLock() == 0), "CHECK !!!");
        pthread_exit(&runConfig->pthread_returnValue);
    }

    if (fcntl(sock, F_SETFL, O_NONBLOCK) != 0)
    {
        freeaddrinfo(local);
        GX_DELETE(thiz->hiredis);
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to make socket non-blocking");
        runConfig->pthread_returnValue = -1;
        runConfig->finished = true;
        DEBUG_ASSERT(__LOGTAG__, (thiz->run_mutex.unLock() == 0), "CHECK !!!");
        pthread_exit(&runConfig->pthread_returnValue);
    }

    if (bind(sock, local->ai_addr, local->ai_addrlen) < 0)
    {
        freeaddrinfo(local);
        GX_DELETE(thiz->hiredis);
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to connect socket");
        runConfig->pthread_returnValue = -1;
        runConfig->finished = true;
        DEBUG_ASSERT(__LOGTAG__, (thiz->run_mutex.unLock() == 0), "CHECK !!!");
        pthread_exit(&runConfig->pthread_returnValue);
    }

    thiz->config = quiche_config_new(PROTOCOL_VERSION);
    if (thiz->config == nullptr)
    {
        freeaddrinfo(local);
        GX_DELETE(thiz->hiredis);
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create config");
        runConfig->pthread_returnValue = -1;
        runConfig->finished = true;
        DEBUG_ASSERT(__LOGTAG__, (thiz->run_mutex.unLock() == 0), "CHECK !!!");
        pthread_exit(&runConfig->pthread_returnValue);
    }

    fs::path certFile(runConfig->rootDir / "cert.crt");
    fs::path keyFile(runConfig->rootDir / "cert.key");
    DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "cert file %s, key file %s", certFile.c_str(), keyFile.c_str());
    int res_crt_load = quiche_config_load_cert_chain_from_pem_file(thiz->config, certFile.c_str());
    if (res_crt_load!=0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "CERT load error - %s", certFile.c_str());
    }
    int res_key_load = quiche_config_load_priv_key_from_pem_file(thiz->config, keyFile.c_str());
    if (res_key_load!=0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "KEY load error - %s", keyFile.c_str());
    }
    
    quiche_config_set_application_protos(thiz->config,
                                         (uint8_t *)"\x0ahq-interop\x05hq-29\x05hq-28\x05hq-27\x08http/0.9", 38);

    quiche_config_set_max_idle_timeout(thiz->config, 30000);
    quiche_config_set_max_recv_udp_payload_size(thiz->config, Q_MAX_DATAGRAM_SIZE);
    quiche_config_set_max_send_udp_payload_size(thiz->config, Q_MAX_DATAGRAM_SIZE);
    quiche_config_set_initial_max_data(thiz->config, 10000000);
    quiche_config_set_initial_max_stream_data_bidi_local(thiz->config, 1000000);
    quiche_config_set_initial_max_stream_data_bidi_remote(thiz->config, 1000000);
    quiche_config_set_initial_max_streams_bidi(thiz->config, 100);
    quiche_config_set_cc_algorithm(thiz->config, Reno);

    struct connections c;
    c.sock = sock;
    c.h = nullptr;
    c.local_addr = local->ai_addr;
    c.local_addr_len = local->ai_addrlen;

    thiz->conns = &c;

    ev_io watcher;

    thiz->mainloop = ev_loop_new(0);

    thiz->hiredis_async = DEBUG_NEW qhiredis_async(runConfig->redis_ip, runConfig->redis_port, thiz);
    if (thiz->hiredis_async->connect_async_redis(thiz->mainloop)!=0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to connect async hiredis, Exiting !!!");
        GX_DELETE(thiz->hiredis_async);
        ev_loop_destroy(thiz->mainloop);
        freeaddrinfo(local);
        GX_DELETE(thiz->hiredis);
        runConfig->finished = true;
        runConfig->pthread_returnValue = -1;
        pthread_exit(&runConfig->pthread_returnValue);
    }
    
    ev_io_init(&watcher, recv_cb, sock, EV_READ);
    ev_io_start(thiz->mainloop, &watcher);
    watcher.data = thiz;

    thiz->network_server_begin();
    
    ev_loop(thiz->mainloop, 0);
    
    thiz->network_server_end();
    
    thiz->hiredis_async->disconnect_async_redis();
    GX_DELETE(thiz->hiredis_async);
    GX_DELETE(thiz->hiredis);
    
    ev_loop_destroy(thiz->mainloop);
    
    freeaddrinfo(local);

    quiche_config_free(thiz->config);

    thiz->logger.end_session();
    
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "waiting for services to finish !!!");
    struct ev_loop* wait_loop = ev_loop_new();
    qtimer_sceduler wait_scheduler;
    wait_scheduler.set_ev_lopp(wait_loop);
    qtimer* wait_timer = wait_scheduler.schedule_repeat_timer([thiz, wait_loop](qtimer& timer) {
        int service_shutdown_cnt = 0;
        if (thiz->logger.config.finished) {
            DEBUG_PRINT_IMPORTANT(__LOGTAG__, "stats service finished !!!");
            service_shutdown_cnt++;
        }
        if (service_shutdown_cnt >= 1) {
            ev_break(wait_loop, EVBREAK_ONE);
        }
    }, 3);
    ev_run(wait_loop, 0);
    wait_scheduler.cancel_and_destroy_timer(wait_timer);
    ev_loop_destroy(wait_loop);
    
    runConfig->finished = true;
    return 0;
}

bool qnetworkserver::is_run() {
    DEBUG_ASSERT(__LOGTAG__, (runconfig_mutex.tryLock(__FUNCTION__) == 0), __FUNCTION__);
    bool is_run = run_server_config.finished;
    DEBUG_ASSERT(__LOGTAG__, (runconfig_mutex.unLock() == 0), __FUNCTION__);
    return is_run;
}

int qnetworkserver::run(qstring host, qstring port, fs::path rootDir, const qstring& redis_ip, const uint16_t redis_port)
{
    DEBUG_ASSERT(__LOGTAG__, (runconfig_mutex.tryLock(__FUNCTION__) == 0), __FUNCTION__);
    run_server_config.host = host;
    run_server_config.port = port;
    run_server_config.redis_ip = redis_ip;
    run_server_config.redis_port = redis_port;
    run_server_config.thiz = this;
    run_server_config.finished = false;
    run_server_config.rootDir = rootDir;
    run_server_config.id = qnetworkserver::runID++;
    DEBUG_ASSERT(__LOGTAG__, (runconfig_mutex.unLock() == 0), __FUNCTION__);
    if (pthread_create(&run_thread_id, nullptr, qnetworkserver::run_internal, (void *)&run_server_config) < 0)
    {
        DEBUG_PRINT_ERROR(__LOGTAG__, "could not create thread: %s - %d", strerror(errno), errno);
        return -1;
    }
    return 0;
}
