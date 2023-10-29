//
//  main.cpp
//  GNetwork
//
//  Created by Arun A on 20/09/23.
//
#include "gameserver.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "qserver"

#if PLATFORM == PLATFORM_LINUX
#include <linux/limits.h>
#endif
#include <netdb.h>

static std::string version_string = "0.1";
static unsigned version_code = 1;

int32_t main(int32_t argc, const char * argv[]) {
    if (argc==2 && strcmp(argv[1], "--version")==0) {
        DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "version %s(%d)", version_string.c_str(), version_code);
        return 0;
    }
    
    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "version %s(%d)", version_string.c_str(), version_code);
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
    gameserver server;
    server.run(host, port, rootDir);
    
    // dummy run loop
    struct ev_loop* loop = ev_default_loop(0);
    ev_tstamp creation_time = ev_now(loop);
    qtimer_sceduler scheduler;
    scheduler.set_ev_lopp(loop);
    scheduler.schedule_repeat_timer([loop, creation_time](qtimer& timer){
        DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "iam alive - t:%5.2fs", ev_now(loop) - creation_time );
    }, 60);
    
    ev_run(loop, 0);
    
    return 0;
}

