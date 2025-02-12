//
//  Copyright 2024 homenet25
//  qh3router_structs.h
//  qh3server
//
//  Created by Arun A on 30/12/23.
//

#ifndef qh3router_structs_h
#define qh3router_structs_h

#include "../../networkcommon/source/essentials.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "qh3router_structs"

struct route;
class bridge_command_center {
   public:
	virtual const std::vector<route*>& get_routes() = 0;
	virtual void cmd_feedback_from_client(struct sockaddr* client_addr, const qstring& cmd) = 0;
};

struct route {
	route(const qstring& host, const qstring& port, int server_id_) : host(host), port(port), server_id(server_id_) {}
	~route() { close_bridge_socket(); }
	int create_bridge(struct ev_loop* loop, void* arg, void (*router_command_recv_cb_ptr)(EV_P_ ev_io* w, int revents) = nullptr);
	int close_bridge_socket();
	ssize_t relay(uint8_t* buf, ssize_t len);
	void refresh_hb_timestamp(struct ev_loop* loop);
	qstring host;
	qstring port;
	int server_id = -1;
	int bridge_sock = -1;
	struct addrinfo* peer = nullptr;
	ev_io command_watcher;
	void* arg = nullptr;
	pid_t child_process_id = -1;
	ev_tstamp last_hb_received_time = 0;
};
struct port_range {
	const int MIN_VAL = 5100;
	const int MAX_VAL = 5200;
};

#endif /* qh3router_structs_h */
