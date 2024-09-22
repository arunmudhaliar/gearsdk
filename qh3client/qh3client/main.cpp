//
//  Copyright 2024 homenet25
//  main.cpp
//  qh3client
//
//  Created by Arun A on 30/10/23.
//

#include "http3_sample_client.hpp"

#define DEFAULT_REQUEST_IP "127.0.0.1:4004"
#define DEFAULT_REQUEST_COUNT 10

typedef struct app_state_t {
	qstring host;
	qstring port;
	struct ev_loop* loop;
	http3_sample_client client;
	qtimer_scheduler scheduler;
	ev_tstamp creation_time;
	int request_count = DEFAULT_REQUEST_COUNT;
} app_state_t;

app_state_t app_state;

void parse_arguments(int argc, const char* argv[]) {
	// server address
	app_state.host = "127.0.0.1";
	app_state.port = "4004";
	app_state.request_count = DEFAULT_REQUEST_COUNT;

	qstring request_ip(DEFAULT_REQUEST_IP);
	std::vector<qstring> request_ip_parts;
	request_ip.split(":", request_ip_parts, false);
	if (request_ip_parts.size() == 2) {
		app_state.host = request_ip_parts[0];
		app_state.port = request_ip_parts[1];
	} else {
		debug_print_error(__LOGTAG__, "Invalid request IP: %s", request_ip.c_str());
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
				debug_print_error(__LOGTAG__, "Invalid request IP: %s. Defaulting to %s", request_ip.c_str(), DEFAULT_REQUEST_IP);
			}
		} else if (strcmp(argv[i], "--requests") == 0 && i + 1 < argc) {
			app_state.request_count = atoi(argv[++i]);
		}
	}
}

void print_usage(const char* program_name) {
	fprintf(stderr, "Usage: %s [options]\n\n", program_name);
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "  --server <host:port>      Server address in the form of host:port (default: %s)\n", DEFAULT_REQUEST_IP);
	fprintf(stderr, "  --requests <count>        [optional] No of requests (default: %d)\n\n", DEFAULT_REQUEST_COUNT);
	fprintf(stderr, "Example:\n");
	fprintf(stderr, "  %s --server 127.0.0.1:4004 --requests 10\n", program_name);
}

int main(int argc, const char* argv[]) {
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--help") == 0) {
			print_usage(argv[0]);
			return 0;
		}
	}

	parse_arguments(argc, argv);
	init_gsdk();

	//	  qstring host = "192.168.0.230";
	//    app_state.host = "15.206.79.30";  // override

	debug_print(LOG_LEVEL_0, __LOGTAG__, "host %s, port %s", app_state.host.c_str(), app_state.port.c_str());

	app_state.loop = ev_default_loop(0);

	app_state.client.set_server_info(app_state.host, app_state.port);
	app_state.client.set_loop(app_state.loop);
	app_state.client.create_connections(app_state.request_count);

	app_state.creation_time = ev_now(app_state.loop);
	app_state.scheduler.set_loop(app_state.loop);

	qtimer* keep_alive_loop = app_state.scheduler.schedule_repeat_timer(
		[&](qtimer& timer) {
			UNUSED(timer);
			debug_print_important(__LOGTAG__, "client alive - t:%5.2fs", ev_now(app_state.loop) - app_state.creation_time);
			if (app_state.client.is_finished()) {
				ev_break(app_state.loop, EVBREAK_ONE);
			}
		},
		3);
	UNUSED(keep_alive_loop);
	ev_run(app_state.loop, 0);
	return 0;
}
