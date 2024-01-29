//
//  qh3client.hpp
//  qh3client
//
//  Created by Arun A on 30/10/23.
//

#ifndef qh3client_hpp
#define qh3client_hpp

#include <string>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

#include <fcntl.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <ev.h>
#include <quiche.h>
#include "../../networkcommon/source/essentials.hpp"
#include <map>

#define LOCAL_CONN_ID_LEN 16

#define ORIGINAL_CLIENT_ADDR_SZ (3*sizeof(uint16_t))
#define MAX_DATAGRAM_SIZE 1350 - ORIGINAL_CLIENT_ADDR_SZ   // last 6 bytes is reserved for original client adress verification

#undef __LOGTAG__
#define __LOGTAG__ "qh3client"

namespace client {
class bridge_h3client_connection;
struct conn_io {
    conn_io() {
        response = conn_io_req_res::create();
    }
    ~conn_io() {
        GX_DELETE(response);
    }
    ev_timer timer;
    const char* host = nullptr;

    int sock;
    struct sockaddr_storage local_addr;
    socklen_t local_addr_len;

    Connection* conn = nullptr;
    Connection* http3 = nullptr;
    bridge_h3client_connection* bridge = nullptr;

    bool req_sent = false;
    bool settings_received = false;
    uint8_t buf[65535];
    uint8_t out[MAX_DATAGRAM_SIZE + ORIGINAL_CLIENT_ADDR_SZ];
    conn_io_req_res* response = nullptr;
    bool res_received = false;
    ev_tstamp creation_time = 0;
};

class bridge_h3client_connection {
public:
    virtual void flush_egress(struct ev_loop* loop, struct conn_io* conn_io) = 0;
    inline virtual struct ev_loop* get_mainloop() = 0;
    inline virtual const struct conn_io_req_res* get_getorpost_http_request() = 0;
    virtual int64_t send_get_http_request(const conn_io_req_res* data_getorpost_, struct conn_io* conn_io) = 0;
    virtual int64_t send_post_http_request(const conn_io_req_res* data_getorpost_, struct conn_io* conn_io) = 0;
};

class qh3client : public bridge_h3client_connection {
public:

    qh3client(const qstring& host, const qstring& port, void* arg);
    virtual ~qh3client();

    struct ev_loop* mainloop = nullptr;

    static void debug_log(const uint8_t* line, void* arg);
    void flush_egress(struct ev_loop* loop, struct conn_io* conn_io) override final;
    inline struct ev_loop* get_mainloop() override final {
        return mainloop;
    }
    inline const struct conn_io_req_res* get_getorpost_http_request() override final {
        return http_request;
    }
    static int for_each_setting(uint64_t identifier, uint64_t value,
        void* argp);
    static int for_each_header(const uint8_t* name, size_t name_len,
        const uint8_t* value, size_t value_len,
        void* argp);
    static void recv_cb(EV_P_ ev_io* w, int revents);
    static void timeout_cb(EV_P_ ev_timer* w, int revents);
    int64_t send_get_http_request(const conn_io_req_res* data_getorpost_, struct conn_io* conn_io) override final;
    int64_t send_post_http_request(const conn_io_req_res* data_getorpost_, struct conn_io* conn_io) override final;
    virtual void on_prepare_client_send();
    int send_request(const conn_io_req_res* data_getorpost_);
    virtual void on_post_send_cleanup();
    virtual void* get_client_specific_data();
    const qstring host;
    const qstring port;
    const conn_io_req_res* http_request = nullptr;
    struct conn_io* conn_io = nullptr;
    void* arg = nullptr;

private:
    int close_socket(int sock);
};
};
#endif /* qh3client_hpp */
