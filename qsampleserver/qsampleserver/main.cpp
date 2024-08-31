//
//  main.cpp
//  qsampleserver
//
//  Created by Arun A on 30/06/24.
//

#include "../../qserver/source/gameserver.hpp"
#include "../../qutils/discord_util.hpp"
#include "../../servercommon/source/servercommon.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "qsampleserver"

static qstring version_string = "0.1";
static unsigned version_code = 1;

#define DISCORD_WEBHOOK "https://discord.com/api/webhooks/1214655271985750056/ExoVMFlV4Pu_t3fxRQqi_Jbo7SUP1Q5lTnkC6-eG81jKaCxqhggLD_sYg41WL38UmgQe"
// https://discord.com/api/webhooks/1214655271985750056/ExoVMFlV4Pu_t3fxRQqi_Jbo7SUP1Q5lTnkC6-eG81jKaCxqhggLD_sYg41WL38UmgQe

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
	init_gsdk();
	gsdk::servercommon::init_server_common();
	discord_util::set_web_hook(DISCORD_WEBHOOK);
	gsdk::set_warn_callback(warn_callback);
	gsdk::set_error_callback(error_callback);
	gsdk::set_assert_callback(assert_callback);
	qstring host = "127.0.0.1";
	qstring port = "4000";
	qstring mongodb_uri = "mongodb://18.208.130.48:27017";	// "mongodb://192.168.0.230:27017";
	qstring redis_ip = "18.208.130.48";
	qstring zk_uri = "18.208.130.48:2181";
	uint16_t redis_port = 6379;
	fs::path rootDir;
	int result = essentials::resolve_cmd_line_args(__LOGTAG__, argc, argv, version_string, version_code, host, port, mongodb_uri, rootDir, redis_ip, redis_port, zk_uri);
	if (result < 0) {
		discord_util::shutdown();
		exit(0);
	}
	gameserver server;
	server.run(host, port, rootDir, redis_ip, redis_port);

	// dummy run loop
	struct ev_loop* loop = ev_default_loop(0);
	ev_tstamp creation_time = ev_now(loop);
	qtimer_sceduler scheduler;
	scheduler.set_ev_lopp(loop);
	scheduler.schedule_repeat_timer(
		[&server, loop, creation_time](qtimer& timer) {
			UNUSED(timer);
			UNUSED(creation_time);
			// DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "iam alive - t:%5.2fs", ev_now(loop) - creation_time );
			if (server.is_run()) {
				ev_break(loop, EVBREAK_ONE);
			}
		},
		15);

	ev_run(loop, 0);

	discord_util::shutdown();
	
	return 0;
}
