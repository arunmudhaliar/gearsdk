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

#define LOCAL_CONN_ID_LEN 16
#define MAX_DATAGRAM_SIZE 1350


class qh3client {
public:
    struct conn_io {
        ev_timer timer;
        const char *host = nullptr;

        int sock;
        struct sockaddr_storage local_addr;
        socklen_t local_addr_len;

        Connection *conn = nullptr;
        Connection *http3 = nullptr;
    };
    
    static void debug_log(const uint8_t *line, void *argp);
    static void flush_egress(struct ev_loop *loop, struct conn_io *conn_io);
    static int for_each_setting(uint64_t identifier, uint64_t value,
                                void *argp);
    static int for_each_header(const uint8_t *name, size_t name_len,
                               const uint8_t *value, size_t value_len,
                               void *argp);
    static void recv_cb(EV_P_ ev_io *w, int revents);
    static void timeout_cb(EV_P_ ev_timer *w, int revents);
    static int run(const std::string& host, const std::string& port);
};
#endif /* qh3client_hpp */
