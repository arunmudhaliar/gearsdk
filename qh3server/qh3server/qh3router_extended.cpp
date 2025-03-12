//
//  Copyright 2024 homenet25
//  qh3router_extended.cpp
//  qh3server
//
//  Created by Arun A on 25/06/24.
//

#ifndef QH3SIMPLE_ROUTER_EXTENDED_CPP
#define QH3SIMPLE_ROUTER_EXTENDED_CPP

#include "qh3router.hpp"

#define ROUTER_EVENT_ERROR(thiz, error_code)                   \
	do {                                                       \
		if (event_observer && thiz) {                          \
			event_observer->on_router_error(thiz, error_code); \
		}                                                      \
	} while (0)

#define ROUTER_EVENT_PRE_START(thiz)                   \
	do {                                               \
		if (event_observer && thiz) {                  \
			event_observer->on_router_pre_start(thiz); \
		}                                              \
	} while (0)

#define ROUTER_EVENT_START(thiz)                   \
	do {                                           \
		if (event_observer && thiz) {              \
			event_observer->on_router_start(thiz); \
		}                                          \
	} while (0)

#define ROUTER_EVENT_STOP(thiz)                   \
	do {                                          \
		if (event_observer && thiz) {             \
			event_observer->on_router_stop(thiz); \
		}                                         \
	} while (0)

using namespace client;
using namespace gsdk::common;

template <typename U, typename V>
int qh3router::run(qh3router_run_flag run_flag, observer_router_events* event_observer) {
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
		ROUTER_EVENT_ERROR(this, -1);
		return -1;
	}
	// return
	freeaddrinfo(router_return);
	if (getaddrinfo(config.host.c_str(), qstring::format_string("%d", config.router_port_return).c_str(), &HINTS, &router_return) != 0) {
		debug_print_error(__LOGTAG__, "failed to resolve host (port_return)");
		freeaddrinfo(router);
		router = nullptr;
		ROUTER_EVENT_ERROR(this, -1);
		return -1;
	}
	//

	//================================================
	if (sock != -1) {
		close(sock);
	}
	sock = socket(router->ai_family, SOCK_DGRAM, 0);
	if (sock < 0) {
		debug_print_error(__LOGTAG__, "f:run - failed to create socket");
		freeaddrinfo(router);
		freeaddrinfo(router_return);
		router_return = nullptr;
		router = nullptr;
		ROUTER_EVENT_ERROR(this, -1);
		return -1;
	}

	if (fcntl(sock, F_SETFL, O_NONBLOCK) != 0) {
		debug_print_error(__LOGTAG__, "f:run - failed to make socket non-blocking");
		freeaddrinfo(router);
		freeaddrinfo(router_return);
		router_return = nullptr;
		router = nullptr;
		close(sock);
		ROUTER_EVENT_ERROR(this, -1);
		return -1;
	}

	if (bind(sock, router->ai_addr, router->ai_addrlen) < 0) {
		debug_print_error(__LOGTAG__, "f:run - failed to bind socket");
		freeaddrinfo(router);
		freeaddrinfo(router_return);
		router_return = nullptr;
		router = nullptr;
		close(sock);
		ROUTER_EVENT_ERROR(this, -1);
		return -1;
	}

	// return socket
	if (sock_return != -1) {
		close(sock_return);
	}
	sock_return = socket(router_return->ai_family, SOCK_DGRAM, 0);
	if (sock_return < 0) {
		debug_print_error(__LOGTAG__, "f:run - failed to create sock_return");
		freeaddrinfo(router);
		freeaddrinfo(router_return);
		router_return = nullptr;
		router = nullptr;
		close(sock);
		ROUTER_EVENT_ERROR(this, -1);
		return -1;
	}

	if (fcntl(sock_return, F_SETFL, O_NONBLOCK) != 0) {
		debug_print_error(__LOGTAG__, "f:run - failed to make sock_return non-blocking");
		freeaddrinfo(router);
		freeaddrinfo(router_return);
		router_return = nullptr;
		router = nullptr;
		close(sock_return);
		close(sock);
		ROUTER_EVENT_ERROR(this, -1);
		return -1;
	}

	if (bind(sock_return, router_return->ai_addr, router_return->ai_addrlen) < 0) {
		debug_print_error(__LOGTAG__, "f:run - failed to bind sock_return. port[%u]", config.router_port_return);
		freeaddrinfo(router);
		freeaddrinfo(router_return);
		router_return = nullptr;
		router = nullptr;
		close(sock_return);
		close(sock);
		ROUTER_EVENT_ERROR(this, -1);
		return -1;
	}
	//

	mainloop = ev_loop_new();

	debug_print_important2(__LOGTAG__, "spawning command center !!!");
	// command server
	if (spawn_qh3server_command_server<U>(config.host, config.command_port, config, this) == nullptr) {
		debug_print_error(__LOGTAG__, "failed to create command server !!!");
		freeaddrinfo(router);
		freeaddrinfo(router_return);
		router_return = nullptr;
		router = nullptr;
		close(sock_return);
		close(sock);
		shutdown_zk();
		ROUTER_EVENT_ERROR(this, -1);
		return -1;
	}

	debug_print_important2(__LOGTAG__, "pre-start");
	ROUTER_EVENT_PRE_START(this);

	// spawn initial servers
	int spawned_servers = 0;  // never decrement
	int overflow = 0;
	int index = 0;
	pid_t parent_process_id = getpid();
	pid_t child_process_id = -1;
	while ((run_flag & SKIP_DEFAULT_QH3SERVER_SPAWN) == 0 && spawned_servers < NO_OF_DEFAULT_SERVERS_TO_SPAWN) {
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
		int result = spawn_qh3server<V>(config.host, qstring::format_string("%d", free_port), config, child_process_id, fork_result, this);
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
	debug_print_important2(__LOGTAG__, "pre-start - finish:prespawn");

	if (child_process_id != 0) {
		server_info_reader* info_reader = server_info_reader::get_instance();
		if (!info_reader->load_config(config.inf_file.c_str())) {
			freeaddrinfo(router);
			freeaddrinfo(router_return);
			router_return = nullptr;
			router = nullptr;
			close(sock_return);
			close(sock);
			debug_print_error(__LOGTAG__, "info_reader failed to load config !!!, Exiting.");
			ROUTER_EVENT_ERROR(this, -1);
			return -1;
		}
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
			ROUTER_EVENT_ERROR(this, -1);
			return -1;
		}
#endif

		GX_DELETE(zkconfig);
		zkconfig = DEBUG_NEW serverconfig(qzk, nullptr);
#if PROD_BUILD
		fs::path config_path(config.root_dir / "configs/prod/runtime-config.json");
#else
		fs::path config_path(config.root_dir / "configs/dev/runtime-config.json");
#endif
		qstring router_zk_root_node = info_reader->get_value_else_default("router_zk_root_node", "/qh3router");
		if (!zkconfig->load(config_path, qzk, router_zk_root_node)) {
			debug_print_error(__LOGTAG__, "zkconfig load error - %s.", config_path.c_str());
			freeaddrinfo(router);
			freeaddrinfo(router_return);
			router_return = nullptr;
			router = nullptr;
			close(sock_return);
			close(sock);
			shutdown_zk();
			GX_DELETE(hiredis);
			GX_DELETE(zkconfig);
			ROUTER_EVENT_ERROR(this, -1);
			return -1;
		}
		debug_print_important2(__LOGTAG__, "pre-start - finish:config");

		GX_DELETE(hiredis);
		qstring router_redis_user = info_reader->get_value_else_default("router_redis_user", "gsdkuser");
		qstring router_redis_password = info_reader->get_value_else_default("router_redis_password", "Fr0gmoon123");
		hiredis = DEBUG_NEW qhiredis("router_hiredis", config.redis_ip, config.redis_port, router_redis_user, router_redis_password);
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
			ROUTER_EVENT_ERROR(this, -1);
			return -1;
		}

		debug_print_important2(__LOGTAG__, "pre-start - finish:redis");

		ev_io watcher;
		ev_io_init(&watcher, recv_cb, sock, EV_READ);
		ev_io_start(mainloop, &watcher);
		watcher.data = this;

		ev_io watcher_return;
		ev_io_init(&watcher_return, recv_return_cb, sock_return, EV_READ);
		ev_io_start(mainloop, &watcher_return);
		watcher_return.data = this;

		hiredis_async = DEBUG_NEW qhiredis_async(config.redis_ip, config.redis_port, router_redis_user, router_redis_password, this, "CONFIG SET notify-keyspace-events KEA");
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
			ROUTER_EVENT_ERROR(this, -1);
			return -1;
		}

		debug_print_important2(__LOGTAG__, "pre-start - end");

		qtimer_scheduler unresponsive_routes_scheduler;
		unresponsive_routes_scheduler.set_loop(mainloop);
		qtimer* unresponsive_timer = check_and_remove_unresponsive_routes(unresponsive_routes_scheduler);

		qtimer_scheduler update_redis_about_servers_scheduler;
		update_redis_about_servers_scheduler.set_loop(mainloop);
		qtimer* update_redis_about_servers_timer = update_redis_about_servers(update_redis_about_servers_scheduler);

		debug_print_important2(__LOGTAG__, "Star router !!!");
		ROUTER_EVENT_START(this);

		ev_loop(mainloop, 0);

		ev_io_stop(mainloop, &watcher_return);
		ev_io_stop(mainloop, &watcher);

		update_redis_about_servers_scheduler.cancel_and_destroy_timer(update_redis_about_servers_timer);
		unresponsive_routes_scheduler.cancel_and_destroy_timer(unresponsive_timer);

		// NOTE: delete hiredis_async before destroying the 'mainloop'
		GX_DELETE(hiredis_async);

		ev_loop_destroy(mainloop);

		freeaddrinfo(router);
		router = nullptr;
		freeaddrinfo(router_return);
		router_return = nullptr;
		close(sock_return);
		close(sock);
		shutdown_zk();
		debug_print_important2(__LOGTAG__, "Stop router !!!");
		ROUTER_EVENT_STOP(this);
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

template <typename U>
route* qh3router::spawn_qh3server_command_server(const qstring& host, const qstring& port, const st_qh3router_config_in& config, qh3router* router) {
	st_qh3server_config_in* new_config = DEBUG_NEW st_qh3server_config_in(host, port, config.root_dir, config.inf_file, nullptr, config.command_port, config.router_port, config.router_port_return, config.app_id);
	new_config->command_server = true;

	st_services_config* new_services_config = DEBUG_NEW st_services_config(config.redis_ip, config.redis_port, config.mongodb_uri, config.zk_uri);

	std::tuple<st_qh3server_config_in*, st_services_config*, bridge_command_center*>* tuple_in =
		DEBUG_NEW std::tuple<st_qh3server_config_in*, st_services_config*, bridge_command_center*>(new_config, new_services_config, dynamic_cast<bridge_command_center*>(router));
	if (pthread_create(&new_config->run_thread_id, nullptr, qh3router::spawn_qh3_command_server_internal<U>, (void*) tuple_in) < 0) {
		debug_print_error(__LOGTAG__, "spawn_qh3server_command_server - could not create thread: %s - %d", strerror(errno), errno);
		GX_DELETE(tuple_in);
		GX_DELETE(new_services_config);
		GX_DELETE(new_config);
		return nullptr;
	}

	GX_DELETE(router->command_feedback_route);
	router->command_feedback_route = DEBUG_NEW route(host, qstring::format_string("%d", new_config->command_feedback_port), -1);
	router->command_feedback_route->create_bridge(router->mainloop, (bridge_command_center*) router, U::command_feedback_recv_cb);
	router->command_feedback_route->refresh_hb_timestamp(router->mainloop);

	GX_DELETE(router->command_route);
	router->command_route = DEBUG_NEW route(host, port, router->server_counter++);
	debug_print_important2(__LOGTAG__, "spawned qh3 command server: %s:%s id-%d", host.c_str(), port.c_str(), router->command_route->server_id);
	router->command_route->create_bridge(router->mainloop, router->command_route, nullptr);
	router->command_route->refresh_hb_timestamp(router->mainloop);
	return router->command_route;
}

template <typename V>
int qh3router::spawn_qh3server(const qstring& host, const qstring& port, const st_qh3router_config_in& config, pid_t& child_process_id, bool& fork_result, qh3router* router, observer_qh3server_events* server_event_observer) {
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
		st_qh3server_config_in* new_config = DEBUG_NEW st_qh3server_config_in(host, port, config.root_dir, config.inf_file, router->router, config.command_port, config.router_port, config.router_port_return, config.app_id);
		st_services_config* new_services_config = DEBUG_NEW st_services_config(config.redis_ip, config.redis_port, config.mongodb_uri, config.zk_uri);
		std::tuple<st_qh3server_config_in*, st_services_config*, observer_qh3server_events*>* tuple_in =
			DEBUG_NEW std::tuple<st_qh3server_config_in*, st_services_config*, observer_qh3server_events*>(new_config, new_services_config, server_event_observer);
		if (pthread_create(&new_config->run_thread_id, nullptr, qh3router::spawn_qh3server_internal<V>, (void*) tuple_in) < 0) {
			debug_print_error(__LOGTAG__, "spawn_qh3server - could not create thread: %s - %d", strerror(errno), errno);
			GX_DELETE(tuple_in);
			GX_DELETE(new_services_config);
			GX_DELETE(new_config);
			GX_DELETE(server_event_observer);
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
		router->create_qh3server_route(host, port, child_process_id);
		return 0;
	}
#else
	UNUSED(child_process_id);
	fork_result = false;
	st_qh3server_config_in* new_config = DEBUG_NEW st_qh3server_config_in(host, port, config.root_dir, config.inf_file, router->router, config.command_port, config.router_port, config.router_port_return, config.app_id);
	st_services_config* new_services_config = DEBUG_NEW st_services_config(config.redis_ip, config.redis_port, config.mongodb_uri, config.zk_uri);
	std::tuple<st_qh3server_config_in*, st_services_config*, observer_qh3server_events*>* tuple_in =
		DEBUG_NEW std::tuple<st_qh3server_config_in*, st_services_config*, observer_qh3server_events*>(new_config, new_services_config, server_event_observer);
	if (pthread_create(&new_config->run_thread_id, nullptr, qh3router::spawn_qh3server_internal<V>, (void*) tuple_in) < 0) {
		debug_print_error(__LOGTAG__, "spawn_qh3server - could not create thread: %s - %d", strerror(errno), errno);
		GX_DELETE(tuple_in);
		GX_DELETE(new_services_config);
		GX_DELETE(new_config);
		GX_DELETE(server_event_observer);
		return -1;
	}
	router->create_qh3server_route(host, port);
	return 0;
#endif
	return -1;
}

template <typename V>
void* qh3router::spawn_qh3server_internal(void* data) {
	std::tuple<st_qh3server_config_in*, st_services_config*, observer_qh3server_events*>* tuple_in = (std::tuple<st_qh3server_config_in*, st_services_config*, observer_qh3server_events*>*) data;
	st_qh3server_config_in* config = (st_qh3server_config_in*) std::get<0>(*tuple_in);
	st_services_config* services_config = (st_services_config*) std::get<1>(*tuple_in);
	observer_qh3server_events* event_observer = (observer_qh3server_events*) std::get<2>(*tuple_in);
	if (config->command_server) {
		debug_print_error(__LOGTAG__, "spawn_qh3server_internal failed, its a command server");
		GX_DELETE(tuple_in);
		GX_DELETE(services_config);
		GX_DELETE(config);
		GX_DELETE(event_observer);
		return nullptr;
	}

	PTHREAD_NAME(V::get_server_name());
	V* new_server = DEBUG_NEW V(*config, *services_config);
	new_server->run(*config, event_observer);
	GX_DELETE(tuple_in);
	GX_DELETE(new_server);
	GX_DELETE(services_config);
	GX_DELETE(config);
	GX_DELETE(event_observer);
	return nullptr;
}

template <typename U>
void* qh3router::spawn_qh3_command_server_internal(void* data) {
	std::tuple<st_qh3server_config_in*, st_services_config*, bridge_command_center*>* tuple_in = (std::tuple<st_qh3server_config_in*, st_services_config*, bridge_command_center*>*) data;
	st_qh3server_config_in* config = (st_qh3server_config_in*) std::get<0>(*tuple_in);
	st_services_config* services_config = (st_services_config*) std::get<1>(*tuple_in);
	bridge_command_center* bridge_ref = (bridge_command_center*) std::get<2>(*tuple_in);
	if (!config->command_server) {
		debug_print_error(__LOGTAG__, "spawn_qh3_command_server_internal failed, not a command server");
		GX_DELETE(tuple_in);
		GX_DELETE(services_config);
		GX_DELETE(config);
		return nullptr;
	}
	PTHREAD_NAME(U::get_server_name());
	U* new_server = DEBUG_NEW U(*config, *services_config, bridge_ref);
	new_server->run(*config);
	GX_DELETE(tuple_in);
	GX_DELETE(services_config);
	GX_DELETE(new_server);
	GX_DELETE(config);
	return nullptr;
}

#endif
