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
#include <filesystem>

namespace fs = std::__fs::filesystem;

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

class QPeerConnection;
struct connections {
    int sock;
    struct sockaddr *local_addr = nullptr;
    socklen_t local_addr_len;
    QPeerConnection *h = nullptr;
};

class MQPeerConnectionBridge {
public:
    virtual void FlushEgress(struct ev_loop *loop, QPeerConnection* qconnection) = 0;
    virtual void DestroyConnection(struct ev_loop *loop, QPeerConnection* qconnection) = 0;
    virtual void OnConnection(QPeerConnection* qconnection) = 0;
    virtual void OnMessage(ssize_t recv_len, uint8_t* buf, QPeerConnection* qconnection) = 0;
    virtual void OnDestroyConnection(QPeerConnection* qconnection) = 0;
    inline virtual struct ev_loop * GetMainLoop() = 0;
};

class QPeerConnection {
public:
    QPeerConnection(MQPeerConnectionBridge* bridge, uint8_t *scid, size_t scid_len, int sock);
    ~QPeerConnection();
    
    void SendMessage(const char *buf, size_t buflen, bool flush);
    void SendMessage(const std::string& buffer, bool flush);
    
    MQPeerConnectionBridge* bridge = nullptr;
    uint8_t cid[LOCAL_CONN_ID_LEN];
    ev_timer timer;
    int sock;
    Connection *conn = nullptr;
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len;
    UT_hash_handle hh;
};

class QNetworkServer : protected MQPeerConnectionBridge {
public:
    int run(std::string host, std::string port, fs::path executablePath);
    void BroadCastMessage(const std::string& buffer, bool flush);
    
protected:
    void FlushEgress(struct ev_loop *loop, QPeerConnection* qconnection) override;
    void DestroyConnection(struct ev_loop *loop, QPeerConnection* qconnection) override;
    void OnMessage(ssize_t recv_len, uint8_t* buf, QPeerConnection* qconnection) override;
    void OnConnection(QPeerConnection* qconnection) override;
    void OnDestroyConnection(QPeerConnection* qconnection) override;
    inline struct ev_loop * GetMainLoop() override {
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
    QPeerConnection *create_conn(uint8_t *scid, size_t scid_len,
                                       uint8_t *odcid, size_t odcid_len,
                                       struct sockaddr *local_addr,
                                       socklen_t local_addr_len,
                                       struct sockaddr_storage *peer_addr,
                                socklen_t peer_addr_len);
    static void recv_cb(EV_P_ ev_io *w, int revents);
    void Recv_cb(EV_P_ ev_io *w, int revents);
    
    Config *config = nullptr;
    struct ev_loop *mainloop = nullptr;
    struct connections *conns = nullptr;
};

#endif /* QNetworkServer_hpp */
