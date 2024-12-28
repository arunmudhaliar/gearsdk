//
//  Copyright 2024 homenet25
//  main.cpp
//  qh3server
//
//  Created by Arun A on 30/10/23.
//

#include "../../common/sdktypes.hpp"
#include "../../common/signal_handler/signal_handler.hpp"
#include "../../qutils/discord_util.hpp"
#include "../../servercommon/source/servercommon.hpp"
#include "http3_command_server.hpp"
#include "http3_sample_server.hpp"
#include "qh3router.hpp"

static qstring version_string = "0.1";
static unsigned version_code = 1;
#define DISCORD_WEB_HOOK "https://discord.com/api/webhooks/1207911659214082058/A0S49aiBOJKVZJk5FUUQaAw3Qxl2oRmRFdf7R93B8Y60QPuagXS0F3gLKS3yYRQrTyo4"

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
	signal_handler::setup_signal_handler();

	init_gsdk();
	gsdk::servercommon::init_server_common();
	discord_util::initialize_with_webhook_url(DISCORD_WEB_HOOK);
	gsdk::set_warn_callback(warn_callback);
	gsdk::set_error_callback(error_callback);
	gsdk::set_assert_callback(assert_callback);
	// main http server

	qstring host = "127.0.0.1";
	qstring port = "4004";

	qstring mongodb_uri = "mongodb://3.109.144.159:27017";	//"mongodb://192.168.0.230:27017"
	qstring redis_ip = "3.109.144.159";
	qstring zk_uri = "3.109.144.159:2181";

	uint16_t redis_port = 6379;
	fs::path root_dir;
	int result = essentials::resolve_cmd_line_args(__LOGTAG__, argc, argv, version_string, version_code, host, port, mongodb_uri, root_dir, redis_ip, redis_port, zk_uri);
	if (result < 0) {
		discord_util::shutdown();
		exit(0);
	}

	//    http3_sample_server server(mongodb_uri.c_str(), redis_ip.c_str(),
	//    redis_port, zk_uri); server.run(host, port, rootDir, nullptr, 4010, 0);   // port return not used, so passing 0

	server_config_in config(host, port, mongodb_uri, redis_ip, redis_port, root_dir, nullptr, 4010, port, zk_uri, 4005, "qh3server-app");
	http3_sample_router router(config);
	router.run<http3_command_server, http3_sample_server>();

	discord_util::shutdown();

	debug_print(LOG_LEVEL_0, __LOGTAG__, "main suspending for 5 seconds !!!");
	sleep(5);
	debug_print(LOG_LEVEL_0, __LOGTAG__, "main exiting !!!");
	return 0;
}
