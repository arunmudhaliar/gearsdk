//
//  Copyright 2024 homenet25
//  qh3simple_router.cpp
//  qh3server
//
//  Created by Arun A on 21/12/23.
//

#include "qh3simple_router.hpp"

using namespace client;
qh3simple_router::qh3simple_router(const server_config_in& config) : config(config) {}
qh3simple_router::~qh3simple_router() {
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
	DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "qh3simple_router destroyed...");
}

int qh3simple_router::run() {
	// router object needs to be inited first, since this info is needed by child
	// qh3server child process
	const struct addrinfo hints = {.ai_family = PF_UNSPEC, .ai_socktype = SOCK_DGRAM, .ai_protocol = IPPROTO_UDP};
	freeaddrinfo(router);
	if (getaddrinfo(config.host.c_str(), config.port.c_str(), &hints, &router) != 0) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "failed to resolve host");
		return -1;
	}
	// return
	freeaddrinfo(router_return);
	if (getaddrinfo(config.host.c_str(), qstring::format_string("%d", config.router_port_return).c_str(), &hints, &router_return) != 0) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "failed to resolve host (port_return)");
		freeaddrinfo(router);
		router = nullptr;
		return -1;
	}
	//

	//================================================
	if (sock != -1) {
		close(sock);
	}
	sock = socket(router->ai_family, SOCK_DGRAM, 0);
	if (sock < 0) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create socket");
		freeaddrinfo(router);
		freeaddrinfo(router_return);
		router_return = nullptr;
		router = nullptr;
		return -1;
	}

	if (fcntl(sock, F_SETFL, O_NONBLOCK) != 0) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "failed to make socket non-blocking");
		freeaddrinfo(router);
		freeaddrinfo(router_return);
		router_return = nullptr;
		router = nullptr;
		close(sock);
		return -1;
	}

	if (bind(sock, router->ai_addr, router->ai_addrlen) < 0) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "failed to connect socket");
		freeaddrinfo(router);
		freeaddrinfo(router_return);
		router_return = nullptr;
		router = nullptr;
		close(sock);
		return -1;
	}

	// return socket
	if (sock_return != -1) {
		close(sock_return);
	}
	sock_return = socket(router_return->ai_family, SOCK_DGRAM, 0);
	if (sock_return < 0) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create sock_return");
		freeaddrinfo(router);
		freeaddrinfo(router_return);
		router_return = nullptr;
		router = nullptr;
		close(sock);
		return -1;
	}

	if (fcntl(sock_return, F_SETFL, O_NONBLOCK) != 0) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "failed to make sock_return non-blocking");
		freeaddrinfo(router);
		freeaddrinfo(router_return);
		router_return = nullptr;
		router = nullptr;
		close(sock_return);
		close(sock);
		return -1;
	}

	if (bind(sock_return, router_return->ai_addr, router_return->ai_addrlen) < 0) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "failed to connect sock_return");
		freeaddrinfo(router);
		freeaddrinfo(router_return);
		router_return = nullptr;
		router = nullptr;
		close(sock_return);
		close(sock);
		return -1;
	}
	//

	mainloop = ev_default_loop(0);

	DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "Creating  command center !!!");
	// command server
	if (spawn_qh3server_command_server(config.host, config.command_port, config) == nullptr) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create command server !!!");
		freeaddrinfo(router);
		freeaddrinfo(router_return);
		router_return = nullptr;
		router = nullptr;
		close(sock_return);
		close(sock);
		shutdown_zk();
		GX_DELETE(hiredis_async);
		GX_DELETE(hiredis);
		return -1;
	}

	// spawn initial servers
	int spawned_servers = 0;  // never decrement
	int overflow = 0;
	int index = 0;
	pid_t parent_process_id = getpid();
	pid_t child_process_id = -1;
	while (spawned_servers < NO_OF_SERVERS_TO_SPAWN) {
		overflow++;
		if (overflow > 1000) {
			DEBUG_PRINT_ERROR(__LOGTAG__, "OVERFLOW ON SPAWNING SERVERS !!!");
			DEBUG_PRINT_ERROR(__LOGTAG__, "OVERFLOW ON SPAWNING SERVERS !!!");
			DEBUG_PRINT_ERROR(__LOGTAG__, "OVERFLOW ON SPAWNING SERVERS !!!");
			break;
		}
		int free_port = next_available_port(config.host, range, index);
		if (free_port == 0) {
			DEBUG_PRINT_ERROR(__LOGTAG__, "NO PORT AVAILABLE !!!");
			break;
		}
		bool fork_result = false;  // only valid inside FORK_QH3_SERVER preprocessor
		int result = spawn_qh3server(config.host, qstring::format_string("%d", free_port), config, child_process_id, fork_result);
#if FORK_QH3_SERVER
		UNUSED(result);
		if (fork_result) {	// forking is successfull
			if (child_process_id == 0) {
				break;	// child process has exited. no need to continue the loop.
			} else if (child_process_id > 0) {
				// cache child process ids for later use (shutdown events)
				// recommended design is to handle shutdown events by these child
				// process themselves.
				server_process_ids.push_back(child_process_id);
			}
		} else {
			if (parent_process_id == getpid()) {
				// allow the parent to continue
				DEBUG_PRINT_ERROR(__LOGTAG__, "forking failed, allowing parent(pid:%d) to continue !!!", parent_process_id);
				continue;
			}
		}
#else
		UNUSED(parent_process_id);
		if (result != 0) {
			continue;
		}
#endif
		spawned_servers++;
	}
	//

	if (child_process_id != 0) {
		DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "Star router !!!");

#if ENABLE_ZK
		GX_DELETE(qzk);
		qzk = DEBUG_NEW qzookeeper();
		int zk_result = qzk->connect(config.zk_uri);
		if (zk_result != 0) {
			freeaddrinfo(router);
			freeaddrinfo(router_return);
			router_return = nullptr;
			router = nullptr;
			close(sock_return);
			close(sock);
			DEBUG_PRINT_ERROR(__LOGTAG__, "zk failed to connect !!!, Exiting.");
			GX_DELETE(qzk);
			return -1;
		}

		GX_DELETE(zkconfig);
		zkconfig = DEBUG_NEW serverconfig(qzk);
#if DEV_BUILD
		fs::path config_path(config.rootDir / "configs/dev/runtime-config.json");
		zkconfig->load(config_path, qzk, "/qh3router");
#elif PROD_BUILD
		fs::path config_path(config.rootDir / "configs/prod/runtime-config.json");
		zkconfig->load(config_path, qzk, "/qh3router");
#else
		fs::path config_path(config.rootDir / "configs/dev/runtime-config.json");
		zkconfig->load(config_path, qzk, "/qh3router");
#endif
#endif

		GX_DELETE(hiredis);
		hiredis = DEBUG_NEW qhiredis(config.redis_ip, config.redis_port);
		if (hiredis->connect_redis() != 0) {
			DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create redis connection !!!");
			freeaddrinfo(router);
			freeaddrinfo(router_return);
			router_return = nullptr;
			router = nullptr;
			close(sock_return);
			close(sock);
			shutdown_zk();
			GX_DELETE(hiredis);
			return -1;
		}

		int expire_timer_unresponsive_route_zk_check_in_sec = zkconfig->get_int32("router/expire_timer_unresponsive_route_zk_check_in_sec", EXPIRE_TIMER_UNRESPONSIVE_ROUTE_ZK_CHECK_IN_SECONDS);
		const qstring& hash_key = qstring::format_string("servers:%s", gsdk::server::machine_public_ip);
		hiredis->set_hash_value(hash_key, "router", qstring::format_string("%s:%s", config.host.c_str(), config.port.c_str()));
		hiredis->set_hash_value(hash_key, "router-return", qstring::format_string("%s:%d", config.host.c_str(), config.router_port_return));
		hiredis->expire_key(hash_key, expire_timer_unresponsive_route_zk_check_in_sec);

		ev_io watcher;
		ev_io_init(&watcher, recv_cb, sock, EV_READ);
		ev_io_start(mainloop, &watcher);
		watcher.data = this;

		ev_io watcher_return;
		ev_io_init(&watcher_return, recv_return_cb, sock_return, EV_READ);
		ev_io_start(mainloop, &watcher_return);
		watcher_return.data = this;

		// refresh the hb timestamp.
		for (auto r : routes) {
			r->refresh_hb_timestamp(mainloop);
		}
		//

		hiredis_async = DEBUG_NEW qhiredis_async(config.redis_ip, config.redis_port, this);
		if (hiredis_async->connect_async_redis(mainloop) != 0) {
			DEBUG_PRINT_ERROR(__LOGTAG__, "failed to connect async hiredis, Exiting !!!");
			GX_DELETE(hiredis_async);
			freeaddrinfo(router);
			freeaddrinfo(router_return);
			router_return = nullptr;
			router = nullptr;
			close(sock_return);
			close(sock);
			shutdown_zk();
			GX_DELETE(hiredis);
			return -1;
		}

		qtimer_sceduler unresponsive_routes_scheduler;
		unresponsive_routes_scheduler.set_ev_lopp(mainloop);
		qtimer* unresponsive_timer = check_and_remove_unresponsive_routes(unresponsive_routes_scheduler);

		ev_loop(mainloop, 0);

		ev_io_stop(mainloop, &watcher_return);
		ev_io_stop(mainloop, &watcher);

		unresponsive_routes_scheduler.cancel_and_destroy_timer(unresponsive_timer);

		ev_loop_destroy(mainloop);

		freeaddrinfo(router);
		router = nullptr;
		freeaddrinfo(router_return);
		router_return = nullptr;
		close(sock_return);
		close(sock);
		shutdown_zk();
		DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "Stop router !!!");
		return 0;
	} else {
		freeaddrinfo(router);
		router = nullptr;
		freeaddrinfo(router_return);
		router_return = nullptr;
		DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "Child process exiting !!!");
		return 0;
	}
}

void qh3simple_router::shutdown_zk() {
	GX_DELETE(zkconfig);
#if ENABLE_ZK
	qzk->shutdown();
	DEBUG_PRINT_IMPORTANT(__LOGTAG__, "waiting for qh3simple_router services to finish !!!");
	struct ev_loop* wait_loop = ev_loop_new();
	qtimer_sceduler wait_scheduler;
	wait_scheduler.set_ev_lopp(wait_loop);
	qtimer* wait_timer = wait_scheduler.schedule_repeat_timer(
		[this, wait_loop](qtimer& timer) {
			UNUSED(timer);
			if (!qzk->is_running()) {
				DEBUG_PRINT_IMPORTANT(__LOGTAG__, "qzk service finished !!!");
				ev_break(wait_loop, EVBREAK_ONE);
			} else {
				DEBUG_PRINT_IMPORTANT(__LOGTAG__, "qzk running %d %s !!!", qzk->is_zk_active(), qzookeeper::state2String(qzk->get_connection_state()));
			}
		},
		3);
	ev_run(wait_loop, 0);
	wait_scheduler.cancel_and_destroy_timer(wait_timer);
	ev_loop_destroy(wait_loop);
	GX_DELETE(qzk);
#endif
}

void qh3simple_router::recv_cb(EV_P_ ev_io* w, int revents) {
	UNUSED(loop);
	UNUSED(revents);
	qh3simple_router* router = (qh3simple_router*) w->data;
	static uint8_t buf[65535];

	while (1) {
		struct sockaddr_storage peer_addr;
		socklen_t peer_addr_len = sizeof(peer_addr);
		memset(&peer_addr, 0, peer_addr_len);

		ssize_t read = recvfrom(router->sock, buf, sizeof(buf), 0, (struct sockaddr*) &peer_addr, &peer_addr_len);

		if (read < 0) {
			if ((errno == EWOULDBLOCK) || (errno == EAGAIN)) {
				DEBUG_PRINT(LOG_LEVEL_5, __LOGTAG__, "recv would block");
				break;
			}

			DEBUG_PRINT_ERROR(__LOGTAG__, "failed to read");
			return;
		}

		if (router->routes.size() == 0) {
			//            DEBUG_PRINT_ERROR(__LOGTAG__, "zero routes !!!");
			return;
		}

#if LOG_LEVEL >= LOG_LEVEL_5
		char name[INET6_ADDRSTRLEN];
		char port[10];
		getnameinfo((struct sockaddr*) &peer_addr, sizeof(struct sockaddr), name, sizeof(name), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
		DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "from client %s:%s read:%d", name, port, read);
#endif

		struct sockaddr* peer_addr_to_pass = (struct sockaddr*) &peer_addr;
		memcpy((void*) &buf[read], (void*) peer_addr_to_pass, peer_addr_len);

		unsigned long crc_ = crc32(0L, Z_NULL, 0);
		//        crc_ = essentials::mod_crc32_z(crc_, (const unsigned char*)dcid,
		//        dcid_len);
		crc_ = essentials::mod_crc32_z(crc_, (const unsigned char*) &buf[read], peer_addr_len);
		int index = crc_ % (int) router->routes.size();
		route* route = router->routes[index];
		route->relay(buf, read + peer_addr_len);
	}
}

bool qh3simple_router::is_route_available(const route* r) {
	return (std::find(routes.begin(), routes.end(), r) == routes.end());
}

void qh3simple_router::on_qhiredis_async_key_expired(const qstring& expired_key) {
	const qstring& hash_key = qstring::format_string("servers:%s", gsdk::server::machine_public_ip);
	if (hash_key.compare(expired_key) != 0) {
		return;
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

	// update the expiry
	int expire_timer_unresponsive_route_zk_check_in_sec = zkconfig->get_int32("router/expire_timer_unresponsive_route_zk_check_in_sec", EXPIRE_TIMER_UNRESPONSIVE_ROUTE_ZK_CHECK_IN_SECONDS);
	hiredis->expire_key(hash_key, expire_timer_unresponsive_route_zk_check_in_sec);
}

void qh3simple_router::recv_return_cb(EV_P_ ev_io* w, int revents) {
	UNUSED(loop);
	UNUSED(revents);
	qh3simple_router* router = (qh3simple_router*) w->data;
	static uint8_t buf_return[65535];

	while (1) {
		struct sockaddr_storage peer_addr;
		socklen_t peer_addr_len = sizeof(peer_addr);
		memset(&peer_addr, 0, peer_addr_len);

		ssize_t read = recvfrom(router->sock_return, buf_return, sizeof(buf_return), 0, (struct sockaddr*) &peer_addr, &peer_addr_len);

		if (read < 0) {
			if ((errno == EWOULDBLOCK) || (errno == EAGAIN)) {
				DEBUG_PRINT(LOG_LEVEL_5, __LOGTAG__, "recv_return would block");
				break;
			}

			DEBUG_PRINT_ERROR(__LOGTAG__, "recv_return - failed to read");
			return;
		}

		if (router->routes.size() == 0) {
			//            DEBUG_PRINT_ERROR(__LOGTAG__, "zero routes !!!");
			return;
		}

#if LOG_LEVEL >= LOG_LEVEL_5
		char name[INET6_ADDRSTRLEN];
		char port[10];
		getnameinfo((struct sockaddr*) &peer_addr, sizeof(struct sockaddr), name, sizeof(name), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
		DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "recv_return - from server %s:%s read:%d", name, port, read);
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
		DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "recv_return - send to %s:%s bytes:%d", name, port, sent);
#endif

		if (sent != read) {
			char name[INET6_ADDRSTRLEN];
			char port[10];
			getnameinfo((struct sockaddr*) &peer_addr, sizeof(struct sockaddr), name, sizeof(name), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
			DEBUG_PRINT_ERROR(__LOGTAG__, "ERROR recv_return - sending to %s:%s", name, port);
			DEBUG_PRINT_ERROR(__LOGTAG__, "recv_return - failed to send %d<>%d", sent, read);
		}
	}
}

route* qh3simple_router::spawn_qh3server_command_server(const qstring& host, const qstring& port, const server_config_in& config) {
	server_config_in* new_config = DEBUG_NEW server_config_in(host, port, config.mongodb_uri, config.redis_ip, config.redis_port, config.rootDir, nullptr, config.command_port, config.router_port, config.zk_uri, config.router_port_return);
	new_config->command_server = true;
	new_config->ref = this;

	if (pthread_create(&new_config->run_thread_id, nullptr, qh3simple_router::spawn_qh3server_internal, (void*) new_config) < 0) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "spawn_qh3server_command_server - could not create thread: %s - %d", strerror(errno), errno);
		GX_DELETE(new_config);
		return nullptr;
	}

	GX_DELETE(command_feedback_route);
	command_feedback_route = DEBUG_NEW route(host, qstring::format_string("%d", new_config->command_feedback_port), -1);
	command_feedback_route->create_bridge(mainloop, (bridge_command_center*) this, http3_command_server::command_feedback_recv_cb);
	command_feedback_route->refresh_hb_timestamp(mainloop);

	GX_DELETE(command_route);
	command_route = DEBUG_NEW route(host, port, server_counter++);
	DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "spawned qh3 command server: %s:%s id-%d", host.c_str(), port.c_str(), command_route->server_id);
	command_route->create_bridge(mainloop, command_route, nullptr);
	command_route->refresh_hb_timestamp(mainloop);
	return command_route;
}

int qh3simple_router::spawn_qh3server(const qstring& host, const qstring& port, const server_config_in& config, pid_t& child_process_id, bool& fork_result) {
#if FORK_QH3_SERVER
	DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "Parent process (PID: %d)", getpid());
	fork_result = false;
	child_process_id = fork();
	if (child_process_id < 0) {
		fork_result = false;
		DEBUG_PRINT_ERROR(__LOGTAG__, "fork failed !!!");
		return -1;
	}
	if (child_process_id == 0) {
		fork_result = true;
		// Code executed by the child process
		DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "Child process (PID: %d) [%d]", getpid(), child_process_id);
		server_config_in* new_config =
			DEBUG_NEW server_config_in(host, port, config.mongodb_uri, config.redis_ip, config.redis_port, config.rootDir, router, config.command_port, config.router_port, config.zk_uri, config.router_port_return);
		if (pthread_create(&new_config->run_thread_id, nullptr, qh3simple_router::spawn_qh3server_internal, (void*) new_config) < 0) {
			DEBUG_PRINT_ERROR(__LOGTAG__, "spawn_qh3server - could not create thread: %s - %d", strerror(errno), errno);
			GX_DELETE(new_config);
			return -1;
		}

		// Wait for the thread to finish
		if (pthread_join(new_config->run_thread_id, nullptr) != 0) {
			DEBUG_PRINT_ERROR(__LOGTAG__, "spawn_qh3server - could not join thread: %s - %d", strerror(errno), errno);
		}

		fflush(stdout);	 // Flush output
		return 0;
	} else if (child_process_id > 0) {
		fork_result = true;
		// Code executed by the parent process
		DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "Parent process after fork (PID: %d) [%d]", getpid(), child_process_id);
		route* child = DEBUG_NEW route(host, port, server_counter++);
		child->child_process_id = child_process_id;
		routes.push_back(child);
		DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "spawned qh3server: %s:%s id-%d", host.c_str(), port.c_str(), child->server_id);
		child->create_bridge(mainloop, child, nullptr);
		return 0;
	}
#else
	fork_result = false;
	server_config_in* new_config = DEBUG_NEW server_config_in(host, port, config.mongodb_uri, config.redis_ip, config.redis_port, config.rootDir, router, config.command_port, config.router_port, config.zk_uri, config.router_port_return);
	if (pthread_create(&new_config->run_thread_id, nullptr, qh3simple_router::spawn_qh3server_internal, (void*) new_config) < 0) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "spawn_qh3server - could not create thread: %s - %d", strerror(errno), errno);
		GX_DELETE(new_config);
		return -1;
	}
	route* child = DEBUG_NEW route(host, port, server_counter++);
	routes.push_back(child);
	DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "spawned qh3server: %s:%s id-%d", host.c_str(), port.c_str(), child->server_id);
	child->create_bridge(mainloop, child, nullptr);
	return 0;
#endif
	return -1;
}

void* qh3simple_router::spawn_qh3server_internal(void* data) {
	server_config_in* config = (server_config_in*) data;
	qstring& host = config->host;
	qstring& port = config->port;
	qstring& mongodb_uri = config->mongodb_uri;
	qstring& redis_ip = config->redis_ip;
	uint16_t redis_port = config->redis_port;
	fs::path& rootDir = config->rootDir;
	qstring& zk_uri = config->zk_uri;
	if (config->command_server) {
		PTHREAD_NAME("http3_command_server");
		http3_command_server* new_server = DEBUG_NEW http3_command_server(redis_ip.c_str(), redis_port, config->ref, config->router_port);
		new_server->run(host.c_str(), port.c_str(), rootDir, config->router, config->command_feedback_port, config->router_port_return);
		GX_DELETE(new_server);
	} else {
		PTHREAD_NAME("http3_sample_server");
		http3_sample_server* new_server = DEBUG_NEW http3_sample_server(mongodb_uri.c_str(), redis_ip.c_str(), redis_port, zk_uri);
		new_server->run(host.c_str(), port.c_str(), rootDir, config->router, config->command_feedback_port, config->router_port_return);
		GX_DELETE(new_server);
	}
	GX_DELETE(config);
	pthread_exit(0);
}

int qh3simple_router::next_available_port(const qstring& host, port_range& range, int& index) {
	int min = range.min;
	int max = range.max;
	int start_index = index;
	for (int port = min + start_index; port < max; port++) {
		index++;
		if (is_port_available(host, port) == port) {
			return port;
		}
	}
	return 0;
}

int qh3simple_router::is_port_available(const qstring& host, int port_number) {
	qstring port = qstring::format_string("%d", port_number);
	const struct addrinfo hints = {.ai_family = PF_UNSPEC, .ai_socktype = SOCK_DGRAM, .ai_protocol = IPPROTO_UDP};
	struct addrinfo* local;
	if (getaddrinfo(host.c_str(), port.c_str(), &hints, &local) != 0) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "failed to resolve host. port:%d", port_number);
		return -1;
	}

	int sock = socket(local->ai_family, SOCK_DGRAM, 0);
	if (sock < 0) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create socket. port:%d", port_number);
		freeaddrinfo(local);
		return -1;
	}

	if (fcntl(sock, F_SETFL, O_NONBLOCK) != 0) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "failed to make socket non-blocking. port:%d", port_number);
		freeaddrinfo(local);
		close(sock);
		return -1;
	}

	if (bind(sock, local->ai_addr, local->ai_addrlen) < 0) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "failed to connect socket. port:%d", port_number);
		freeaddrinfo(local);
		close(sock);
		return -1;
	}

	freeaddrinfo(local);
	close(sock);
	return port_number;
}

void qh3simple_router::cmd_feedback_from_client(struct sockaddr* client_addr, const qstring& cmd) {
	char host[INET6_ADDRSTRLEN];
	char port[10];
	int ret = getnameinfo(client_addr, sizeof(struct sockaddr), host, sizeof(host), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
	if (ret != 0) {
		DEBUG_PRINT_ERROR(__LOGTAG__,
						  "getnameinfo() failed in cmd_feedback_from_client on "
						  "command '%s', returning !!!",
						  cmd.c_str());
		return;
	}

	// find the route
	route* found = nullptr;
	for (auto r : routes) {
		if (r->host == host && r->port == port) {
			found = r;
			break;
		}
	}

	// check if its in unresponsive list
	if (found == nullptr) {
		for (auto r : unresponsive_routes) {
			if (r->host == host && r->port == port) {
				found = r;
				break;
			}
		}
		if (found) {
			remove_from_unresponsive_routes(found);
			push_to_routes(found);
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
						[](conn_io_req_res* response, void* client_specific_data, void* arg, bool success) {
							UNUSED(response);
							UNUSED(client_specific_data);
							UNUSED(arg);
							UNUSED(success);
							DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "shutdown-return");
						},
						1);
				}
			}
		} else if (cmd.compare(qstring::format_string("hb-%s", port)) == 0) {  // HB
			found->refresh_hb_timestamp(mainloop);
			DEBUG_PRINT(LOG_LEVEL_5, __LOGTAG__, "HB received route %s:%s", found->host.c_str(), found->port.c_str());
		}
	} else {
		// check if its command server or not
		if (command_route->host == host && command_route->port == port) {
			if (cmd.compare(qstring::format_string("shut-ack-%s", port)) == 0) {  // shut downed
				assert(routes.size() == 0);										  // command center must be destroyed last.
				DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "Removed command-route %s:%s from router", host, port);
				GX_DELETE(command_route);
				ev_break(mainloop, EVBREAK_ONE);
			} else if (cmd.compare(qstring::format_string("hb-%s", port)) == 0) {  // HB
				command_route->refresh_hb_timestamp(mainloop);
				DEBUG_PRINT(LOG_LEVEL_5, __LOGTAG__, "HB received cmd route %s:%s", host, port);
			}
		} else {
			DEBUG_PRINT_ERROR(__LOGTAG__, "route not found %s:%s in the list !!!", host, port);
		}
	}
	//
}

qtimer* qh3simple_router::check_and_remove_unresponsive_routes(qtimer_sceduler& scheduler) {
	int timer_unresponsive_route_check_in_sec = zkconfig->get_int32("router/timer_unresponsive_route_check_in_sec", TIMER_UNRESPONSIVE_ROUTE_CHECK_IN_SECONDS);
	DEBUG_PRINT_IMPORTANT(__LOGTAG__, "check_and_remove_unresponsive_routes timer %d", timer_unresponsive_route_check_in_sec);
	qtimer* timer = scheduler.schedule_repeat_timer(
		[this](qtimer& timer) {
			int new_timer_val = zkconfig->get_int32("router/timer_unresponsive_route_check_in_sec", TIMER_UNRESPONSIVE_ROUTE_CHECK_IN_SECONDS);
			float diff = new_timer_val - timer.delay;
			if (GX_ABS(diff) > 1.0f) {
				DEBUG_PRINT_IMPORTANT(__LOGTAG__, "check_and_remove_unresponsive_routes timer updated from %5.2f to %d", timer.delay, new_timer_val);
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
				DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "Expired active route %s:%s from router. elapsed:%f timeout_unresponsive_routes_in_sec:%f", r->host.c_str(), r->port.c_str(), elapsed, timeout_unresponsive_routes_in_sec);
				expired.push_back(r);
			}

			for (route* r : expired) {
				if (remove_from_active_routes(r)) {
					push_to_unresponsive_routes(r);
				} else {
					DEBUG_WARN(LOG_LEVEL_0, __LOGTAG__, "Not found in active route list !!!");
				}
			}
			expired.clear();

			// check and remove unresponsive routes
			for (auto r : unresponsive_routes) {
				ev_tstamp elapsed = now - r->last_hb_received_time;
				if (elapsed < (timeout_unresponsive_routes_in_sec << 1)) {
					continue;
				}
				DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "Expired unresponsive route %s:%s from router. elapsed:%f timeout_unresponsive_routes_in_sec:%f", r->host.c_str(), r->port.c_str(), elapsed, (timeout_unresponsive_routes_in_sec << 1));
				expired.push_back(r);
			}

			for (route* r : expired) {
				if (remove_from_unresponsive_routes(r)) {
					GX_DELETE(r);
				} else {
					DEBUG_WARN(LOG_LEVEL_0, __LOGTAG__, "Not found in inactive route list !!!");
				}
			}
			//

			// check for cmd center
			if (command_route) {
				ev_tstamp elapsed = now - command_route->last_hb_received_time;
				DEBUG_WARN_COND(__LOGTAG__, (elapsed > (timeout_unresponsive_routes_in_sec << 1)), "Unresponsive command center !!!");
			}
		},
		timer_unresponsive_route_check_in_sec);
	return timer;
}

route* qh3simple_router::remove_from_active_routes(route* r) {
	size_t oldSz = routes.size();
	routes.erase(std::remove(routes.begin(), routes.end(), r), routes.end());
	if (oldSz != routes.size()) {
		DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "Removed route %s:%s from active router list", r->host.c_str(), r->port.c_str());
		const qstring& hash_key = qstring::format_string("servers:%s", gsdk::server::machine_public_ip);
		hiredis->delete_hash_field(hash_key, qstring::format_string("server-%s", r->port.c_str()));
		return r;
	}
	return nullptr;
}

route* qh3simple_router::remove_from_unresponsive_routes(route* r) {
	size_t oldSz = unresponsive_routes.size();
	unresponsive_routes.erase(std::remove(unresponsive_routes.begin(), unresponsive_routes.end(), r), unresponsive_routes.end());
	if (oldSz != unresponsive_routes.size()) {
		DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "Removed route %s:%s from router", r->host.c_str(), r->port.c_str());
		const qstring& hash_key = qstring::format_string("servers:%s", gsdk::server::machine_public_ip);
		hiredis->delete_hash_field(hash_key, qstring::format_string("server-%s", r->port.c_str()));
		return r;
	}
	return nullptr;
}

void qh3simple_router::push_to_unresponsive_routes(route* r) {
	if (r == nullptr || std::find(unresponsive_routes.begin(), unresponsive_routes.end(), r) != unresponsive_routes.end()) {
		return;
	}
	unresponsive_routes.push_back(r);
}

void qh3simple_router::push_to_routes(route* r) {
	if (r == nullptr || std::find(routes.begin(), routes.end(), r) != routes.end()) {
		return;
	}
	routes.push_back(r);
}
