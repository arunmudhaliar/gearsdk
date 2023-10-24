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
#include <vector>

#if USE_PTHREAD
#include <pthread.h>
#endif
#include <sstream>

#include "../../Common/SDKTypes.hpp"

extern "C" {
#include <quiche.h>
}

#undef __LOGTAG__
#define __LOGTAG__ "QNetworkClient"

#define LOCAL_CONN_ID_LEN 16

#define MAX_DATAGRAM_SIZE 1350

class QMutex;
class QMutexCondition {
public:
    QMutexCondition();
    ~QMutexCondition();
    int init(const std::string& name);
    int signal(const char* msg = nullptr);
    int broadcast(const char* msg = nullptr);
    int conditionWait(QMutex& qmutex, const char* msg);
    
private:
    int tryInitIfNot();
    bool inited = false;
    pthread_cond_t  cond;
    std::string name;
};

class QMutex {
public:
    QMutex();
    ~QMutex();
    int init(const std::string& name);
    int tryLock(const char* lockedBy, const char* msg = nullptr);
    int unLock(const char* msg = nullptr);
    inline pthread_mutex_t* getMutexInternal() {
        return &mutex;
    }
    void conditionalWait(const char* waiting_at);
    void block(const char* blockedBy = nullptr);
    void unBlock(const char* unblockedBy);
private:
    int tryInitIfNot();
    bool inited = false;
    bool allowTask = true;
    pthread_mutex_t mutex;
    std::string name;
    std::string blockedBy = "none";
    std::string lockedBy = "none";
    std::string waitingAT = "none";
    std::string unblockedBy = "none";
    QMutexCondition condition;
    long blockCount = 0;
    int wanted = 0;
};

struct QData {
    QData(uint8_t* _data, ssize_t sz, bool fin = false) : size(sz), fin(fin) {
        this->data = new uint8_t[sz];
        memcpy(this->data, _data, sz);
    }
    ~QData() {
        GX_DELETE_ARY(this->data);
    }
    uint8_t* data = nullptr;
    ssize_t size = 0;
    bool fin = false;
};

class MQCommandBridge;
class QConnection {
private:
    QConnection() {};
public:
    QConnection(MQCommandBridge* bridge, int id);
    QConnection(MQCommandBridge* bridge, Config *config, int id);
    ~QConnection();
    
    void SetConfig(Config *config) { this->config = config; }
    int id = -1;
    ev_timer timer;
    ev_timer sendTimer;
    ev_tstamp last_sendTime;
    ev_io watcher;
    int sock;
    struct sockaddr_storage local_addr;
    socklen_t local_addr_len;
    Connection *conn = nullptr;
    MQCommandBridge* bridge = nullptr;
    Config *config = nullptr;
    struct addrinfo *peer = nullptr;
    int Connect(std::string host, std::string port);
    ssize_t SendMessage(const char *buf, size_t buflen, bool fin);
    ssize_t SendMessage(const std::string& buffer, bool fin);
    int ConnectionActive();
    
    void Release();
    
    uint8_t recv_buf[65535];
    uint8_t egress_out[MAX_DATAGRAM_SIZE];
    
    std::vector<QData*> sendBuffer;
};

class MQConnectionBridge {
public:
    virtual void OnConnect(QConnection* qconnection) = 0;
    virtual void OnMessage(ssize_t recv_len, uint8_t* buf, QConnection* qconnection) = 0;
    virtual void OnReleaseConnection(QConnection* qconnection) = 0;
    virtual void OnClose(QConnection* qconnection) = 0;
};

enum CON_STATE {
    STATE_OPEN,
    STATE_CONNECT,
    STATE_CLOSE
};

class MQCommandBridge {
public:
    virtual void FlushEgress(struct ev_loop *loop, QConnection* qconnection) = 0;
    virtual int ReleaseConnection(struct ev_loop *loop, QConnection* qconnection) = 0;
    inline virtual struct ev_loop * GetMainLoop() = 0;
    
    virtual void Event_Connect(QConnection* qconnection) = 0;
    virtual void Event_MsgReceived(ssize_t recv_len, uint8_t* buf, QConnection* qconnection) = 0;
    virtual void Event_Close(QConnection* qconnection) = 0;
    virtual int SendMessage(const std::string& buffer, bool flush) = 0;
    virtual int Close() = 0;
    virtual CON_STATE GetState() = 0;
    
    virtual QMutex* Get_runMutex() = 0;
    virtual QMutex* Get_closeMutex() = 0;
//    virtual QMutex* Get_destroyMutex() = 0;
    virtual QMutex* Get_sendMutex() = 0;
    virtual QMutex* Get_sendLoopMutex() = 0;
//    virtual QMutex* Get_flushMutex() = 0;
//    virtual QMutex* Get_recvMutex() = 0;
};

class QNetworkClient : public MQCommandBridge, public MQConnectionBridge {
private:
    struct RunConfig {
        std::string host;
        std::string port;
        QNetworkClient* thiz;
        int pthread_returnValue;
        bool finished = false;
        int id = -1;
    };
    static int connectionID;
    struct ev_loop *mainloop = nullptr;
    struct RunConfig runConfig;
    
#if USE_PTHREAD
    QMutex runMutex;
    QMutex closeMutex;
//    QMutex destroyMutex;
    QMutex sendMutex;
    QMutex sendLoopMutex;
//    QMutex flushMutex;
    QMutex runConfigMutex;
//    QMutex recvMutex;
    pthread_t run_thread_id;
#endif

    static void debug_log(const uint8_t *line, void *argp);
    static void recv_cb(EV_P_ ev_io *w, int revents);
    static void timeout_cb(EV_P_ ev_timer *w, int revents);
    static void send_cb(EV_P_ ev_timer *w, int revents);
    static void* run_internal(void* data);

    void SetState(CON_STATE state);
    inline CON_STATE GetState() override final { return state; }
    inline bool IsOpen() { return state == STATE_OPEN; }
    inline bool IsClosed() { return state == STATE_CLOSE; }
    CON_STATE state = STATE_OPEN;
    
protected:
    void FlushEgress(struct ev_loop *loop, QConnection* qconnection) override final;
    int ReleaseConnection(struct ev_loop *loop, QConnection* qconnection) override final;
    void OnConnect(QConnection* qconnection) override;
    void OnMessage(ssize_t recv_len, uint8_t* buf, QConnection* qconnection) override;
    void OnReleaseConnection(QConnection* qconnection) override;
    void OnClose(QConnection* qconnection) override;
    inline struct ev_loop * GetMainLoop() override final {
        return mainloop;
    }
    void Event_Connect(QConnection* qconnection) override final;
    void Event_MsgReceived(ssize_t recv_len, uint8_t* buf, QConnection* qconnection) override final;
    void Event_Close(QConnection* qconnection) override final;
    
    QMutex* Get_runMutex() override final {
        return &runMutex;
    }
    QMutex* Get_closeMutex() override final {
        return &closeMutex;
    }
    QMutex* Get_sendLoopMutex() override final {
        return &sendLoopMutex;
    }
//    QMutex* Get_destroyMutex() override final {
//        return &destroyMutex;
//    }
    QMutex* Get_sendMutex() override final {
        return &sendMutex;
    }
//    QMutex* Get_flushMutex() override final {
//        return &flushMutex;
//    }
//    QMutex* Get_recvMutex() override final {
//        return &recvMutex;
//    }
    
    QConnection* qclientConnection = nullptr;
    
public:
    QNetworkClient();
    ~QNetworkClient();
    
    int SendMessage(const std::string& buffer, bool flush)  override final;
    int Close() override final;
//    bool IsClosed();
    
    bool IsRunFinished();
    
    int run(std::string host, std::string port);
    
    void ForceRelease();
    inline QMutex& GetRunConfigMutex() { return runConfigMutex; }
    // const RunConfig& GetRunConfig() { return runConfig; }
};
#endif /* QNetworkClient_hpp */
