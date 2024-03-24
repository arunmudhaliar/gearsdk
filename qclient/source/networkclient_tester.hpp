//
//  networkclient_tester.hpp
//  networkclient_tester
//
//  Created by Arun A on 29/09/23.
//

#ifndef networkclient_tester_hpp
#define networkclient_tester_hpp

#include "gameclient.hpp"

class networkclient_tester {
   public:
	void run(const qstring& host, const qstring& port, int send_interval, int close_timeout, int shutdown_server_after = 0);

   private:
	static void send_msg_timer_cb(EV_P_ ev_timer* w, int revents);
	static void delete_cb(EV_P_ ev_timer* w, int revents);
	static void shutdown_cb(EV_P_ ev_timer* w, int revents);
	std::vector<gameclient*> clientList;
};

#endif /* networkclient_tester_hpp */
