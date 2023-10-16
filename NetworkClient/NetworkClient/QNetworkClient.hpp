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
    enum CON_STATE {
        STATE_OPEN,
        STATE_CONNECT,
        STATE_CLOSE
    };
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
    int SendMessage(const char *buf, size_t buflen, bool flush);
    int SendMessage(const std::string& buffer, bool flush);
    void SetState(CON_STATE state);
    inline bool IsOpen() { return state == STATE_OPEN; }
    inline bool IsClosed() { return state == STATE_CLOSE; }
    int ConnectionActive();
    
    uint8_t recv_buf[65535];
    uint8_t egress_out[MAX_DATAGRAM_SIZE];
    
private:
    CON_STATE state = STATE_OPEN;
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
    
    virtual void Event_Connect(QConnection* qconnection) = 0;
    virtual void Event_MsgReceived(ssize_t recv_len, uint8_t* buf, QConnection* qconnection) = 0;
    virtual void Event_Close(QConnection* qconnection) = 0;
    virtual int SendMessage(const std::string& buffer, bool flush) = 0;
    virtual int Close() = 0;
};

//struct RunConfig;
class QNetworkClient : public MQCommandBridge, public MQConnectionBridge {
private:
    struct RunConfig {
        std::string host;
        std::string port;
        QNetworkClient* thiz;
        int pthread_returnValue;
    };
    
    struct ev_loop *mainloop = nullptr;
    struct RunConfig runConfig;
    
    pthread_t run_thread_id;
    pthread_mutex_t run_mutex;
    pthread_mutex_t close_mutex;
    //pthread_mutex_t conn_mutex;
    //pthread_mutex_t recv_mutex;
    pthread_mutex_t send_mutex;
    
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
    void Event_Connect(QConnection* qconnection) override final;
    void Event_MsgReceived(ssize_t recv_len, uint8_t* buf, QConnection* qconnection) override final;
    void Event_Close(QConnection* qconnection) override final;
    
    QConnection* qclientConnection = nullptr;
    
public:
    QNetworkClient();
    ~QNetworkClient();
    
    int SendMessage(const std::string& buffer, bool flush)  override final;
    int Close() override final;
    
    int run(std::string host, std::string port);
};
#endif /* QNetworkClient_hpp */
