//
//  Copyright 2024 homenet25
//  qh3simple_router_extended.cpp
//  qh3server
//
//  Created by Arun A on 25/06/24.
//

#ifndef QH3SIMPLE_ROUTER_EXTENDED_CPP
#define QH3SIMPLE_ROUTER_EXTENDED_CPP

#include "qh3simple_router.hpp"

using namespace client;
template <typename U, typename V>
int qh3simple_router::run() {
	PTHREAD_NAME(qstring::format_string("router-%s", config.port.c_str()).c_str());

#if FORK_QH3_SERVER
	debug_print_important2(__LOGTAG__, "Forking enabled !!!");
#else
	debug_print_important2(__LOGTAG__, "Forking disabled !!!");
#endif
	// router object needs to be inited first, since this info is needed by child
	// qh3server child process
	const struct addrinfo HINTS = {.ai_family = PF_UNSPEC, .ai_socktype = SOCK_DGRAM, .ai_protocol = IPPROTO_UDP};
	freeaddrinfo(router);
	if (getaddrinfo(config.host.c_str(), config.port.c_str(), &HINTS, &router) != 0) {
		debug_print_error(__LOGTAG__, "failed to resolve host");
		return -1;
	}
	// return
	freeaddrinfo(router_return);
	if (getaddrinfo(config.host.c_str(), qstring::format_string("%d", config.router_port_return).c_str(), &HINTS, &router_return) != 0) {
		debug_print_error(__LOGTAG__, "failed to resolve host (port_return)");
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
		debug_print_error(__LOGTAG__, "failed to create socket");
		freeaddrinfo(router);
		freeaddrinfo(router_return);
		router_return = nullptr;
		router = nullptr;
		return -1;
	}

	if (fcntl(sock, F_SETFL, O_NONBLOCK) != 0) {
		debug_print_error(__LOGTAG__, "failed to make socket non-blocking");
		freeaddrinfo(router);
		freeaddrinfo(router_return);
		router_return = nullptr;
		router = nullptr;
		close(sock);
		return -1;
	}

	if (bind(sock, router->ai_addr, router->ai_addrlen) < 0) {
		debug_print_error(__LOGTAG__, "failed to connect socket");
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
		debug_print_error(__LOGTAG__, "failed to create sock_return");
		freeaddrinfo(router);
		freeaddrinfo(router_return);
		router_return = nullptr;
		router = nullptr;
		close(sock);
		return -1;
	}

	if (fcntl(sock_return, F_SETFL, O_NONBLOCK) != 0) {
		debug_print_error(__LOGTAG__, "failed to make sock_return non-blocking");
		freeaddrinfo(router);
		freeaddrinfo(router_return);
		router_return = nullptr;
		router = nullptr;
		close(sock_return);
		close(sock);
		return -1;
	}

	if (bind(sock_return, router_return->ai_addr, router_return->ai_addrlen) < 0) {
		debug_print_error(__LOGTAG__, "failed to connect sock_return");
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

	debug_print_important2(__LOGTAG__, "Creating  command center !!!");
	// command server
	if (spawn_qh3server_command_server<U, V>(config.host, config.command_port, config) == nullptr) {
		debug_print_error(__LOGTAG__, "failed to create command server !!!");
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
			debug_print_error(__LOGTAG__, "OVERFLOW ON SPAWNING SERVERS !!!");
			debug_print_error(__LOGTAG__, "OVERFLOW ON SPAWNING SERVERS !!!");
			debug_print_error(__LOGTAG__, "OVERFLOW ON SPAWNING SERVERS !!!");
			break;
		}
		int free_port = next_available_port(config.host, range, index);
		if (free_port == 0) {
			debug_print_error(__LOGTAG__, "NO PORT AVAILABLE !!!");
			break;
		}
		bool fork_result = false;  // only valid inside FORK_QH3_SERVER preprocessor
		int result = spawn_qh3server<U, V>(config.host, qstring::format_string("%d", free_port), config, child_process_id, fork_result);
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
				debug_print_error(__LOGTAG__, "forking failed, allowing parent(pid:%d) to continue !!!", parent_process_id);
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
		debug_print_important2(__LOGTAG__, "Star router !!!");

#if ENABLE_ZK
		GX_DELETE(qzk);
		qzk = DEBUG_NEW qzookeeper(qstring::format_string("zk-router-%s", config.port.c_str()));
		int zk_result = qzk->connect(config.zk_uri);
		if (zk_result != 0) {
			freeaddrinfo(router);
			freeaddrinfo(router_return);
			router_return = nullptr;
			router = nullptr;
			close(sock_return);
			close(sock);
			debug_print_error(__LOGTAG__, "zk failed to connect !!!, Exiting.");
			GX_DELETE(qzk);
			return -1;
		}

		GX_DELETE(zkconfig);
		zkconfig = DEBUG_NEW serverconfig(qzk, nullptr);
#if PROD_BUILD
		fs::path config_path(config.root_dir / "configs/prod/runtime-config.json");
#else
		fs::path config_path(config.root_dir / "configs/dev/runtime-config.json");
#endif
		if (!zkconfig->load(config_path, qzk, "/qh3router")) {
			debug_print_error(__LOGTAG__, "zkconfig load error - %s.", config_path.c_str());
			GX_DELETE(zkconfig);
			return false;
		}
#endif

		GX_DELETE(hiredis);
		hiredis = DEBUG_NEW qhiredis("router_hiredis", config.redis_ip, config.redis_port);
		if (hiredis->connect_redis() != 0) {
			debug_print_error(__LOGTAG__, "failed to create redis connection !!!");
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

		hiredis_async = DEBUG_NEW qhiredis_async(config.redis_ip, config.redis_port, this, "CONFIG SET notify-keyspace-events KEA");
		if (hiredis_async->connect_async_redis(mainloop) != 0) {
			debug_print_error(__LOGTAG__, "failed to connect async hiredis, Exiting !!!");
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

		qtimer_scheduler unresponsive_routes_scheduler;
		unresponsive_routes_scheduler.set_loop(mainloop);
		qtimer* unresponsive_timer = check_and_remove_unresponsive_routes(unresponsive_routes_scheduler);

		qtimer_scheduler update_redis_about_servers_scheduler;
		update_redis_about_servers_scheduler.set_loop(mainloop);
		qtimer* update_redis_about_servers_timer = update_redis_about_servers(update_redis_about_servers_scheduler);

		ev_loop(mainloop, 0);

		ev_io_stop(mainloop, &watcher_return);
		ev_io_stop(mainloop, &watcher);

		update_redis_about_servers_scheduler.cancel_and_destroy_timer(update_redis_about_servers_timer);
		unresponsive_routes_scheduler.cancel_and_destroy_timer(unresponsive_timer);

		ev_loop_destroy(mainloop);

		freeaddrinfo(router);
		router = nullptr;
		freeaddrinfo(router_return);
		router_return = nullptr;
		close(sock_return);
		close(sock);
		shutdown_zk();
		debug_print_important2(__LOGTAG__, "Stop router !!!");
		return 0;
	} else {
		freeaddrinfo(router);
		router = nullptr;
		freeaddrinfo(router_return);
		router_return = nullptr;
		debug_print_important2(__LOGTAG__, "Child process exiting !!!");
		return 0;
	}
}

template <typename U, typename V>
route* qh3simple_router::spawn_qh3server_command_server(const qstring& host, const qstring& port, const server_config_in& config) {
	server_config_in* new_config =
		DEBUG_NEW server_config_in(host, port, config.mongodb_uri, config.redis_ip, config.redis_port, config.root_dir, nullptr, config.command_port, config.router_port, config.zk_uri, config.router_port_return, config.app_id);
	new_config->command_server = true;
	new_config->ref = this;

	if (pthread_create(&new_config->run_thread_id, nullptr, qh3simple_router::spawn_qh3server_internal<U, V>, (void*) new_config) < 0) {
		debug_print_error(__LOGTAG__, "spawn_qh3server_command_server - could not create thread: %s - %d", strerror(errno), errno);
		GX_DELETE(new_config);
		return nullptr;
	}

	GX_DELETE(command_feedback_route);
	command_feedback_route = DEBUG_NEW route(host, qstring::format_string("%d", new_config->command_feedback_port), -1);
	command_feedback_route->create_bridge(mainloop, (bridge_command_center*) this, U::command_feedback_recv_cb);
	command_feedback_route->refresh_hb_timestamp(mainloop);

	GX_DELETE(command_route);
	command_route = DEBUG_NEW route(host, port, server_counter++);
	debug_print_important2(__LOGTAG__, "spawned qh3 command server: %s:%s id-%d", host.c_str(), port.c_str(), command_route->server_id);
	command_route->create_bridge(mainloop, command_route, nullptr);
	command_route->refresh_hb_timestamp(mainloop);
	return command_route;
}

template <typename U, typename V>
int qh3simple_router::spawn_qh3server(const qstring& host, const qstring& port, const server_config_in& config, pid_t& child_process_id, bool& fork_result) {
#if FORK_QH3_SERVER
	debug_print(LOG_LEVEL_0, __LOGTAG__, "Parent process (PID: %d)", getpid());
	fork_result = false;
	child_process_id = fork();
	if (child_process_id < 0) {
		fork_result = false;
		debug_print_error(__LOGTAG__, "fork failed !!!");
		return -1;
	}
	if (child_process_id == 0) {
		fork_result = true;
		// Code executed by the child process
		debug_print(LOG_LEVEL_0, __LOGTAG__, "Child process (PID: %d) [%d]", getpid(), child_process_id);
		server_config_in* new_config =
			DEBUG_NEW server_config_in(host, port, config.mongodb_uri, config.redis_ip, config.redis_port, config.root_dir, router, config.command_port, config.router_port, config.zk_uri, config.router_port_return, config.app_id);
		if (pthread_create(&new_config->run_thread_id, nullptr, qh3simple_router::spawn_qh3server_internal<U, V>, (void*) new_config) < 0) {
			debug_print_error(__LOGTAG__, "spawn_qh3server - could not create thread: %s - %d", strerror(errno), errno);
			GX_DELETE(new_config);
			return -1;
		}

		// Wait for the thread to finish
		if (pthread_join(new_config->run_thread_id, nullptr) != 0) {
			debug_print_error(__LOGTAG__, "spawn_qh3server - could not join thread: %s - %d", strerror(errno), errno);
		}

		fflush(stdout);	 // Flush output
		return 0;
	} else if (child_process_id > 0) {
		fork_result = true;
		// Code executed by the parent process
		debug_print(LOG_LEVEL_0, __LOGTAG__, "Parent process after fork (PID: %d) [%d]", getpid(), child_process_id);
		route* child = DEBUG_NEW route(host, port, server_counter++);
		child->child_process_id = child_process_id;
		routes.push_back(child);
		debug_print_important2(__LOGTAG__, "spawned qh3server: %s:%s id-%d", host.c_str(), port.c_str(), child->server_id);
		child->create_bridge(mainloop, child, nullptr);
		return 0;
	}
#else
	fork_result = false;
	server_config_in* new_config =
		DEBUG_NEW server_config_in(host, port, config.mongodb_uri, config.redis_ip, config.redis_port, config.root_dir, router, config.command_port, config.router_port, config.zk_uri, config.router_port_return, config.app_id);
	if (pthread_create(&new_config->run_thread_id, nullptr, qh3simple_router::spawn_qh3server_internal<U, V>, (void*) new_config) < 0) {
		debug_print_error(__LOGTAG__, "spawn_qh3server - could not create thread: %s - %d", strerror(errno), errno);
		GX_DELETE(new_config);
		return -1;
	}
	route* child = DEBUG_NEW route(host, port, server_counter++);
	routes.push_back(child);
	debug_print_important2(__LOGTAG__, "spawned qh3server: %s:%s id-%d", host.c_str(), port.c_str(), child->server_id);
	child->create_bridge(mainloop, child, nullptr);
	return 0;
#endif
	return -1;
}

template <typename U, typename V>
void* qh3simple_router::spawn_qh3server_internal(void* data) {
	server_config_in* config = (server_config_in*) data;
	qstring& host = config->host;
	qstring& port = config->port;
	qstring& mongodb_uri = config->mongodb_uri;
	qstring& redis_ip = config->redis_ip;
	uint16_t redis_port = config->redis_port;
	fs::path& root_dir = config->root_dir;
	qstring& zk_uri = config->zk_uri;
	if (config->command_server) {
		PTHREAD_NAME(U::get_server_name());
		U* new_server = DEBUG_NEW U(redis_ip.c_str(), redis_port, config->ref, config->router_port);
		new_server->run(host.c_str(), port.c_str(), root_dir, config->router, config->command_feedback_port, config->router_port_return, config->app_id);
		GX_DELETE(new_server);
	} else {
		PTHREAD_NAME(V::get_server_name());
		V* new_server = DEBUG_NEW V(mongodb_uri.c_str(), redis_ip.c_str(), redis_port, zk_uri);
		new_server->run(host.c_str(), port.c_str(), root_dir, config->router, config->command_feedback_port, config->router_port_return, config->app_id);
		GX_DELETE(new_server);
	}
	GX_DELETE(config);
	pthread_exit(0);
}

#endif
