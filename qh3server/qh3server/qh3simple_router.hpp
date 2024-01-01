//
//  qh3simple_router.hpp
//  qh3server
//
//  Created by Arun A on 21/12/23.
//

#ifndef qh3simple_router_hpp
#define qh3simple_router_hpp

#include "http3_sample_server.hpp"
#include "http3_command_server.hpp"
#include "qh3simple_router_structs.h"

#define MAX_ROUTES 16

#undef __LOGTAG__
#define __LOGTAG__ "qh3simple_router"

class qh3simple_router : public bridge_command_center {
public:
    qh3simple_router(const router_config& config);
    virtual ~qh3simple_router();
    int run();
    
    const std::vector<route*>& get_routes() override final {
        return routes;
    }
    void cmd_feedback_from_client(struct sockaddr* client_addr, const qstring& cmd) override final;
    
private:
    static void recv_cb(EV_P_ ev_io* w, int revents);
    struct ev_loop* mainloop = nullptr;
    int sock = -1;
    
    // qh3
    route* spawn_qh3server_command_server(const qstring& host, const qstring& port, const router_config& config);
    int spawn_qh3server(const qstring& host, const qstring& port, const router_config& config, pid_t& child_process_id, bool& fork_result);
    static void* spawn_qh3server_internal(void* data);
    
    static int next_available_port(const qstring& host, port_range& range, int index);  // index starts with 0
    static int is_port_available(const qstring& host, int port_number);
    
    route* command_feedback_route = nullptr;
    route* command_route = nullptr;
    std::vector<route*> routes;
#if FORK_QH3_SERVER
    std::vector<pid_t> server_process_ids;
#endif
    int server_counter = 0;
    port_range range;
    router_config config;   //default config
    struct addrinfo* router = nullptr;
};

class http3_sample_router : public qh3simple_router {
public:
    
};
#endif /* qh3simple_router_hpp */
