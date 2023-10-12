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

extern "C" {
#include <quiche.h>
}

#undef __LOGTAG__
#define __LOGTAG__ "QNetworkClient"


#define LOCAL_CONN_ID_LEN 16

#define MAX_DATAGRAM_SIZE 1350

struct conn_io {
    ev_timer timer;

    int sock;

    struct sockaddr_storage local_addr;
    socklen_t local_addr_len;

    Connection *conn;
};

class QNetworkClient {
    static void debug_log(const uint8_t *line, void *argp);
    static void flush_egress(struct ev_loop *loop, struct conn_io *conn_io);
    static void recv_cb(EV_P_ ev_io *w, int revents);
    static void timeout_cb(EV_P_ ev_timer *w, int revents);
    
public:
    static int run(std::string host, std::string port);
};
#endif /* QNetworkClient_hpp */
