//
//  main.cpp
//  qh3client
//
//  Created by Arun A on 30/10/23.
//

#include "http3_sample_client.hpp"

int main(int argc, const char *argv[]) {
    UNUSED(argc);
    UNUSED(argv);
    init_gsdk();
//    std::string host = "192.168.0.230";
    std::string host = "localhost";
    
    struct ev_loop* loop = ev_default_loop(0);
    
    http3_sample_client client(host, "4004");
    client.set_ev_lopp(loop);
    client.init_connection();
    
    qtimer_sceduler scheduler;
    
    ev_tstamp creation_time = ev_now(loop);
    scheduler.set_ev_lopp(loop);
    
    qtimer* keep_alive_loop = scheduler.schedule_repeat_timer([loop, creation_time](qtimer& timer) {
        UNUSED(timer);
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "client alive - t:%5.2fs", ev_now(loop) - creation_time);
    }, 600);
    UNUSED(keep_alive_loop);
    ev_run(loop, 0);

    
    return 0;
}

