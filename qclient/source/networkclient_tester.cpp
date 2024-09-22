//
//  Copyright 2024 homenet25
//  networkclient_tester.cpp
//  networkclient_tester
//
//  Created by Arun A on 20/09/23.
//

#include "networkclient_tester.hpp"
#undef __LOGTAG__
#define __LOGTAG__ "networkclient"
#include <algorithm>
#include <unistd.h>

static bool sent_hello = false;
void networkclient_tester::send_msg_timer_cb(EV_P_ ev_timer* w, int revents) {
	UNUSED(revents);
	if (sent_hello) {
		return;
	}
	networkclient_tester* tester = (networkclient_tester*) w->data;
	debug_print_important(__LOGTAG__, "TIMEOUT MAIN %d", tester->client_list.size());
	std::vector<gameclient*> finished_list;
	for (int x = 0; x < (int) tester->client_list.size(); x++) {
		gameclient* client = tester->client_list[x];
		client->send_message("hello from client", true);
		//        client->sendMessage("hello12 from client", true);
		//        client->sendMessage("hello123 from client", true);
		//        client->sendMessage("hello1234 from client", true);
		//        sent_hello = true;
		if (client->is_runfinished()) {
			finished_list.push_back(client);
		}
	}

	for (auto it = finished_list.cbegin(); it != finished_list.cend(); it++) {
		gameclient* p = *it;
		size_t old_sz = tester->client_list.size();
		tester->client_list.erase(std::remove(tester->client_list.begin(), tester->client_list.end(), p), tester->client_list.end());
		if (old_sz != tester->client_list.size()) {
			GX_DELETE(p);
		}
	}
	if (tester->client_list.size() == 0) {
		ev_break(loop, EVBREAK_ALL);
	}
}

void networkclient_tester::delete_cb(EV_P_ ev_timer* w, int revents) {
	UNUSED(loop);
	UNUSED(w);
	UNUSED(revents);
	//    networkclient_tester* tester = (networkclient_tester*)w->data;
	//    debug_print_important(__LOGTAG__, "TIMEOUT DELETE %d", tester->client_list.size());
	//    //    int finished = 0;
	//    for (int x = 0;x < tester->client_list.size();x++) {
	//        gameclient* client = tester->client_list[x];
	//        client->close();
	//        //        //client->ForceRelease();
	//        break;
	//        //        if (client->IsRunFinished()) {
	//        //            finished++;
	//        //        }
	//    }
	//    //    ev_break(loop, EVBREAK_ONE);
}

void networkclient_tester::shutdown_cb(EV_P_ ev_timer* w, int revents) {
	UNUSED(loop);
	UNUSED(w);
	UNUSED(revents);
	//    networkclient_tester* tester = (networkclient_tester*)w->data;
	//    debug_print_important(__LOGTAG__, "SHUTDOWN EVENT %d", tester->client_list.size());
}

void networkclient_tester::run(const qstring& host, const qstring& port, int send_interval, int close_timeout, int shutdown_server_after) {
	const int TOTAL_LOOP = 1;
	const int CLIENTS_PER_LOOP = 5;
	const int WAIT_SEC_AFTER_LOOP = 6;
	for (int y = 0; y < TOTAL_LOOP; y++) {
		for (int x = 0; x < CLIENTS_PER_LOOP; x++) {
			gameclient* new_client = DEBUG_NEW gameclient();
			//			new_client->shutdown_client = (y == TOTAL_LOOP - 1 && x == CLIENTS_PER_LOOP - 1);
			new_client->run(host, port);
			client_list.push_back(new_client);
		}

		// send timer
		struct ev_loop* loop = ev_default_loop(0);
		static ev_timer tw;
		ev_timer_init(&tw, send_msg_timer_cb, send_interval, 1);
		tw.data = this;
		ev_timer_start(loop, &tw);

		// close timer
		static ev_timer tw2;
		ev_timer_init(&tw2, delete_cb, close_timeout, 1);
		tw2.data = this;
		ev_timer_start(loop, &tw2);

		if (shutdown_server_after > 0) {
			// shutdown timer
			static ev_timer tw3;
			ev_timer_init(&tw3, shutdown_cb, shutdown_server_after, 1);
			tw2.data = this;
			ev_timer_start(loop, &tw2);
		}
		ev_run(loop, 0);

		// destroy the list
		for (int x = 0; x < (int) client_list.size(); x++) {
			gameclient* client = client_list[x];
			GX_DELETE(client);
		}
		client_list.clear();
		usleep(WAIT_SEC_AFTER_LOOP * 1000 * 1000);	// wait for wait_sec_after_loop sec.
	}
}
