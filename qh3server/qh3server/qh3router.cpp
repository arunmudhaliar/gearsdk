//
//  Copyright 2024 homenet25
//  qh3router.cpp
//  qh3server
//
//  Created by Arun A on 21/12/23.
//

#include "qh3router.hpp"

using namespace client;
qh3router::qh3router(const server_config_in& config) : config(config) {}
qh3router::~qh3router() {
	for (auto r : routes) {
		GX_DELETE(r);
	}
	for (auto r : unresponsive_routes) {
		GX_DELETE(r);
	}
	GX_DELETE(command_feedback_route);
	GX_DELETE(command_route);
	GX_DELETE(hiredis_async);
	GX_DELETE(hiredis);
	debug_print(LOG_LEVEL_0, __LOGTAG__, "qh3router destroyed...");
}

void qh3router::shutdown_zk() {
	GX_DELETE(zkconfig);
#if ENABLE_ZK
	if (qzk != nullptr) {
		qzk->shutdown();
		debug_print_important(__LOGTAG__, "waiting for qh3router services to finish !!!");
		struct ev_loop* wait_loop = ev_loop_new();
		qtimer_scheduler wait_scheduler;
		wait_scheduler.set_loop(wait_loop);
		qtimer* wait_timer = wait_scheduler.schedule_repeat_timer(
			[this, wait_loop](qtimer& timer) {
				UNUSED(timer);
				if (!qzk->is_running()) {
					debug_print_important(__LOGTAG__, "qzk service finished !!!");
					ev_break(wait_loop, EVBREAK_ONE);
				} else {
					debug_print_important(__LOGTAG__, "qzk running %d %s !!!", qzk->is_zk_active(), qzookeeper::state_to_string(qzk->get_connection_state()));
				}
			},
			3);
		ev_run(wait_loop, 0);
		wait_scheduler.cancel_and_destroy_timer(wait_timer);
		ev_loop_destroy(wait_loop);
		GX_DELETE(qzk);
	}
#endif
}

void qh3router::recv_cb(EV_P_ ev_io* w, int revents) {
	UNUSED(loop);
	UNUSED(revents);
	qh3router* router = (qh3router*) w->data;
	static uint8_t buf[65535];

	while (1) {
		struct sockaddr_storage peer_addr;
		socklen_t peer_addr_len = sizeof(peer_addr);
		memset(&peer_addr, 0, peer_addr_len);

		ssize_t read = recvfrom(router->sock, buf, sizeof(buf), 0, (struct sockaddr*) &peer_addr, &peer_addr_len);

		if (read < 0) {
			if ((errno == EWOULDBLOCK) || (errno == EAGAIN)) {
				debug_print(LOG_LEVEL_6, __LOGTAG__, "recv would block");
				break;
			}

			debug_print_error(__LOGTAG__, "failed to read");
			return;
		}

		if (router->routes.size() == 0) {
			//            debug_print_error(__LOGTAG__, "zero routes !!!");
			return;
		}

#if LOG_LEVEL >= LOG_LEVEL_5
		char name[INET6_ADDRSTRLEN];
		char port[10];
		getnameinfo((struct sockaddr*) &peer_addr, sizeof(struct sockaddr), name, sizeof(name), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
		debug_print(LOG_LEVEL_0, __LOGTAG__, "from client %s:%s read:%d", name, port, read);
#endif

		struct sockaddr* peer_addr_to_pass = (struct sockaddr*) &peer_addr;
		memcpy((void*) &buf[read], (void*) peer_addr_to_pass, peer_addr_len);

		unsigned long crc = crc32(0L, Z_NULL, 0);
		//        crc_ = essentials::mod_crc32_z(crc_, (const unsigned char*)dcid,
		//        dcid_len);
		crc = essentials::mod_crc32_z(crc, (const unsigned char*) &buf[read], peer_addr_len);
		int index = crc % (int) router->routes.size();
		route* route = router->routes[index];
		route->relay(buf, read + peer_addr_len);
	}
}

bool qh3router::is_route_available(const route* r) {
	return (std::find(routes.begin(), routes.end(), r) == routes.end());
}

void qh3router::on_qhiredis_async_key_expired(const qstring& expired_key) {}

void qh3router::on_qhiredis_async_key_changed(const qstring& modified_key, const qstring& event) {
	UNUSED(modified_key);
}

void qh3router::on_qhiredis_connect() {}

void qh3router::on_qhiredis_disconnect() {}

void qh3router::recv_return_cb(EV_P_ ev_io* w, int revents) {
	UNUSED(loop);
	UNUSED(revents);
	qh3router* router = (qh3router*) w->data;
	static uint8_t buf_return[65535];

	while (1) {
		struct sockaddr_storage peer_addr;
		socklen_t peer_addr_len = sizeof(peer_addr);
		memset(&peer_addr, 0, peer_addr_len);

		ssize_t read = recvfrom(router->sock_return, buf_return, sizeof(buf_return), 0, (struct sockaddr*) &peer_addr, &peer_addr_len);

		if (read < 0) {
			if ((errno == EWOULDBLOCK) || (errno == EAGAIN)) {
				debug_print(LOG_LEVEL_5, __LOGTAG__, "recv_return would block");
				break;
			}

			debug_print_error(__LOGTAG__, "recv_return - failed to read");
			return;
		}

		if (router->routes.size() == 0) {
			debug_print_error(__LOGTAG__, "zero routes !!!");
			return;
		}

#if LOG_LEVEL >= LOG_LEVEL_5
		char name[INET6_ADDRSTRLEN];
		char port[10];
		getnameinfo((struct sockaddr*) &peer_addr, sizeof(struct sockaddr), name, sizeof(name), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
		debug_print(LOG_LEVEL_0, __LOGTAG__, "recv_return - from server %s:%s read:%d", name, port, read);
#endif

		EV_START_RECORD(router_server_port_deserialise_time);
		read = read - ORIGINAL_CLIENT_ADDR_SZ;	// remove the size of port bytes from
												// actual packet (quiche packet)
		const uint8_t* port_number_info = &buf_return[read];
		uint16_t port_from_packet = 0;
		memcpy(&port_from_packet, port_number_info, sizeof(uint16_t));
		port_from_packet = ntohs(port_from_packet);

		// update the ip adress to re-transmit to original client
		essentials::update_port((struct sockaddr*) &peer_addr, port_from_packet);
		struct sockaddr* client_info = (struct sockaddr*) &peer_addr;
		memcpy(&client_info->sa_data[2], &buf_return[read + 2],
			   4);	// 0.0.0.0 = 4 bytes
		EV_PRINT_IF_ELAPSED(router_server_port_deserialise_time, __LOGTAG__, "router_server_port_deserialise_time t:%lu ms", 10);
		//

		ssize_t sent = sendto(router->sock, buf_return, read, 0, client_info, peer_addr_len);
#if LOG_LEVEL >= LOG_LEVEL_5
		getnameinfo((struct sockaddr*) &peer_addr, sizeof(struct sockaddr), name, sizeof(name), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
		debug_print(LOG_LEVEL_0, __LOGTAG__, "recv_return - send to %s:%s bytes:%d", name, port, sent);
#endif

		if (sent != read) {
			char name[INET6_ADDRSTRLEN];
			char port[10];
			getnameinfo((struct sockaddr*) &peer_addr, sizeof(struct sockaddr), name, sizeof(name), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
			debug_print_error(__LOGTAG__, "ERROR recv_return - sending to %s:%s", name, port);
			debug_print_error(__LOGTAG__, "recv_return - failed to send %d<>%d", sent, read);
		}
	}
}

int qh3router::next_available_port(const qstring& host, port_range& range, int& index) {
	int min = range.MIN_VAL;
	int max = range.MAX_VAL;
	int start_index = index;
	for (int port = min + start_index; port < max; port++) {
		index++;
		if (is_port_available(host, port) == port) {
			return port;
		}
	}
	return 0;
}

int qh3router::is_port_available(const qstring& host, int port_number) {
	qstring port = qstring::format_string("%d", port_number);
	const struct addrinfo HINTS = {.ai_family = PF_UNSPEC, .ai_socktype = SOCK_DGRAM, .ai_protocol = IPPROTO_UDP};
	struct addrinfo* local;
	if (getaddrinfo(host.c_str(), port.c_str(), &HINTS, &local) != 0) {
		debug_print_error(__LOGTAG__, "f:is_port_available - failed to resolve host. port:%d", port_number);
		return -1;
	}

	int sock = socket(local->ai_family, SOCK_DGRAM, 0);
	if (sock < 0) {
		debug_print_error(__LOGTAG__, "f:is_port_available - failed to create socket. port:%d", port_number);
		freeaddrinfo(local);
		return -1;
	}

	if (fcntl(sock, F_SETFL, O_NONBLOCK) != 0) {
		debug_print_error(__LOGTAG__, "f:is_port_available - failed to make socket non-blocking. port:%d", port_number);
		freeaddrinfo(local);
		close(sock);
		return -1;
	}

	if (bind(sock, local->ai_addr, local->ai_addrlen) < 0) {
		debug_print_error(__LOGTAG__, "f:is_port_available - failed to bind socket. port:%d", port_number);
		freeaddrinfo(local);
		close(sock);
		return -1;
	}

	freeaddrinfo(local);
	close(sock);
	return port_number;
}

void qh3router::cmd_feedback_from_client(struct sockaddr* client_addr, const qstring& cmd) {
	char host[INET6_ADDRSTRLEN];
	char port[10];
	int ret = getnameinfo(client_addr, sizeof(struct sockaddr), host, sizeof(host), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
	if (ret != 0) {
		debug_print_error(__LOGTAG__,
						  "getnameinfo() failed in cmd_feedback_from_client on "
						  "command '%s', returning !!!",
						  cmd.c_str());
		return;
	}

	// find the route in active list
	route* found = is_in_active_routes(host, port);

	// if not found, check if its in unresponsive list
	if (found == nullptr) {
		found = is_in_unresponsive_routes(host, port);
		if (found) {
			// remove and push to routes. make the route active.
			push_to_routes(remove_from_unresponsive_routes(found));
		}
	}

	// if found delete
	if (found) {
		if (cmd.compare(qstring::format_string("shut-ack-%s", port)) == 0) {  // shut downed
			if (remove_from_active_routes(found) || remove_from_unresponsive_routes(found)) {
				GX_DELETE(found);
				// if no routes then shut down command center
				if (routes.size() == 0 && unresponsive_routes.size() == 0) {
					conn_io_req_res* req = conn_io_req_res::create("/shutdown_cmd_center", "");
					qh3client_helper::send_async_request<client::qh3client>(
						command_route->host, command_route->port, req, nullptr,
						[](conn_io_req_res* request, conn_io_req_res* response, void* client_specific_data, void* arg, bool success) {
							UNUSED(response);
							UNUSED(client_specific_data);
							UNUSED(arg);
							UNUSED(success);
							debug_print(LOG_LEVEL_0, __LOGTAG__, "f:cmd_feedback_from_client - shutdown-return");
						},
						1);
				}
			}
		} else if (cmd.compare(qstring::format_string("hb-%s", port)) == 0) {  // HB
			found->refresh_hb_timestamp(mainloop);
			debug_print(LOG_LEVEL_6, __LOGTAG__, "f:cmd_feedback_from_client - HB received route %s:%s", found->host.c_str(), found->port.c_str());
		}
	} else {
		// check if its command server or not
		if (command_route->host == host && command_route->port == port) {
			if (cmd.compare(qstring::format_string("shut-ack-%s", port)) == 0) {  // shut downed
				assert(routes.size() == 0);										  // command center must be destroyed last.
				debug_print(LOG_LEVEL_0, __LOGTAG__, "f:cmd_feedback_from_client - Removed command-route %s:%s from router", host, port);
				GX_DELETE(command_route);
				ev_break(mainloop, EVBREAK_ONE);
			} else if (cmd.compare(qstring::format_string("hb-%s", port)) == 0) {  // HB
				command_route->refresh_hb_timestamp(mainloop);
				debug_print(LOG_LEVEL_6, __LOGTAG__, "f:cmd_feedback_from_client - HB received cmd route %s:%s", host, port);
			}
		} else {
			if (cmd.compare(qstring::format_string("qh3server-start-%s", port)) == 0) {
				create_qh3server_route(host, port);
			} else {
				debug_print_error(__LOGTAG__, "f:cmd_feedback_from_client - route not found %s:%s in the list !!!", host, port);
			}
		}
	}
	//
}

qtimer* qh3router::check_and_remove_unresponsive_routes(qtimer_scheduler& scheduler) {
	int timer_unresponsive_route_check_in_sec = zkconfig->get_int32("router/timer_unresponsive_route_check_in_sec", TIMER_UNRESPONSIVE_ROUTE_CHECK_IN_SECONDS);
	debug_print_important(__LOGTAG__, "check_and_remove_unresponsive_routes timer %d", timer_unresponsive_route_check_in_sec);
	qtimer* timer = scheduler.schedule_repeat_timer(
		[this](qtimer& timer) {
			int new_timer_val = zkconfig->get_int32("router/timer_unresponsive_route_check_in_sec", TIMER_UNRESPONSIVE_ROUTE_CHECK_IN_SECONDS);
			float diff = new_timer_val - timer.delay;
			if (GX_ABS(diff) > 1.0f) {
				debug_print_important(__LOGTAG__, "check_and_remove_unresponsive_routes timer updated from %5.2f to %d", timer.delay, new_timer_val);
				timer.update_delay(new_timer_val);
			}
			int timeout_unresponsive_routes_in_sec = zkconfig->get_int32("router/timeout_unresponsive_routes_in_sec", TIMEOUT_UNRESPONSIVE_ROUTE_IN_SECONDS);
			ev_tstamp now = ev_now(mainloop);
			std::vector<route*> expired;
			for (auto r : routes) {
				ev_tstamp elapsed = now - r->last_hb_received_time;
				if (elapsed < timeout_unresponsive_routes_in_sec) {
					continue;
				}
				debug_print(LOG_LEVEL_0, __LOGTAG__, "Expired active route %s:%s from router. elapsed:%f timeout_unresponsive_routes_in_sec:%d", r->host.c_str(), r->port.c_str(), elapsed, timeout_unresponsive_routes_in_sec);
				expired.push_back(r);
			}

			for (route* r : expired) {
				if (remove_from_active_routes(r)) {
					push_to_unresponsive_routes(r);
				} else {
					debug_warn(LOG_LEVEL_0, __LOGTAG__, "Not found in active route list !!!");
				}
			}
			expired.clear();

#if DESTROY_ROUTES_ON_URESPONSIVE_LIST
			// check and remove unresponsive routes
			for (auto r : unresponsive_routes) {
				ev_tstamp elapsed = now - r->last_hb_received_time;
				if (elapsed < (timeout_unresponsive_routes_in_sec << 1)) {
					continue;
				}
				debug_print(LOG_LEVEL_0, __LOGTAG__, "Expired unresponsive route %s:%s from router. elapsed:%f timeout_unresponsive_routes_in_sec:%d", r->host.c_str(), r->port.c_str(), elapsed, (timeout_unresponsive_routes_in_sec << 1));
				expired.push_back(r);
			}

			for (route* r : expired) {
				if (remove_from_unresponsive_routes(r)) {
					GX_DELETE(r);
				} else {
					debug_warn(LOG_LEVEL_0, __LOGTAG__, "Not found in inactive route list !!!");
				}
			}
		//
#endif
			// check for cmd center
			if (command_route) {
				ev_tstamp elapsed = now - command_route->last_hb_received_time;
				debug_warn_cond(__LOGTAG__, (elapsed > (timeout_unresponsive_routes_in_sec << 1)), "Unresponsive command center !!!");
			}
		},
		timer_unresponsive_route_check_in_sec);
	return timer;
}

qtimer* qh3router::update_redis_about_servers(qtimer_scheduler& scheduler) {
	int grace_time = 10;
	int expire_timer_unresponsive_route_zk_check_in_sec = zkconfig->get_int32("router/expire_timer_unresponsive_route_zk_check_in_sec", EXPIRE_TIMER_UNRESPONSIVE_ROUTE_ZK_CHECK_IN_SECONDS);
	const qstring& hash_key = qstring::format_string("servers:%s", gsdk::device::public_ip);
	hiredis->set_hash_value(hash_key, "router", qstring::format_string("%s:%s", config.host.c_str(), config.port.c_str()));
	hiredis->set_hash_value(hash_key, "router-return", qstring::format_string("%s:%d", config.host.c_str(), config.router_port_return));
	hiredis->expire_key(hash_key, expire_timer_unresponsive_route_zk_check_in_sec + grace_time);

	debug_print_important(__LOGTAG__, "update_redis_about_servers timer %d", expire_timer_unresponsive_route_zk_check_in_sec);
	qtimer* timer = scheduler.schedule_repeat_timer(
		[this, hash_key, grace_time](qtimer& timer) {
			int next_expire_in_sec = zkconfig->get_int32("router/expire_timer_unresponsive_route_zk_check_in_sec", EXPIRE_TIMER_UNRESPONSIVE_ROUTE_ZK_CHECK_IN_SECONDS);
			float diff = next_expire_in_sec - timer.delay;
			if (GX_ABS(diff) > 1.0f) {
				debug_print_important(__LOGTAG__, "update_redis_about_servers timer updated from %5.2f to %d", timer.delay, next_expire_in_sec);
				timer.update_delay(next_expire_in_sec);
			}
			hiredis->set_hash_value(hash_key, "router", qstring::format_string("%s:%s", config.host.c_str(), config.port.c_str()));
			hiredis->set_hash_value(hash_key, "router-return", qstring::format_string("%s:%d", config.host.c_str(), config.router_port_return));
			for (auto r : routes) {
				hiredis->set_hash_value(hash_key, qstring::format_string("server-%s", r->port.c_str()), qstring::format_string("%s:%s", r->host.c_str(), r->port.c_str()));
			}
			for (auto r : unresponsive_routes) {
				hiredis->set_hash_value(hash_key, qstring::format_string("server-%s", r->port.c_str()), qstring::format_string("%s:%s", r->host.c_str(), r->port.c_str()));
			}
			// cmd
			hiredis->set_hash_value(hash_key, "command_center", qstring::format_string("%s:%d", config.host.c_str(), config.command_port));
			hiredis->expire_key(hash_key, next_expire_in_sec + grace_time);
		},
		expire_timer_unresponsive_route_zk_check_in_sec);
	return timer;
}

route* qh3router::is_in_active_routes(const qstring& host, const qstring& port) const {
	for (auto r : routes) {
		if (r->host == host && r->port == port) {
			return r;
		}
	}
	return nullptr;
}

route* qh3router::is_in_unresponsive_routes(const qstring& host, const qstring& port) const {
	for (auto r : unresponsive_routes) {
		if (r->host == host && r->port == port) {
			return r;
		}
	}
	return nullptr;
}

route* qh3router::remove_from_active_routes(route* r) {
	size_t old_sz = routes.size();
	routes.erase(std::remove(routes.begin(), routes.end(), r), routes.end());
	if (old_sz != routes.size()) {
		debug_warn(LOG_LEVEL_0, __LOGTAG__, "Removed route %s:%s from ACTIVE list", r->host.c_str(), r->port.c_str());
		const qstring& hash_key = qstring::format_string("servers:%s", gsdk::device::public_ip);
		hiredis->delete_hash_field(hash_key, qstring::format_string("server-%s", r->port.c_str()));
		return r;
	}
	return nullptr;
}

route* qh3router::remove_from_unresponsive_routes(route* r) {
	size_t old_sz = unresponsive_routes.size();
	unresponsive_routes.erase(std::remove(unresponsive_routes.begin(), unresponsive_routes.end(), r), unresponsive_routes.end());
	if (old_sz != unresponsive_routes.size()) {
		debug_print(LOG_LEVEL_0, __LOGTAG__, "Removed route %s:%s from UNRESPONSIVE list", r->host.c_str(), r->port.c_str());
		const qstring& hash_key = qstring::format_string("servers:%s", gsdk::device::public_ip);
		hiredis->delete_hash_field(hash_key, qstring::format_string("server-%s", r->port.c_str()));
		return r;
	}
	return nullptr;
}

void qh3router::push_to_unresponsive_routes(route* r) {
	if (r == nullptr || std::find(unresponsive_routes.begin(), unresponsive_routes.end(), r) != unresponsive_routes.end()) {
		return;
	}
	unresponsive_routes.push_back(r);
	debug_print(LOG_LEVEL_0, __LOGTAG__, "Pushed route %s:%s to UNRESPONSIVE list.", r->host.c_str(), r->port.c_str());
}

void qh3router::push_to_routes(route* r) {
	if (r == nullptr || std::find(routes.begin(), routes.end(), r) != routes.end()) {
		return;
	}
	debug_warn(LOG_LEVEL_0, __LOGTAG__, "Re-pushed route %s:%s to ACTIVE list", r->host.c_str(), r->port.c_str());
	const qstring& hash_key = qstring::format_string("servers:%s", gsdk::device::public_ip);
	hiredis->set_hash_value(hash_key, qstring::format_string("server-%s", r->port.c_str()), qstring::format_string("%s:%s", r->host.c_str(), r->port.c_str()));
	routes.push_back(r);
}

route* qh3router::create_qh3server_route(const qstring& host, const qstring& port, pid_t child_process_id) {
	route* route_obj = DEBUG_NEW route(host, port, server_counter++);
	route_obj->refresh_hb_timestamp(mainloop);
	routes.push_back(route_obj);
	debug_print_important2(__LOGTAG__, "spawned qh3server: %s:%s id-%d", host.c_str(), port.c_str(), route_obj->server_id);
	route_obj->create_bridge(mainloop, route_obj, nullptr);
	return route_obj;
}
