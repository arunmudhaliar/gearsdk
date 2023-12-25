//
//  qh3simple_router.hpp
//  qh3server
//
//  Created by Arun A on 21/12/23.
//

#ifndef qh3simple_router_hpp
#define qh3simple_router_hpp

#include "http3_sample_server.hpp"

#define MAX_ROUTES 16

#undef __LOGTAG__
#define __LOGTAG__ "qh3simple_router"

struct router_config {
    router_config(const qstring& host, const qstring& port,
                  const qstring& mongodb_uri, const qstring& redis_ip, int redis_port_,
                  const fs::path& rootDir, struct addrinfo* router_) :
                    host(host), port(port),
                    mongodb_uri(mongodb_uri), redis_ip(redis_ip), redis_port(redis_port_),
                    rootDir(rootDir), router(router_) {
    }
    
    router_config(const router_config& config) :
                    host(config.host), port(config.port),
                    mongodb_uri(config.mongodb_uri), redis_ip(config.redis_ip), redis_port(config.redis_port),
                    rootDir(config.rootDir) {
    }
    qstring host = "localhost";
    qstring port = "4034";
    qstring mongodb_uri = "mongodb://localhost:27017";      //"mongodb://192.168.0.230:6006"
    qstring redis_ip = "127.0.0.1";
    int redis_port = 6379;
    fs::path rootDir = ".";
    pthread_t run_thread_id = 0;
    struct addrinfo* router = nullptr;  //only for slaves
};

struct route {
    route(const qstring& host, const qstring& port, int server_id_) :
    host(host), port(port), server_id(server_id_) {
    }
    ~route() {
        close_socket();
    }
    int create_bridge(struct ev_loop* loop);
    int close_socket();
    ssize_t relay(uint8_t* buf, ssize_t len);
    qstring host;
    qstring port;
    int server_id = -1;
    int bridge_sock = -1;
    struct sockaddr_storage local_addr;
    socklen_t local_addr_len;
    struct addrinfo* peer = nullptr;
    ev_io watcher;
    static void router_recv_cb(EV_P_ ev_io* w, int revents);
};
struct port_range {
    const int min = 5100;
    const int max = 5200;
};

class qh3simple_router {
public:
    qh3simple_router(const router_config& config);
    virtual ~qh3simple_router();
    int run();
private:
    static void recv_cb(EV_P_ ev_io* w, int revents);
    struct ev_loop* mainloop = nullptr;
    int sock = -1;
    
    // qh3
    route* spawn_qh3server(const qstring& host, const qstring& port,
                        const qstring& mongodb_uri, const qstring& redis_ip, int redis_port_,
                        const fs::path& rootDir);
    static void* spawn_qh3server_internal(void* data);
    
    static int next_available_port(const qstring& host, port_range& range, int index);  // index starts with 0
    static int is_port_available(const qstring& host, int port_number);
    
    std::vector<route*> routes;
    int server_counter = 0;
    port_range range;
    router_config config;   //default config
    struct addrinfo* router = nullptr;
};

class http3_sample_router : public qh3simple_router {
public:
    
};
#endif /* qh3simple_router_hpp */
