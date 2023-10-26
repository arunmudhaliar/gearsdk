//
//  QNetworkServer.hpp
//  NetworkServer
//
//  Created by Arun A on 12/10/23.
//

#ifndef QNetworkServer_hpp
#define QNetworkServer_hpp

#include <stdio.h>
#include <ev.h>
#include <uthash.h>
#include <string>
#include <map>
#include <algorithm>
#include <filesystem>

#include "../../Common/SDKTypes.hpp"
#include "../../NetworkCommon/Source/essentials.hpp"

extern "C" {
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
    uint8_t buf[65535];
    uint8_t out[MAX_DATAGRAM_SIZE];
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
    void Close();
    
    MQPeerConnectionBridge* bridge = nullptr;
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

class QNetworkServer : protected MQPeerConnectionBridge {
private:
    struct RunServerConfig {
        std::string host;
        std::string port;
        QNetworkServer* thiz;
        int pthread_returnValue;
        bool finished = false;
        int id = -1;
        fs::path rootDir;
    };
    static int runID;
public:
    int run(std::string host, std::string port, fs::path executablePath);
    void BroadCastMessage(const std::string& buffer, bool flush);
    
protected:
    void FlushEgress(struct ev_loop *loop, QPeerConnection* qconnection) override final;
    void DestroyConnection(struct ev_loop *loop, QPeerConnection* qconnection) override final;
    void OnMessage(ssize_t recv_len, uint8_t* buf, QPeerConnection* qconnection) override;
    void OnConnection(QPeerConnection* qconnection) override;
    void OnDestroyConnection(QPeerConnection* qconnection) override;
    inline struct ev_loop * GetMainLoop() override final {
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
    
    static void* run_internal(void* data);
    
    struct RunServerConfig runServerConfig;
    QMutex runMutex;
    QMutex runConfigMutex;
    pthread_t run_thread_id;
};

#endif /* QNetworkServer_hpp */
