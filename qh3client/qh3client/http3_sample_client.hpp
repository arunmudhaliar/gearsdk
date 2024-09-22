//
//  Copyright 2024 homenet25
//  http3_sample_client.hpp
//  qh3client
//
//  Created by Arun A on 03/11/23.
//

#ifndef http3_sample_client_hpp
#define http3_sample_client_hpp

#include "../../networkcommon/source/message.hpp"
#include "qh3client_helper.hpp"

#include <atomic>

#undef __LOGTAG__
#define __LOGTAG__ "http3_sample_client"

class http3_sample_client : public qtimer_scheduler {
   public:
	http3_sample_client() {}
	http3_sample_client(const qstring& host, const qstring& port);
	~http3_sample_client();
	void create_connections(int count);
	void on_login_complete(const qstring& token, bool result);
	void set_server_info(const qstring& host, const qstring& port);

	bool is_finished() { return total_connections_issued == total_connections_returned; }

   private:
	qtimer* keep_alive_loop = nullptr;
	qstring host = "192.168.0.230";
	qstring port = "4004";
	std::atomic<int> total_connections_issued = 0;
	std::atomic<int> total_connections_returned = 0;
	std::atomic<int> total_connections_returned_success = 0;

	qstring session_token;
	qstring pid;
	message_parser msg_parser;
};
#endif /* http3_sample_client_hpp */
