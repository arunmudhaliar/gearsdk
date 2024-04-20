//
//  Copyright 2024 homenet25
//  main.cpp
//  networkclient
//
//  Created by Arun A on 25/10/23.
//

#include "networkclient_tester.hpp"
#undef __LOGTAG__
#define __LOGTAG__ "qclient"
#include <netdb.h>
#include <unistd.h>

static qstring version_string = "0.1";
static unsigned version_code = 0;

int32_t main(int32_t argc, const char* argv[]) {
	if (argc == 2 && strcmp(argv[1], "--version") == 0) {
		DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "version %s(%d)", version_string.c_str(), version_code);
		return 0;
	}

	init_gsdk();
	DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "version %s(%d)", version_string.c_str(), version_code);
	print_common_info();
	qstring host = "127.0.0.1";
	//    qstring host = "192.168.0.230";
	qstring port = "4000";
	if (argc == 3) {
		host = argv[1];
		port = argv[2];
		const struct addrinfo hints = {.ai_family = PF_UNSPEC, .ai_socktype = SOCK_DGRAM, .ai_protocol = IPPROTO_UDP};
		struct addrinfo* peer = nullptr;
		if (getaddrinfo(host.c_str(), port.c_str(), &hints, &peer) != 0) {
			DEBUG_PRINT_ERROR(__LOGTAG__, "Failed to resolve host. Exiting !!!");
			DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "Usage : <executable> 'ip address' 'port'");
			return -1;
		}
		if (peer) {
			freeaddrinfo(peer);
			peer = nullptr;
		}
	} else {
		DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "Usage : <executable> 'ip address' 'port'. Ignore for debug builds running locally.");
	}
	DEBUG_PRINT_IMPORTANT(__LOGTAG__, "host:%s, port:%s", host.c_str(), port.c_str());

	networkclient_tester tester;
	tester.run(host, port, 1, 7, 10);

	return 0;
}
