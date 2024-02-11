//
//  qnetworkclient.hpp
//  networkclient
//
//  Created by Arun A on 12/10/23.
//

#ifndef qnetworkclient_hpp
#define qnetworkclient_hpp

#include <stdio.h>
#include <ev.h>
#include <uthash.h>
#include <string>
#include <vector>
#include <sstream>

#if USE_PTHREAD
#include <pthread.h>
#endif

#include "../../common/sdktypes.hpp"
#include "../../networkcommon/source/essentials.hpp"

extern "C"
{
#include <quiche.h>
}

#undef __LOGTAG__
#define __LOGTAG__ "qnetworkclient"

#define Q_LOCAL_CONN_ID_LEN 16
#define Q_MAX_DATAGRAM_SIZE 1350

namespace client {
struct qdata {
    qdata(const uint8_t* _data, ssize_t sz, bool fin = false) : size(sz), fin(fin) {
        this->data = new uint8_t[sz];
        memcpy(this->data, _data, sz);
    }
    ~qdata() {
        GX_DELETE_ARY(this->data);
    }
    uint8_t* data = nullptr;
    ssize_t size = 0;
    bool fin = false;
};

class bridge_qcommand;
class conn_io_client {
private:
    conn_io_client() {};

public:
    conn_io_client(bridge_qcommand* bridge, int id);
    conn_io_client(bridge_qcommand* bridge, Config* config, int id);
    ~conn_io_client();

    void SetConfig(Config* config) { this->config = config; }
    int id = -1;
    ev_timer timer;
    ev_timer sendTimer;
    ev_io watcher;
    int sock;
    struct sockaddr_storage local_addr;
    socklen_t local_addr_len;
    Connection* conn = nullptr;
    bridge_qcommand* bridge = nullptr;
    Config* config = nullptr;
    struct addrinfo* peer = nullptr;
    int Connect(qstring host, qstring port);
    ssize_t SendMessage(const char* buf, size_t buflen, bool fin);
    ssize_t SendMessage(const qstring& buffer, bool fin);
    int ConnectionActive();
    void Release();

    uint8_t recv_buf[65535];
    uint8_t egress_out[Q_MAX_DATAGRAM_SIZE];

    std::vector<qdata*> sendBuffer;
};

class bridge_qconnection {
public:
    virtual void onconnect(conn_io_client* qconnection) = 0;
    virtual void onmessage(ssize_t recv_len, uint8_t* buf, conn_io_client* qconnection) = 0;
    virtual void onreleaseconnection(conn_io_client* qconnection) = 0;
    virtual void onclose(conn_io_client* qconnection) = 0;
};

enum CON_STATE {
    STATE_OPEN,
    STATE_CONNECT,
    STATE_CLOSE
};

class bridge_qcommand {
public:
    virtual void flushegress(struct ev_loop* loop, conn_io_client* qconnection) = 0;
    virtual int release_connection(struct ev_loop* loop, conn_io_client* qconnection) = 0;
    inline virtual struct ev_loop* getmainloop() = 0;

    virtual void event_connect(conn_io_client* qconnection) = 0;
    virtual void event_msg_received(ssize_t recv_len, uint8_t* buf, conn_io_client* qconnection) = 0;
    virtual void event_close(conn_io_client* qconnection) = 0;
    virtual int sendMessage(const qstring& buffer, bool flush) = 0;
    virtual int sendMessage(const uint8_t* buffer, ssize_t size, bool flush) = 0;
    virtual int close() = 0;
    virtual CON_STATE getstate() = 0;

#if USE_PTHREAD
    virtual qmutex* get_run_mutex() = 0;
    virtual qmutex* het_close_mutex() = 0;
    virtual qmutex* get_send_mutex() = 0;
    virtual qmutex* get_sendloop_mutex() = 0;
#endif
};

class qnetworkclient : public bridge_qcommand, public bridge_qconnection {
private:
    struct RunConfig {
        qstring host;
        qstring port;
        qnetworkclient* thiz;
        int pthread_returnValue;
        bool finished = false;
        int id = -1;
    };
    static int connectionID;
    struct ev_loop* mainloop = nullptr;
    struct RunConfig runConfig;

#if USE_PTHREAD
    qmutex run_mutex;
    qmutex close_mutex;
    qmutex send_mutex;
    qmutex sendloop_mutex;
    qmutex runconfig_mutex;
    pthread_t run_thread_id;
#endif

    static void debug_log(const uint8_t* line, void* argp);
    static void recv_cb(EV_P_ ev_io* w, int revents);
    static void timeout_cb(EV_P_ ev_timer* w, int revents);
    static void send_cb(EV_P_ ev_timer* w, int revents);
    static void* run_internal(void* data);

    void setstate(CON_STATE state);
    CON_STATE state = STATE_OPEN;

protected:
    void flushegress(struct ev_loop* loop, conn_io_client* qconnection) override final;
    int release_connection(struct ev_loop* loop, conn_io_client* qconnection) override final;
    void onconnect(conn_io_client* qconnection) override;
    void onmessage(ssize_t recv_len, uint8_t* buf, conn_io_client* qconnection) override;
    void onreleaseconnection(conn_io_client* qconnection) override;
    void onclose(conn_io_client* qconnection) override;
    inline struct ev_loop* getmainloop() override final {
        return mainloop;
    }
    void event_connect(conn_io_client* qconnection) override final;
    void event_msg_received(ssize_t recv_len, uint8_t* buf, conn_io_client* qconnection) override final;
    void event_close(conn_io_client* qconnection) override final;

#if USE_PTHREAD
    qmutex* get_run_mutex() override final {
        return &run_mutex;
    }
    qmutex* het_close_mutex() override final {
        return &close_mutex;
    }
    qmutex* get_sendloop_mutex() override final {
        return &sendloop_mutex;
    }

    qmutex* get_send_mutex() override final {
        return &send_mutex;
    }
#endif
    conn_io_client* qclient_connection = nullptr;

public:
    qnetworkclient();
    ~qnetworkclient();

    inline CON_STATE getstate() override final { return state; }
    inline bool isopen() { return state == STATE_OPEN; }
    inline bool isclosed() { return state == STATE_CLOSE; }
    
    int sendMessage(const qstring& buffer, bool flush) override final;
    int sendMessage(const uint8_t* buffer, ssize_t size, bool flush) override final;
    int close() override final;
    bool is_runfinished();
    int run(qstring host, qstring port);
    void forcerelease();
#if USE_PTHREAD
    inline qmutex& get_runconfigmutex() { return runconfig_mutex; }
#endif
};
};
#endif /* qnetworkclient_hpp */
