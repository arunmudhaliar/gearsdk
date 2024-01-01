//
//  qh3simple_router_structs.h
//  qh3server
//
//  Created by Arun A on 30/12/23.
//

#ifndef qh3simple_router_structs_h
#define qh3simple_router_structs_h

#include "../../networkcommon/source/essentials.hpp"

struct route;
class bridge_command_center {
public:
    virtual const std::vector<route*>& get_routes() = 0;
    virtual void cmd_feedback_from_client(struct sockaddr* client_addr, const qstring& cmd) = 0;
};

struct router_config {
    router_config(const qstring& host, const qstring& port,
                  const qstring& mongodb_uri, const qstring& redis_ip, int redis_port_,
                  const fs::path& rootDir, struct addrinfo* router_, uint16_t command_port_, const qstring& router_port_) :
                    host(host), port(port),
                    mongodb_uri(mongodb_uri), redis_ip(redis_ip), redis_port(redis_port_),
                    rootDir(rootDir), router(router_), command_port(command_port_), router_port(router_port_) {
    }
    
    router_config(const router_config& config) :
                    host(config.host), port(config.port),
                    mongodb_uri(config.mongodb_uri), redis_ip(config.redis_ip), redis_port(config.redis_port),
                    rootDir(config.rootDir), command_server(config.command_server), command_port(config.command_port),
                    command_feedback_port(config.command_feedback_port), ref(config.ref), router_port(config.router_port) {
    }
    qstring host = "localhost";
    qstring port = "4034";
    qstring mongodb_uri = "mongodb://localhost:27017";      //"mongodb://192.168.0.230:6006"
    qstring redis_ip = "127.0.0.1";
    int redis_port = 6379;
    fs::path rootDir = ".";
    pthread_t run_thread_id = 0;
    struct addrinfo* router = nullptr;  //only for slaves
    bool command_server = false;
    uint16_t command_port = 4010;
    uint16_t command_feedback_port = 4011;  // this has to be re-assigned based on availabaility
    bridge_command_center* ref = nullptr;
    qstring router_port;
};

struct route {
    route(const qstring& host, const qstring& port, int server_id_) :
    host(host), port(port), server_id(server_id_) {
    }
    ~route() {
        close_bridge_socket();
    }
    int create_bridge(struct ev_loop* loop, void* arg, void (*router_command_recv_cb_ptr)(EV_P_ ev_io* w, int revents) = nullptr);
    int close_bridge_socket();
    ssize_t relay(uint8_t* buf, ssize_t len);
    qstring host;
    qstring port;
    int server_id = -1;
    int bridge_sock = -1;
    struct addrinfo* peer = nullptr;
    ev_io command_watcher;
    void* arg = nullptr;
    pid_t child_process_id = -1;
};
struct port_range {
    const int min = 5100;
    const int max = 5200;
};

#endif /* qh3simple_router_structs_h */
