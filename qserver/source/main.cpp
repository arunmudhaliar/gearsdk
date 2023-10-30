//
//  main.cpp
//  GNetwork
//
//  Created by Arun A on 20/09/23.
//
#include "gameserver.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "qserver"

static std::string version_string = "0.1";
static unsigned version_code = 1;

int32_t main(int32_t argc, const char * argv[]) {
    std::string host = "localhost";
    std::string port = "4000";
    fs::path rootDir;
    essentials::resolve_cmd_line_args(__LOGTAG__, argc, argv,
                          version_string, version_code,
                          host, port, rootDir);
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

