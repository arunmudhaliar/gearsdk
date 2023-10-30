//
//  qh3server.hpp
//  qh3server
//
//  Created by Arun A on 30/10/23.
//

#ifndef qh3server_hpp
#define qh3server_hpp

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
#include <uthash.h>
#include <quiche.h>

#include "../../common/sdktypes.hpp"
#include "../../networkcommon/source/essentials.hpp"

//#if PLATFORM == PLATFORM_MAC
//namespace fs = std::__fs::filesystem;
//#elif PLATFORM == PLATFORM_LINUX
//namespace fs = std::filesystem;
//#else
//namespace fs = std::__fs::filesystem;
//#endif

#undef __LOGTAG__
#define __LOGTAG__ "qh3server"

#define LOCAL_CONN_ID_LEN 16
#define MAX_DATAGRAM_SIZE 1350

#define MAX_TOKEN_LEN \
    sizeof("quiche") - 1 + \
    sizeof(struct sockaddr_storage) + \
    MAX_CID_LEN

// trouble shoot
// https://www.chromium.org/for-testers/providing-network-details/

class bridge_h3_connection {
public:
    virtual void flush_egress(struct ev_loop *loop, struct conn_io *conn_io) = 0;
    virtual void destroy_connection(struct ev_loop *loop, struct conn_io *conn_io) = 0;
    inline virtual struct ev_loop *get_mainloop() = 0;
    virtual void parse_header(const uint8_t *name, size_t name_len,
                              const uint8_t *value, size_t value_len, struct conn_io *conn_io) = 0;
    virtual void parse(struct conn_io *conn_io) = 0;
};

struct connections {
    int sock;
    struct sockaddr *local_addr = nullptr;
    socklen_t local_addr_len;
    struct conn_io *h = nullptr;
};

struct conn_io {
    ev_timer timer;
    int sock;
    uint8_t cid[LOCAL_CONN_ID_LEN];
    Connection *conn;
    Connection *http3;
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len;
    UT_hash_handle hh;
    bridge_h3_connection* bridge = nullptr;
    getorpost_reqdata http_request;
    getorpost_response_data http_response;
};

class qh3server : public bridge_h3_connection {
private:
    Config *config = nullptr;
    Config *http3_config = nullptr;
    struct connections *conns = nullptr;
    struct ev_loop *mainloop = nullptr;
    
    static void debug_log(const uint8_t *line, void *argp);
    void flush_egress(struct ev_loop *loop, struct conn_io *conn_io) override final;
    void destroy_connection(struct ev_loop *loop, struct conn_io *conn_io) override final;
    inline virtual struct ev_loop *get_mainloop() override final {
        return mainloop;
    }

    void parse(struct conn_io *conn_io) override;
    
    void mint_token(const uint8_t *dcid, size_t dcid_len,
                           struct sockaddr_storage *addr, socklen_t addr_len,
                           uint8_t *token, size_t *token_len);
    bool validate_token(const uint8_t *token, size_t token_len,
                               struct sockaddr_storage *addr, socklen_t addr_len,
                               uint8_t *odcid, size_t *odcid_len);
    static uint8_t *gen_cid(uint8_t *cid, size_t cid_len);
    struct conn_io *create_conn(uint8_t *scid, size_t scid_len,
                                       uint8_t *odcid, size_t odcid_len,
                                       struct sockaddr *local_addr,
                                       socklen_t local_addr_len,
                                       struct sockaddr_storage *peer_addr,
                                       socklen_t peer_addr_len);
    static int for_each_header(const uint8_t *name, size_t name_len,
                               const uint8_t *value, size_t value_len,
                               void *argp);
    static void recv_cb(EV_P_ ev_io *w, int revents);
    static void timeout_cb(EV_P_ ev_timer *w, int revents);
    
protected:
    void parse_header(const uint8_t *name, size_t name_len,
                      const uint8_t *value, size_t value_len, struct conn_io *conn_io) override;
    
public:
    int run(const std::string& host, const std::string& port, fs::path& rootDir);
};

#endif /* qh3server_hpp */
