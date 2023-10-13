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
#include <map>
#include <algorithm>

#include "../../Common/SDKTypes.hpp"

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

class QConnection;
struct connections {
    int sock;

    struct sockaddr *local_addr = nullptr;
    socklen_t local_addr_len;

    QConnection *h = nullptr;
};

struct conn_io {

};


class MQConnectionBridge {
public:
    virtual void FlushEgress(struct ev_loop *loop, QConnection* qconnection) = 0;
    virtual void DestroyConnection(struct ev_loop *loop, QConnection* qconnection) = 0;
};

class QConnection {
public:
    QConnection(MQConnectionBridge* bridge, uint8_t *scid, size_t scid_len, int sock) :
    bridge(bridge),
    sock(sock)
    {
        if (scid_len != LOCAL_CONN_ID_LEN) {
            DEBUG_PRINT_WARN(__LOGTAG__, "failed, scid length too short");
        }

        memcpy(cid, scid, LOCAL_CONN_ID_LEN);
    }
    ~QConnection() {
    }
    
    MQConnectionBridge* bridge = nullptr;
    uint8_t cid[LOCAL_CONN_ID_LEN];
    ev_timer timer;
    int sock;
    Connection *conn = nullptr;

    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len;
    
    UT_hash_handle hh;
};

class QNetworkServer : public MQConnectionBridge {
public:
    int run(std::string host, std::string port);
    
protected:
    void FlushEgress(struct ev_loop *loop, QConnection* qconnection) override;
    void DestroyConnection(struct ev_loop *loop, QConnection* qconnection) override;
    
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
    QConnection *create_conn(uint8_t *scid, size_t scid_len,
                                       uint8_t *odcid, size_t odcid_len,
                                       struct sockaddr *local_addr,
                                       socklen_t local_addr_len,
                                       struct sockaddr_storage *peer_addr,
                                socklen_t peer_addr_len);
    static void recv_cb(EV_P_ ev_io *w, int revents);
    void Recv_cb(EV_P_ ev_io *w, int revents);
    
    Config *config = nullptr;
    struct connections *conns = nullptr;
};

#endif /* QNetworkServer_hpp */
