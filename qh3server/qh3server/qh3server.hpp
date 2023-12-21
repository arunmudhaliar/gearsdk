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
#include "../../networkcommon/source/qtextfilelogger.hpp"
#include "../../networkcommon/source/qstatslogger.hpp"

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
    virtual void flush_egress(struct ev_loop* loop, struct conn_io* conn_io) = 0;
    virtual void destroy_connection(struct ev_loop* loop, struct conn_io* conn_io) = 0;
    inline virtual struct ev_loop* get_mainloop() = 0;
    virtual void parse_header(const qstring& name, const qstring& value, struct conn_io* conn_io) = 0;
    virtual void parse(struct conn_io* conn_io) = 0;
    virtual bool is_log_quiche() = 0;   //NOTE : TODO - This is polling which is not a recommended solution. Need to use event based system.
};

struct connections {
    int sock;
    struct sockaddr* local_addr = nullptr;
    socklen_t local_addr_len;
    struct conn_io* h = nullptr;
    qstring server_port;
    qstring quic_alternate_protocol_str;
};

struct conn_io {
    conn_io() {
        http_request = conn_io_req_res::create();
        http_response = conn_io_req_res::create();
    }
    ~conn_io() {
        GX_DELETE(http_response);
        GX_DELETE(http_request);
    }
    ev_timer timer;
    int sock;
    uint8_t cid[LOCAL_CONN_ID_LEN];
    Connection* conn = nullptr;
    Connection* http3 = nullptr;
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len;
    UT_hash_handle hh;
    bridge_h3_connection* bridge = nullptr;
    conn_io_req_res* http_request = nullptr;
    conn_io_req_res* http_response = nullptr;
    ev_tstamp creation_time = 0;
};

class qh3server : public bridge_h3_connection {
private:
    Config* config = nullptr;
    Config* http3_config = nullptr;
    struct connections* conns = nullptr;
    struct ev_loop* mainloop = nullptr;

    static void debug_log(const uint8_t* line, void* argp);
    void flush_egress(struct ev_loop* loop, struct conn_io* conn_io) override final;
    void destroy_connection(struct ev_loop* loop, struct conn_io* conn_io) override final;
    inline virtual struct ev_loop* get_mainloop() override final {
        return mainloop;
    }
    inline virtual bool is_log_quiche() override {
        return false;
    }
    
    void parse(struct conn_io* conn_io) override;

    void mint_token(const uint8_t* dcid, size_t dcid_len,
        struct sockaddr_storage* addr, socklen_t addr_len,
        uint8_t* token, size_t* token_len);
    bool validate_token(const uint8_t* token, size_t token_len,
        struct sockaddr_storage* addr, socklen_t addr_len,
        uint8_t* odcid, size_t* odcid_len);
    static uint8_t* gen_cid(uint8_t* cid, size_t cid_len);
    struct conn_io* create_conn(uint8_t* scid, size_t scid_len,
        uint8_t* odcid, size_t odcid_len,
        struct sockaddr* local_addr,
        socklen_t local_addr_len,
        struct sockaddr_storage* peer_addr,
        socklen_t peer_addr_len);
    static int for_each_header(const uint8_t* name, size_t name_len,
        const uint8_t* value, size_t value_len,
        void* argp);
    static void recv_cb(EV_P_ ev_io* w, int revents);
    static void timeout_cb(EV_P_ ev_timer* w, int revents);

    uint8_t out[MAX_DATAGRAM_SIZE];
    uint8_t buf[65535];
//    static uint8_t out[MAX_DATAGRAM_SIZE];
    
protected:
    virtual void on_run_started() = 0;
    virtual void on_run_end() = 0;
    void parse_header(const qstring& name, const qstring& value, struct conn_io* conn_io) override;
    static qtextfilelogger* logger;
    static qstatslogger* stats_logger;

public:
    static qtextfilelogger* get_file_logger() { return logger; }
    static qstatslogger* get_stats_loggeer() { return stats_logger; }

    int run(const qstring& host, const qstring& port, fs::path& rootDir);
};

#endif /* qh3server_hpp */
