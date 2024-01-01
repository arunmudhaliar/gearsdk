//
//  main.cpp
//  GNetwork
//
//  Created by Arun A on 20/09/23.
//
#include "gameserver.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "qserver"

static qstring version_string = "0.1";
static unsigned version_code = 1;

int32_t main(int32_t argc, const char * argv[]) {
    init_gsdk();
    qstring host = "localhost";
    qstring port = "4000";
    qstring mongodb_uri = "mongodb://localhost:27017";      // "mongodb://192.168.0.230:6006";
    qstring redis_ip = "127.0.0.1";
    int redis_port = 6379;
    fs::path rootDir;
    int result = essentials::resolve_cmd_line_args(__LOGTAG__, argc, argv,
                          version_string, version_code,
                          host, port, mongodb_uri, rootDir, redis_ip, redis_port);
    if (result<0) {
        exit(0);
    }
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

