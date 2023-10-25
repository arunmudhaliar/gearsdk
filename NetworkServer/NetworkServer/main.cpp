//
//  main.cpp
//  GNetwork
//
//  Created by Arun A on 20/09/23.
//
#include "GameServer.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "NetworkServer"

#if PLATFORM == PLATFORM_LINUX
#include <linux/limits.h>
#endif
#include <netdb.h>

int32_t main(int32_t argc, const char * argv[]) {
    PrintCommonInfo();
    std::string host = "localhost";
    std::string port = "4000";
    if (argc==3) {
        host = argv[1];
        port = argv[2];
        const struct addrinfo hints = {
            .ai_family = PF_UNSPEC,
            .ai_socktype = SOCK_DGRAM,
            .ai_protocol = IPPROTO_UDP
        };
        struct addrinfo *peer = nullptr;
        if (getaddrinfo(host.c_str(), port.c_str(), &hints, &peer) != 0) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "Failed to resolve host. Exiting !!!");
            DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "Usage : <executable> 'ip address' 'port'");
            return -1;
        }
        if (peer) {
            freeaddrinfo(peer);
            peer = nullptr;
        }
    } else {
        DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "Usage : <executable> 'ip address' 'port'. Ignore for debug builds running locally.");
    }
    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "host:%s, port:%s", host.c_str(), port.c_str());
    
    fs::path executablePath(argc>0 ? argv[0] : "");
    fs::path rootDir = executablePath.parent_path();
    DEBUG_PRINT_IMPORTANT(__DEFAULT_LOG_TAG__, "Root dir : %s", rootDir.c_str());
    GameServer server;
    server.run(host, port, rootDir);
    
    return 0;
}

