//
//  qnetworkserver.hpp
//  NetworkServer
//
//  Created by Arun A on 12/10/23.
//

#ifndef qnetworkserver_hpp
#define qnetworkserver_hpp

#include <stdio.h>
#include <ev.h>
#include <uthash.h>
#include <string>
#include <map>
#include <algorithm>
#include <filesystem>

#include "../../common/sdktypes.hpp"
#include "../../networkcommon/source/essentials.hpp"
#include "../../networkcommon/source/qtextfilelogger.hpp"
#include "../../qhiredis/source/qhiredis.hpp"

extern "C"
{
#include <quiche.h>
}

#undef __LOGTAG__
#define __LOGTAG__ "qnetworkserver"

#define Q_LOCAL_CONN_ID_LEN 16
#define Q_MAX_DATAGRAM_SIZE 1350
#define MAX_TOKEN_LEN                     \
    sizeof("quiche") - 1 +                \
        sizeof(struct sockaddr_storage) + \
        MAX_CID_LEN

class conn_io;
struct connections {
    int sock;
    struct sockaddr* local_addr = nullptr;
    socklen_t local_addr_len;
    conn_io* h = nullptr;
    uint8_t buf[65535];
    uint8_t out[Q_MAX_DATAGRAM_SIZE];
};

class bridge_qpeerconnection {
public:
    virtual void flush_egress(struct ev_loop* loop, conn_io* qconnection) = 0;
    virtual void destroy_connection(struct ev_loop* loop, conn_io* qconnection) = 0;
    virtual void onconnection_connect(conn_io* qconnection) = 0;
    virtual void onconnection_connected(conn_io* qconnection) = 0;
    virtual void onconnection_message(ssize_t recv_len, uint8_t* buf, conn_io* qconnection) = 0;
    virtual void onconnection_destroy(conn_io* qconnection) = 0;
    inline virtual struct ev_loop* get_mainloop() = 0;
};

class conn_io {
public:
    conn_io(bridge_qpeerconnection* bridge, uint8_t* scid, size_t scid_len, int sock);
    ~conn_io();

    void sendmessage(const char* buf, size_t buflen, bool flush);
    void sendmessage(const qstring& buffer, bool flush);
    void close();

    bridge_qpeerconnection* bridge = nullptr;
    uint8_t cid[Q_LOCAL_CONN_ID_LEN];
    unsigned cid_hash_val = 0;
    ev_timer timer;
    int sock;
    Connection* conn = nullptr;
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len;
    UT_hash_handle hh;
    int itrmsg = 0;
    bool connection_established = false;
    uint8_t egress_out[Q_MAX_DATAGRAM_SIZE];
    uint64_t last_stream_s = 0;
};

class qnetworkserver : protected bridge_qpeerconnection {
private:
    struct runserverconfig {
        qstring host;
        qstring port;
        qnetworkserver* thiz;
        int pthread_returnValue;
        bool finished = false;
        int id = -1;
        fs::path rootDir;
        qstring redis_ip;
        uint16_t redis_port;
    };
    static int runID;

public:
    int run(qstring host, qstring port, fs::path executablePath, const qstring& redis_ip, const uint16_t redis_port);
    void broadcast_message(const qstring& buffer, bool flush);
    void network_server_begin();
    void network_server_end();

protected:
    virtual void on_network_server_begin() = 0;
    virtual void on_network_server_end() = 0;
    void flush_egress(struct ev_loop* loop, conn_io* qconnection) override final;
    void destroy_connection(struct ev_loop* loop, conn_io* qconnection) override final;
    void onconnection_message(ssize_t recv_len, uint8_t* buf, conn_io* qconnection) override;
    void onconnection_connect(conn_io* qconnection) override;
    void onconnection_connected(conn_io* qconnection) override;
    void onconnection_destroy(conn_io* qconnection) override;
    inline struct ev_loop* get_mainloop() override final {
        return mainloop;
    }

    qtextfilelogger logger;
private:
    static void debug_log(const uint8_t* line, void* argp);
    static void timeout_cb(EV_P_ ev_timer* w, int revents);
    void mint_token(const uint8_t* dcid, size_t dcid_len,
        struct sockaddr_storage* addr, socklen_t addr_len,
        uint8_t* token, size_t* token_len);
    bool validate_token(const uint8_t* token, size_t token_len,
        struct sockaddr_storage* addr, socklen_t addr_len,
        uint8_t* odcid, size_t* odcid_len);
    uint8_t* gen_cid(uint8_t* cid, size_t cid_len);
    conn_io* create_conn(uint8_t* scid, size_t scid_len,
        uint8_t* odcid, size_t odcid_len,
        struct sockaddr* local_addr,
        socklen_t local_addr_len,
        struct sockaddr_storage* peer_addr,
        socklen_t peer_addr_len);
    static void recv_cb(EV_P_ ev_io* w, int revents);
    void recv_cb_internal(EV_P_ ev_io* w, int revents);

    Config* config = nullptr;
    struct ev_loop* mainloop = nullptr;
    struct connections* conns = nullptr;

    static void* run_internal(void* data);

    struct runserverconfig run_server_config;
    qmutex run_mutex;
    qmutex runconfig_mutex;
    pthread_t run_thread_id;
    qhiredis* hiredis = nullptr;
};

#endif /* qnetworkserver_hpp */
