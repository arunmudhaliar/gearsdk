//
//  Copyright 2024 homenet25
//  main.cpp
//  qh3client
//
//  Created by Arun A on 30/10/23.
//

#include "http3_sample_client.hpp"

#define DEFAULT_REQUEST_IP "127.0.0.1:4004"

typedef struct {
	qstring host;
	qstring port;
	struct ev_loop* loop;
	http3_sample_client client;
	qtimer_sceduler scheduler;
	ev_tstamp creation_time;
} app_state_t;

app_state_t app_state;

void parse_arguments(int argc, const char *argv[]) {
    // server address
	app_state.host = "127.0.0.1";
	app_state.port = "4004";

    qstring request_ip(DEFAULT_REQUEST_IP);
    std::vector<qstring> request_ip_parts;
    request_ip.split(":", request_ip_parts, false);
    if (request_ip_parts.size() == 2) {
        app_state.host = request_ip_parts[0];
        app_state.port = request_ip_parts[1];
    } else {
        DEBUG_PRINT_ERROR(__LOGTAG__, "Invalid request IP: %s", request_ip.c_str());
    }
    //

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--server") == 0 && i + 1 < argc) {
            qstring request_ip(argv[++i]);
            std::vector<qstring> request_ip_parts;
            request_ip.split(":", request_ip_parts, false);
            if (request_ip_parts.size() == 2) {
                app_state.host = request_ip_parts[0];
                app_state.port = request_ip_parts[1];
            } else {
                DEBUG_PRINT_ERROR(__LOGTAG__, "Invalid request IP: %s. Defaulting to %s", request_ip.c_str(), DEFAULT_REQUEST_IP);
            }
        }
    }
}

int main(int argc, const char* argv[]) {
	parse_arguments(argc, argv);
	init_gsdk();

	//    qstring host = "192.168.0.230";
	//    qstring host = "54.173.204.159";

	DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "host %s, port %s", app_state.host.c_str(), app_state.port.c_str());

	app_state.loop = ev_default_loop(0);

	app_state.client.set_server_info(app_state.host, app_state.port);
	app_state.client.set_ev_lopp(app_state.loop);
	app_state.client.init_connection();

	app_state.creation_time = ev_now(app_state.loop);
	app_state.scheduler.set_ev_lopp(app_state.loop);

	qtimer* keep_alive_loop = app_state.scheduler.schedule_repeat_timer(
		[&](qtimer& timer) {
			UNUSED(timer);
			DEBUG_PRINT_IMPORTANT(__LOGTAG__, "client alive - t:%5.2fs", ev_now(app_state.loop) - app_state.creation_time);

			//        http3_sample_client client(host, "4004");
			//        client.set_ev_lopp(loop);
			//        client.init_connection();
			ev_break(app_state.loop, EVBREAK_ONE);
		},
		100);
	UNUSED(keep_alive_loop);
	ev_run(app_state.loop, 0);
	//    ev_loop_destroy(loop);    // this will cause leak, but no solution for now. Will fix this
	return 0;
}
