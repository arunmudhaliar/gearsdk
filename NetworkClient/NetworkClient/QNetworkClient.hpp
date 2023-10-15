//
//  QNetworkClient.hpp
//  NetworkClient
//
//  Created by Arun A on 12/10/23.
//

#ifndef QNetworkClient_hpp
#define QNetworkClient_hpp

#include <stdio.h>
#include <ev.h>
#include <uthash.h>
#include <string>
#include <pthread.h>

#include "../../Common/SDKTypes.hpp"

extern "C" {
#include <quiche.h>
}

#undef __LOGTAG__
#define __LOGTAG__ "QNetworkClient"

#define LOCAL_CONN_ID_LEN 16

#define MAX_DATAGRAM_SIZE 1350

class MQCommandBridge;
class QConnection {
public:
    QConnection(MQCommandBridge* bridge, Config *config);
    ~QConnection();
    ev_timer timer;
    int sock;
    struct sockaddr_storage local_addr;
    socklen_t local_addr_len;
    Connection *conn = nullptr;
    MQCommandBridge* bridge = nullptr;
    Config *config = nullptr;
    struct addrinfo *peer = nullptr;
    int Connect(std::string host, std::string port);
    void SendMessage(const char *buf, size_t buflen, bool flush);
    void SendMessage(const std::string& buffer, bool flush);
};

class MQConnectionBridge {
public:
    virtual void OnConnect(QConnection* qconnection) = 0;
    virtual void OnMessage(ssize_t recv_len, uint8_t* buf, QConnection* qconnection) = 0;
    virtual void OnDestroyConnection(QConnection* qconnection) = 0;
    virtual void OnClose(QConnection* qconnection) = 0;
};

class MQCommandBridge {
public:
    virtual void FlushEgress(struct ev_loop *loop, QConnection* qconnection) = 0;
    virtual void DestroyConnection(struct ev_loop *loop, QConnection* qconnection) = 0;
    inline virtual struct ev_loop * GetMainLoop() = 0;
    
    virtual void Connect(QConnection* qconnection) = 0;
    virtual void MessageReceived(ssize_t recv_len, uint8_t* buf, QConnection* qconnection) = 0;
    virtual void Close(QConnection* qconnection) = 0;
};

struct RunConfig;
class QNetworkClient : public MQCommandBridge, public MQConnectionBridge {
private:
    enum CON_STATE {
        STATE_OPEN,
        STATE_CONNECT,
        STATE_CLOSE
    };
    struct ev_loop *mainloop = nullptr;
    static RunConfig runConfig;
    
    pthread_t run_thread_id;
    pthread_mutex_t run_mutex;
    pthread_mutex_t conn_mutex;
    CON_STATE state = STATE_OPEN;
    
    static void debug_log(const uint8_t *line, void *argp);
//    void flush_egress(struct ev_loop *loop, struct conn_io *conn_io);
    static void recv_cb(EV_P_ ev_io *w, int revents);
    static void timeout_cb(EV_P_ ev_timer *w, int revents);
    static void* run_internal(void* data);
    
protected:
    void FlushEgress(struct ev_loop *loop, QConnection* qconnection) override final;
    void DestroyConnection(struct ev_loop *loop, QConnection* qconnection) override final;
    void OnConnect(QConnection* qconnection) override;
    void OnMessage(ssize_t recv_len, uint8_t* buf, QConnection* qconnection) override;
    void OnDestroyConnection(QConnection* qconnection) override;
    void OnClose(QConnection* qconnection) override;
    inline struct ev_loop * GetMainLoop() override final {
        return mainloop;
    }
    void Connect(QConnection* qconnection) override final;
    void MessageReceived(ssize_t recv_len, uint8_t* buf, QConnection* qconnection) override final;
    void Close(QConnection* qconnection) override final;
    
    QConnection* qclientConnection = nullptr;
    
public:
    QNetworkClient();
    ~QNetworkClient();
    
    void SendMessage(const std::string& buffer, bool flush);
    
    int run(std::string host, std::string port);
};

struct RunConfig {
    std::string host;
    std::string port;
    QNetworkClient* thiz;
    int pthread_returnValue;
};
#endif /* QNetworkClient_hpp */
