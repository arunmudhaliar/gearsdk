//
//  main.cpp
//  qh3server
//
//  Created by Arun A on 30/10/23.
//

#include "http3_sample_server.hpp"

//#include "../../qhiredis/source/qhiredis.hpp"

static std::string version_string = "0.1";
static unsigned version_code = 1;

int main(int argc, const char* argv[]) {
    init_gsdk();
    // main http server
    std::string host = "localhost";
    std::string port = "4004";
    std::string mongodb_uri = "mongodb://localhost:27017";      //"mongodb://192.168.0.230:6006"

    fs::path rootDir;
    int result = essentials::resolve_cmd_line_args(__LOGTAG__, argc, argv,
        version_string, version_code,
        host, port, mongodb_uri, rootDir);
    if (result < 0) {
        exit(0);
    }
    http3_sample_server server(mongodb_uri.c_str());
    server.run(host, port, rootDir);
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
