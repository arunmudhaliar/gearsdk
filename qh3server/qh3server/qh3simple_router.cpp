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
	GX_DELETE(command_feedback_route);
	GX_DELETE(command_route);
	GX_DELETE(hiredis);
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

	// spawn initial servers
	int spawned_servers = 0;  // never decrement
	int overflow = 0;
	int index = 0;
	pid_t parent_process_id = getpid();
	pid_t child_process_id = -1;
	while (spawned_servers < 5) {
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

		DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "Star router !!!");

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
			GX_DELETE(hiredis);
			return -1;
		}

		hiredis->set_hash_value(qstring::format_string("servers:%s", gsdk::server::machine_public_ip), "router", qstring::format_string("%s:%s", config.host.c_str(), config.port.c_str()));
		hiredis->set_hash_value(qstring::format_string("servers:%s", gsdk::server::machine_public_ip), "router-return", qstring::format_string("%s:%d", config.host.c_str(), config.router_port_return));

		mainloop = ev_default_loop(0);

		ev_io watcher;
		ev_io_init(&watcher, recv_cb, sock, EV_READ);
		ev_io_start(mainloop, &watcher);
		watcher.data = this;

		ev_io watcher_return;
		ev_io_init(&watcher_return, recv_return_cb, sock_return, EV_READ);
		ev_io_start(mainloop, &watcher_return);
		watcher_return.data = this;

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
			GX_DELETE(hiredis);
			return -1;
		}
		//

		ev_loop(mainloop, 0);

		ev_io_stop(mainloop, &watcher_return);
		ev_io_stop(mainloop, &watcher);
		ev_loop_destroy(mainloop);

		freeaddrinfo(router);
		router = nullptr;
		freeaddrinfo(router_return);
		router_return = nullptr;
		close(sock_return);
		close(sock);
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
		EV_STOP_RECORD(router_server_port_deserialise_time, __LOGTAG__, "router_server_port_deserialise_time t:%lu ms", 10);
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

	GX_DELETE(command_route);
	command_route = DEBUG_NEW route(host, port, server_counter++);
	DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "spawned qh3 command server: %s:%s id-%d", host.c_str(), port.c_str(), command_route->server_id);
	command_route->create_bridge(mainloop, command_route, nullptr);
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

	// if found delete
	if (found) {
		if (cmd.compare(qstring::format_string("shut-ack-%s", port)) == 0) {  // shut downed
			size_t oldSz = routes.size();
			routes.erase(std::remove(routes.begin(), routes.end(), found), routes.end());
			if (oldSz != routes.size()) {
				DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "Removed route %s:%s from router", found->host.c_str(), found->port.c_str());
				GX_DELETE(found);

				// if no routes then shut down command center
				if (routes.size() == 0) {
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
		}
	} else {
		// check if its command server or not
		if (command_route->host == host && command_route->port == port) {
			if (cmd.compare(qstring::format_string("shut-ack-%s", port)) == 0) {  // shut downed
				assert(routes.size() == 0);										  // command center must be destroyed last.
				DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "Removed command-route %s:%s from router", command_route->host.c_str(), command_route->port.c_str());
				GX_DELETE(command_route);
				ev_break(mainloop, EVBREAK_ONE);
			}
		} else {
			DEBUG_PRINT_ERROR(__LOGTAG__, "route not found %s:%s in the list !!!", host, port);
		}
	}
	//
}
