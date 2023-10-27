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

extern "C"
{
#include <quiche.h>
}

#if PLATFORM == PLATFORM_MAC
namespace fs = std::__fs::filesystem;
#elif PLATFORM == PLATFORM_LINUX
namespace fs = std::filesystem;
#else
namespace fs = std::__fs::filesystem;
#endif

#undef __LOGTAG__
#define __LOGTAG__ "qnetworkserver"

#define LOCAL_CONN_ID_LEN 16
#define MAX_DATAGRAM_SIZE 1350
#define MAX_TOKEN_LEN                     \
    sizeof("quiche") - 1 +                \
        sizeof(struct sockaddr_storage) + \
        MAX_CID_LEN

class qpeerconnection;
struct connections
{
    int sock;
    struct sockaddr *local_addr = nullptr;
    socklen_t local_addr_len;
    qpeerconnection *h = nullptr;
    uint8_t buf[65535];
    uint8_t out[MAX_DATAGRAM_SIZE];
};

class bridge_qpeerconnection
{
public:
    virtual void flush_egress(struct ev_loop *loop, qpeerconnection *qconnection) = 0;
    virtual void destroy_connection(struct ev_loop *loop, qpeerconnection *qconnection) = 0;
    virtual void on_connection(qpeerconnection *qconnection) = 0;
    virtual void on_message(ssize_t recv_len, uint8_t *buf, qpeerconnection *qconnection) = 0;
    virtual void on_destroy_connection(qpeerconnection *qconnection) = 0;
    inline virtual struct ev_loop *get_mainloop() = 0;
};

class qpeerconnection
{
public:
    qpeerconnection(bridge_qpeerconnection *bridge, uint8_t *scid, size_t scid_len, int sock);
    ~qpeerconnection();

    void sendmessage(const char *buf, size_t buflen, bool flush);
    void sendmessage(const std::string &buffer, bool flush);
    void close();

    bridge_qpeerconnection *bridge = nullptr;
    uint8_t cid[LOCAL_CONN_ID_LEN];
    unsigned cid_hash_val = 0;
    ev_timer timer;
    int sock;
    Connection *conn = nullptr;
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len;
    UT_hash_handle hh;
    int itrmsg = 0;

    uint8_t egress_out[MAX_DATAGRAM_SIZE];
};

class qnetworkserver : protected bridge_qpeerconnection
{
private:
    struct runserverconfig
    {
        std::string host;
        std::string port;
        qnetworkserver *thiz;
        int pthread_returnValue;
        bool finished = false;
        int id = -1;
        fs::path rootDir;
    };
    static int runID;

public:
    int run(std::string host, std::string port, fs::path executablePath);
    void BroadCastMessage(const std::string &buffer, bool flush);

protected:
    void flush_egress(struct ev_loop *loop, qpeerconnection *qconnection) override final;
    void destroy_connection(struct ev_loop *loop, qpeerconnection *qconnection) override final;
    void on_message(ssize_t recv_len, uint8_t *buf, qpeerconnection *qconnection) override;
    void on_connection(qpeerconnection *qconnection) override;
    void on_destroy_connection(qpeerconnection *qconnection) override;
    inline struct ev_loop *get_mainloop() override final
    {
        return mainloop;
    }

private:
    static void debug_log(const uint8_t *line, void *argp);
    static void timeout_cb(EV_P_ ev_timer *w, int revents);
    void mint_token(const uint8_t *dcid, size_t dcid_len,
                    struct sockaddr_storage *addr, socklen_t addr_len,
                    uint8_t *token, size_t *token_len);
    bool validate_token(const uint8_t *token, size_t token_len,
                        struct sockaddr_storage *addr, socklen_t addr_len,
                        uint8_t *odcid, size_t *odcid_len);
    uint8_t *gen_cid(uint8_t *cid, size_t cid_len);
    qpeerconnection *create_conn(uint8_t *scid, size_t scid_len,
                                 uint8_t *odcid, size_t odcid_len,
                                 struct sockaddr *local_addr,
                                 socklen_t local_addr_len,
                                 struct sockaddr_storage *peer_addr,
                                 socklen_t peer_addr_len);
    static void recv_cb(EV_P_ ev_io *w, int revents);
    void recv_cb_internal(EV_P_ ev_io *w, int revents);

    Config *config = nullptr;
    struct ev_loop *mainloop = nullptr;
    struct connections *conns = nullptr;

    static void *run_internal(void *data);

    struct runserverconfig run_server_config;
    qmutex run_mutex;
    qmutex runconfig_mutex;
    pthread_t run_thread_id;
};

#endif /* qnetworkserver_hpp */
