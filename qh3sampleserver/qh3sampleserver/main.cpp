//
//  main.cpp
//  qh3sampleserver
//
//  Created by Arun A on 28/06/24.
//

#include "../../common/sdktypes.hpp"
#include "../../qh3server/qh3server/http3_command_server.hpp"
#include "../../qh3server/qh3server/http3_sample_server.hpp"
#include "../../qh3server/qh3server/qh3simple_router.hpp"
#include "../../qutils/discord_util.hpp"
#include "../../servercommon/source/servercommon.hpp"

static qstring version_string = "0.1";
static unsigned version_code = 1;

#undef __LOGTAG__
#define __LOGTAG__ "qh3sampleserver-main"

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
	gsdk::servercommon::init_server_common();
	gsdk::set_warn_callback(warn_callback);
	gsdk::set_error_callback(error_callback);
	gsdk::set_assert_callback(assert_callback);
	// main http server

	qstring host = "127.0.0.1";
	qstring port = "4004";

	qstring mongodb_uri = "mongodb://18.208.130.48:27017";	//"mongodb://192.168.0.230:27017"
	qstring redis_ip = "18.208.130.48";
	qstring zk_uri = "18.208.130.48:2181";

	uint16_t redis_port = 6379;
	fs::path rootDir;
	int result = essentials::resolve_cmd_line_args(__LOGTAG__, argc, argv, version_string, version_code, host, port, mongodb_uri, rootDir, redis_ip, redis_port, zk_uri);
	if (result < 0) {
		exit(0);
	}

	//    http3_sample_server server(mongodb_uri.c_str(), redis_ip.c_str(),
	//    redis_port, zk_uri); server.run(host, port, rootDir, nullptr, 4010, 0);   // port return not used, so passing 0

	server_config_in config(host, port, mongodb_uri, redis_ip, redis_port, rootDir, nullptr, 4010, port, zk_uri, 4005);
	http3_sample_router router(config);
	router.run<http3_command_server, http3_sample_server>();

	DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "main suspending for 5 seconds !!!");
	sleep(5);
	DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "main exiting !!!");
	return 0;
}
