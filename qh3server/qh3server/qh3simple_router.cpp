//
//  qh3simple_router.cpp
//  qh3server
//
//  Created by Arun A on 21/12/23.
//

#include "qh3simple_router.hpp"

qh3simple_router::qh3simple_router(const router_config& config) : config(config){
}
qh3simple_router::~qh3simple_router() {
    for(auto r : routes) {
        GX_DELETE(r);
    }
}

int qh3simple_router::run() {
    const struct addrinfo hints = {
        .ai_family = PF_UNSPEC,
        .ai_socktype = SOCK_DGRAM,
        .ai_protocol = IPPROTO_UDP
    };
    freeaddrinfo(router);
    if (getaddrinfo(config.host.c_str(), config.port.c_str(), &hints, &router) != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to resolve host");
        return -1;
    }

    if (sock!=-1) {
        close(sock);
    }
    sock = socket(router->ai_family, SOCK_DGRAM, 0);
    if (sock < 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create socket");
        freeaddrinfo(router);
        router = nullptr;
        return -1;
    }

    if (fcntl(sock, F_SETFL, O_NONBLOCK) != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to make socket non-blocking");
        freeaddrinfo(router);
        router = nullptr;
        close(sock);
        return -1;
    }

    if (bind(sock, router->ai_addr, router->ai_addrlen) < 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to connect socket");
        freeaddrinfo(router);
        router = nullptr;
        close(sock);
        return -1;
    }
    
    ev_io watcher;

    mainloop = ev_default_loop(0);

    ev_io_init(&watcher, recv_cb, sock, EV_READ);
    ev_io_start(mainloop, &watcher);
    watcher.data = this;
    
    // spawn initial servers
    int spawned_servers = 0;
    int overflow = 0;
    int index = 0;
    while (spawned_servers<5) {
        overflow++;
        if (overflow>1000) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "OVERFLOW ON SPAWNING SERVERS !!!");
            DEBUG_PRINT_ERROR(__LOGTAG__, "OVERFLOW ON SPAWNING SERVERS !!!");
            DEBUG_PRINT_ERROR(__LOGTAG__, "OVERFLOW ON SPAWNING SERVERS !!!");
            break;
        }
        int free_port = next_available_port(config.host, range, index++);
        if (free_port<0) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "NO PORT AVAILABLE !!!");
            break;
        }
        route* route = spawn_qh3server(config.host, qstring::format_string("%d", free_port), config.mongodb_uri, config.redis_ip, config.redis_port, config.rootDir);
        if (route==nullptr) {
            continue;
        }
        spawned_servers++;
    }
    //
    
    ev_loop(mainloop, 0);
    
    ev_io_stop(mainloop, &watcher);
    ev_loop_destroy(mainloop);
    
    freeaddrinfo(router);
    router = nullptr;
    close(sock);
    
    return 0;
}

void qh3simple_router::recv_cb(EV_P_ ev_io* w, int revents) {
    UNUSED(revents);
    qh3simple_router* router = (qh3simple_router*)w->data;
    static uint8_t buf[65535];

    while (1) {
        struct sockaddr_storage peer_addr;
        socklen_t peer_addr_len = sizeof(peer_addr);
        memset(&peer_addr, 0, peer_addr_len);

        ssize_t read = recvfrom(router->sock, buf, sizeof(buf), 0,
            (struct sockaddr*)&peer_addr,
            &peer_addr_len);

        if (read < 0) {
            if ((errno == EWOULDBLOCK) || (errno == EAGAIN)) {
                DEBUG_PRINT(LOG_LEVEL_3, __LOGTAG__, "recv would block");
                break;
            }

            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to read");
            return;
        }

//        qh3server::get_stats_loggeer()->server_count("recv_cb", read, "", "", "", "rx", "qh3server", "");
        
        uint8_t type;
        uint32_t version;

        uint8_t scid[MAX_CID_LEN];
        size_t scid_len = sizeof(scid);

        uint8_t dcid[MAX_CID_LEN];
        size_t dcid_len = sizeof(dcid);

//        uint8_t odcid[MAX_CID_LEN];
//        size_t odcid_len = sizeof(odcid);

        uint8_t token[MAX_TOKEN_LEN];
        size_t token_len = sizeof(token);

        int rc = quiche_header_info(buf, read, LOCAL_CONN_ID_LEN, &version,
            &type, scid, &scid_len, dcid, &dcid_len,
            token, &token_len);
        if (rc < 0) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to parse header: %d", rc);
//            qh3server::get_stats_loggeer()->server_count("recv_cb", 1, "", "", "", "error", "qh3server", "parse_header_fail");
            return;
        }
        
        
        unsigned long  crc_ = crc32(0L, Z_NULL, 0);
        crc_ = crc32_z(crc_, (const unsigned char*)dcid, dcid_len);
        int index = crc_%(int)router->routes.size();
        route* route = router->routes[index];
        struct sockaddr* peer_addr_to_pass = (struct sockaddr*)&peer_addr;
        memcpy((void*)&buf[read], (void*)peer_addr_to_pass, peer_addr_len);
        
        DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "in router crc = 0x%x", essentials::get_crc(&buf[read], peer_addr_len));
        
        route->relay(buf, read+peer_addr_len);
    }
}


void route::router_recv_cb(EV_P_ ev_io* w, int revents) {
    UNUSED(revents);
    route* route_client = (route*)w->data;
    static uint8_t buf_r[65535];

    while (1) {
        struct sockaddr_storage peer_addr;
        socklen_t peer_addr_len = sizeof(peer_addr);
        memset(&peer_addr, 0, peer_addr_len);

        ssize_t read = recvfrom(route_client->bridge_sock, buf_r, sizeof(buf_r), 0,
            (struct sockaddr*)&peer_addr,
            &peer_addr_len);

        if (read < 0) {
            if ((errno == EWOULDBLOCK) || (errno == EAGAIN)) {
                DEBUG_PRINT(LOG_LEVEL_3, __LOGTAG__, "recv would block");
                break;
            }

            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to read");
            return;
        }
        
        uint8_t type;
        uint32_t version;

        uint8_t scid[MAX_CID_LEN];
        size_t scid_len = sizeof(scid);

        uint8_t dcid[MAX_CID_LEN];
        size_t dcid_len = sizeof(dcid);

//        uint8_t odcid[MAX_CID_LEN];
//        size_t odcid_len = sizeof(odcid);

        uint8_t token[MAX_TOKEN_LEN];
        size_t token_len = sizeof(token);

        int rc = quiche_header_info(buf_r, read, LOCAL_CONN_ID_LEN, &version,
            &type, scid, &scid_len, dcid, &dcid_len,
            token, &token_len);
        if (rc < 0) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to parse header: %d", rc);
//            qh3server::get_stats_loggeer()->server_count("recv_cb", 1, "", "", "", "error", "qh3server", "parse_header_fail");
            return;
        }
        
        unsigned long  crc_ = crc32(0L, Z_NULL, 0);
        crc_ = crc32_z(crc_, (const unsigned char*)dcid, dcid_len);
    }
}

ssize_t route::relay(uint8_t* buf, ssize_t len) {
    ssize_t sent = sendto(bridge_sock, buf, len, 0, peer->ai_addr, peer->ai_addrlen);
    if (sent != len) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to send");
        return -1;
    }
    return sent;
}
int route::create_bridge(struct ev_loop* loop) {
    const struct addrinfo hints = {
        .ai_family = PF_UNSPEC,
        .ai_socktype = SOCK_DGRAM,
        .ai_protocol = IPPROTO_UDP
    };

    if (getaddrinfo(host.c_str(), port.c_str(), &hints, &peer) != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to resolve host");
        return -1;
    }

    bridge_sock = socket(peer->ai_family, SOCK_DGRAM, 0);
    if (bridge_sock < 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create socket");
        freeaddrinfo(peer);
        return -1;
    }

    if (fcntl(bridge_sock, F_SETFL, O_NONBLOCK) != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to make socket non-blocking");
        freeaddrinfo(peer);
        close_socket();
        return -1;
    }
    
    local_addr_len = sizeof(local_addr);
    if (getsockname(bridge_sock, (struct sockaddr*)&local_addr,
        &local_addr_len) != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to get local address of socket");
        freeaddrinfo(peer);
        close_socket();
        return -1;
    };

//    ev_io_init(&watcher, router_recv_cb, bridge_sock, EV_READ);
//    ev_io_start(loop, &watcher);
//    watcher.data = this;
    
    return 0;
}

int route::close_socket() {
    if (bridge_sock==-1) {
        return -1;
    }
    int result = close(bridge_sock);
    if (result < 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "Bridge socket closure failed: %s", strerror(errno));
    }
    return result;
}

route* qh3simple_router::spawn_qh3server(const qstring& host, const qstring& port,
                                      const qstring& mongodb_uri, const qstring& redis_ip, int redis_port_,
                                      const fs::path& rootDir) {
    router_config* config = DEBUG_NEW router_config(host, port, mongodb_uri, redis_ip, redis_port_, rootDir, router);
    if (pthread_create(&config->run_thread_id, nullptr, qh3simple_router::spawn_qh3server_internal, (void*)config) < 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "spawn_qh3server - could not create thread: %s - %d", strerror(errno), errno);
        GX_DELETE(config);
        return nullptr;
    }
    route* child = DEBUG_NEW route(host, port, server_counter++);
    child->create_bridge(mainloop);
    routes.push_back(child);
    return child;
}

void* qh3simple_router::spawn_qh3server_internal(void* data) {
    router_config* config = (router_config*)data;
    qstring& host = config->host;
    qstring& port = config->port;
    qstring& mongodb_uri = config->mongodb_uri;      //"mongodb://192.168.0.230:6006"
    qstring& redis_ip = config->redis_ip;
    int redis_port = config->redis_port;
    fs::path& rootDir = config->rootDir;
    http3_sample_server* new_server = DEBUG_NEW http3_sample_server(mongodb_uri.c_str(), redis_ip.c_str(), redis_port);
    new_server->run(host.c_str(), port.c_str(), rootDir, config->router);
    GX_DELETE(new_server);
    GX_DELETE(config);
    pthread_exit(0);
}

int qh3simple_router::next_available_port(const qstring& host, port_range& range, int index) {
    int min = range.min;
    int max = range.max;
    for (int port = min+index; port<max; port++) {
        if (is_port_available(host, port)==port) {
            return port;
        }
    }
    return 0;
}

int qh3simple_router::is_port_available(const qstring& host, int port_number) {
    qstring port = qstring::format_string("%d", port_number);
    const struct addrinfo hints = {
        .ai_family = PF_UNSPEC,
        .ai_socktype = SOCK_DGRAM,
        .ai_protocol = IPPROTO_UDP
    };
    struct addrinfo* local;
    if (getaddrinfo(host.c_str(), port.c_str(), &hints, &local) != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to resolve host");
        return -1;
    }

    int sock = socket(local->ai_family, SOCK_DGRAM, 0);
    if (sock < 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create socket");
        freeaddrinfo(local);
        return -1;
    }

    if (fcntl(sock, F_SETFL, O_NONBLOCK) != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to make socket non-blocking");
        freeaddrinfo(local);
        close(sock);
        return -1;
    }

    if (bind(sock, local->ai_addr, local->ai_addrlen) < 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to connect socket");
        freeaddrinfo(local);
        close(sock);
        return -1;
    }
    
    freeaddrinfo(local);
    close(sock);
    return port_number;
}
