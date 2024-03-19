//
//  main.cpp
//  qh3server
//
//  Created by Arun A on 30/10/23.
//

#include "http3_sample_server.hpp"
#include "qh3simple_router.hpp"
#include "../../qutils/discord_util.hpp"
#include "../../common/sdktypes.hpp"

//#include "../../qhiredis/source/qhiredis.hpp"

static qstring version_string = "0.1";
static unsigned version_code = 1;

#undef __LOGTAG__
#define __LOGTAG__ "qh3server-main"

void warn_callback(const char* msg) {
    discord_util::send_async(qstring::format_string("[%s] - WARN : %s", gsdk::device::device_details.nodename, msg).c_str());
}
void error_callback(const char* msg) {
    discord_util::send_async(qstring::format_string("[%s] - ERROR : %s", gsdk::device::device_details.nodename, msg).c_str());
}
void assert_callback(const char* msg) {
    discord_util::send_async(qstring::format_string("[%s] - ASSERT : %s", gsdk::device::device_details.nodename, msg).c_str());
}

int main(int argc, const char* argv[]) {
    init_gsdk();
    gsdk::set_warn_callback(warn_callback);
    gsdk::set_error_callback(error_callback);
    gsdk::set_assert_callback(assert_callback);
    // main http server
    qstring host = "127.0.0.1";
//    qstring host = "192.168.0.65";
    qstring port = "4004";
    qstring mongodb_uri = "mongodb://localhost:27017";      //"mongodb://192.168.0.230:6006"
    qstring redis_ip = "127.0.0.1";
    qstring zk_uri = "127.0.0.1:2181";
    uint16_t redis_port = 6379;
    fs::path rootDir;
    int result = essentials::resolve_cmd_line_args(__LOGTAG__, argc, argv,
        version_string, version_code,
        host, port, mongodb_uri, rootDir, redis_ip, redis_port, zk_uri);
    if (result < 0) {
        exit(0);
    }
    
//    http3_sample_server server(mongodb_uri.c_str(), redis_ip.c_str(), redis_port, zk_uri);
//    server.run(host, port, rootDir, nullptr, 4010, 0);  // port return not used, so passing 0
    
    server_config_in config(host, port, mongodb_uri, redis_ip, redis_port, rootDir, nullptr, 4010, port, zk_uri, 4005);
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

//     redis
//    qhiredis hiredis(redis_ip, redis_port);
//    hiredis.connect_redis();
//    hiredis.set_hash_value("test_hash", "f1", "test1");
//    hiredis.set_hash_value("test_hash", "f2", "test2");
    
    
//    long long testValue;
//    hiredis.incr("test_add", testValue);
//    hiredis.decr("test_add", testValue);
//    hiredis.decr_by("test_add", 100, testValue);
    // mongo
    //    server.test_mongo_db();

    return 0;
}



