//
//  main.cpp
//  qh3server
//
//  Created by Arun A on 30/10/23.
//

#include "http3_sample_server.hpp"
#include "qh3simple_router.hpp"

//#include "../../qhiredis/source/qhiredis.hpp"

static qstring version_string = "0.1";
static unsigned version_code = 1;

#undef __LOGTAG__
#define __LOGTAG__ "qh3server-main"

int main(int argc, const char* argv[]) {
    init_gsdk();
    // main http server
    qstring host = "localhost";
//    qstring host = "192.168.0.65";
    qstring port = "4004";
    qstring mongodb_uri = "mongodb://localhost:27017";      //"mongodb://192.168.0.230:6006"
    qstring redis_ip = "127.0.0.1";
    int redis_port = 6379;
    fs::path rootDir;
    int result = essentials::resolve_cmd_line_args(__LOGTAG__, argc, argv,
        version_string, version_code,
        host, port, mongodb_uri, rootDir, redis_ip, redis_port);
    if (result < 0) {
        exit(0);
    }
    
//    http3_sample_server server(mongodb_uri.c_str(), redis_ip.c_str(), redis_port);
//    server.run(host, port, rootDir, nullptr);
    
    router_config config(host, port, mongodb_uri, redis_ip, redis_port, rootDir, nullptr);
    qh3simple_router router(config);
    router.run();
    
    //    server.test_mongo_db();

    // log file
    //    qtextfilelogger logger;
    //    logger.start_session("test_log", sizeof("test_log"), 10, 400);
    //    struct ev_loop* loop = ev_default_loop(0);
    //    ev_tstamp creation_time = ev_now(loop);
    //    qtimer_sceduler scheduler;
    //    scheduler.set_ev_lopp(loop);
    //    scheduler.schedule_repeat_timer([&logger, loop, creation_time](qtimer& timer){
    //        logger.log(qlogfile::level_0, __LOGTAG__, "iam alive - t:%5.2fs", ev_now(loop) - creation_time );
    //    }, 0.5);
    //
    //    ev_run(loop, 0);

    // redis
    //    qhiredis hiredis;
    //    hiredis.hiredis_main(1, nullptr);

    // mongo
    //    server.test_mongo_db();

    return 0;
}
