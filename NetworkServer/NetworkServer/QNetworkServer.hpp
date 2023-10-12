//
//  QNetworkServer.hpp
//  NetworkServer
//
//  Created by Arun A on 12/10/23.
//

#ifndef QNetworkServer_hpp
#define QNetworkServer_hpp

#include <stdio.h>

#include <stdio.h>
#include <ev.h>
#include <uthash.h>
#include <string>

extern "C" {
#include <quiche.h>
}

#undef __LOGTAG__
#define __LOGTAG__ "QNetworkServer"

#define LOCAL_CONN_ID_LEN 16

#define MAX_DATAGRAM_SIZE 1350

#define MAX_TOKEN_LEN \
    sizeof("quiche") - 1 + \
    sizeof(struct sockaddr_storage) + \
    MAX_CID_LEN

struct connections {
    int sock;

    struct sockaddr *local_addr;
    socklen_t local_addr_len;

    struct conn_io *h;
};

struct conn_io {
    ev_timer timer;

    int sock;

    uint8_t cid[LOCAL_CONN_ID_LEN];

    Connection *conn;

    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len;

    UT_hash_handle hh;
};

class QNetworkServer {
public:
    static int run(std::string host, std::string port);
    
private:
    static void debug_log(const uint8_t *line, void *argp);
    static void timeout_cb(EV_P_ ev_timer *w, int revents);
    static void flush_egress(struct ev_loop *loop, struct conn_io *conn_io);
    static void mint_token(const uint8_t *dcid, size_t dcid_len,
                           struct sockaddr_storage *addr, socklen_t addr_len,
                    uint8_t *token, size_t *token_len);
    static bool validate_token(const uint8_t *token, size_t token_len,
                               struct sockaddr_storage *addr, socklen_t addr_len,
                        uint8_t *odcid, size_t *odcid_len);
    static uint8_t *gen_cid(uint8_t *cid, size_t cid_len);
    static struct conn_io *create_conn(uint8_t *scid, size_t scid_len,
                                       uint8_t *odcid, size_t odcid_len,
                                       struct sockaddr *local_addr,
                                       socklen_t local_addr_len,
                                       struct sockaddr_storage *peer_addr,
                                socklen_t peer_addr_len);
    static void recv_cb(EV_P_ ev_io *w, int revents);
    
    static Config *config;
    static struct connections *conns;
};

#endif /* QNetworkServer_hpp */
