//
//  Copyright 2024 homenet25
//  main.cpp
//  GNetwork
//
//  Created by Arun A on 20/09/23.
//
#include "../../common/signal_handler/signal_handler.hpp"
#include "../../qutils/discord_util.hpp"
#include "../servercommon/source/servercommon.hpp"
#include "gameserver.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "qserver"

static qstring version_string = "0.1";
static unsigned version_code = 1;

#define DISCORD_WEBHOOK "https://discord.com/api/webhooks/1214655271985750056/ExoVMFlV4Pu_t3fxRQqi_Jbo7SUP1Q5lTnkC6-eG81jKaCxqhggLD_sYg41WL38UmgQe"

void warn_callback(const char* msg) {
	discord_util::send_async(qstring::format_string("[%s] - WARN : %s", gsdk::device::device_details.nodename, msg).c_str());
}
void error_callback(const char* msg) {
	discord_util::send_async(qstring::format_string("[%s] - ERROR : %s", gsdk::device::device_details.nodename, msg).c_str());
}
void assert_callback(const char* msg) {
	discord_util::send_async(qstring::format_string("[%s] - ASSERT : %s", gsdk::device::device_details.nodename, msg).c_str());
}

int32_t main(int32_t argc, const char* argv[]) {
	signal_handler::setup_signal_handler();
	init_gsdk();
	gsdk::servercommon::init_server_common();
	discord_util::initialize_with_webhook_url(DISCORD_WEBHOOK);
	gsdk::set_warn_callback(warn_callback);
	gsdk::set_error_callback(error_callback);
	gsdk::set_assert_callback(assert_callback);
	qstring host = "127.0.0.1";
	qstring port = "4000";
	qstring mongodb_uri = "mongodb://3.109.144.159:27017";  // "mongodb://192.168.0.230:27017";
	qstring redis_ip = "3.109.144.159";
	qstring zk_uri = "3.109.144.159:2181";
	uint16_t redis_port = 6379;
	fs::path root_dir;
	int result = essentials::resolve_cmd_line_args(__LOGTAG__, argc, argv, version_string, version_code, host, port, mongodb_uri, root_dir, redis_ip, redis_port, zk_uri);
	if (result < 0) {
		discord_util::shutdown();
		exit(0);
	}
	gameserver server(zk_uri);
	server.run(host, port, root_dir, redis_ip, redis_port, "qserver-app");

	// dummy run loop
	struct ev_loop* loop = ev_default_loop(0);
	ev_tstamp creation_time = ev_now(loop);
	qtimer_scheduler scheduler;
	scheduler.set_loop(loop);
	scheduler.schedule_repeat_timer(
		[&server, loop, creation_time](qtimer& timer) {
			UNUSED(timer);
			UNUSED(creation_time);
			// debug_print(LOG_LEVEL_0, __LOGTAG__, "iam alive - t:%5.2fs", ev_now(loop) - creation_time );
			if (server.is_run()) {
				ev_break(loop, EVBREAK_ONE);
			}
		},
		15);

	ev_run(loop, 0);

	discord_util::shutdown();

	return 0;
}
