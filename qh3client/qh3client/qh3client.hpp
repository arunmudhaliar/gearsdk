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

#define LOCAL_CONN_ID_LEN 16
#define MAX_DATAGRAM_SIZE 1350

class bridge_h3client_connection;
struct conn_io {
    ev_timer timer;
    const char *host = nullptr;

    int sock;
    struct sockaddr_storage local_addr;
    socklen_t local_addr_len;

    Connection *conn = nullptr;
    Connection *http3 = nullptr;
    bridge_h3client_connection* bridge = nullptr;
};

class bridge_h3client_connection {
public:
    virtual void flush_egress(struct ev_loop *loop, struct conn_io *conn_io) = 0;
    inline virtual struct ev_loop *get_mainloop() = 0;
    inline virtual const struct getorpost_reqdata& get_getorpost_http_request() = 0;
    virtual int64_t send_get_http_request(const getorpost_reqdata& data_getorpost_, struct conn_io *conn_io) = 0;
    virtual int64_t send_post_http_request(const getorpost_reqdata& data_getorpost_, struct conn_io *conn_io) = 0;
};

class qh3client : public bridge_h3client_connection{
public:
    
    qh3client(const std::string& host, const std::string& port);
    virtual ~qh3client();
    
    struct ev_loop *mainloop = nullptr;
    
    static void debug_log(const uint8_t *line, void *argp);
    void flush_egress(struct ev_loop *loop, struct conn_io *conn_io) override final;
    inline struct ev_loop *get_mainloop() override final {
        return mainloop;
    }
    inline const struct getorpost_reqdata& get_getorpost_http_request() override final {
        return http_request;
    }
    static int for_each_setting(uint64_t identifier, uint64_t value,
                                void *argp);
    static int for_each_header(const uint8_t *name, size_t name_len,
                               const uint8_t *value, size_t value_len,
                               void *argp);
    static void recv_cb(EV_P_ ev_io *w, int revents);
    static void timeout_cb(EV_P_ ev_timer *w, int revents);
    int64_t send_get_http_request(const getorpost_reqdata& data_getorpost_, struct conn_io *conn_io) override final;
    int64_t send_post_http_request(const getorpost_reqdata& data_getorpost_, struct conn_io *conn_io) override final;
    int send_request(const getorpost_reqdata& data_getorpost_);
    
    const std::string host;
    const std::string port;
    getorpost_reqdata http_request;
};
#endif /* qh3client_hpp */
