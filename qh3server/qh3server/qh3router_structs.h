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

struct server_config_in {
	server_config_in(const qstring& host, const qstring& port, const qstring& mongodb_uri, const qstring& redis_ip, uint16_t redis_port_, const fs::path& root_dir, struct addrinfo* router, uint16_t command_port, const qstring& router_port,
					 const qstring& zk_uri, uint16_t router_port_return, const qstring& app_id, void* user_arg = nullptr)
		: host(host),
		  port(port),
		  mongodb_uri(mongodb_uri),
		  redis_ip(redis_ip),
		  redis_port(redis_port_),
		  zk_uri(zk_uri),
		  root_dir(root_dir),
		  router(router),
		  command_port(command_port),
		  router_port(router_port),
		  router_port_return(router_port_return),
		  app_id(app_id),
		  user_arg(user_arg) {}

	server_config_in(const server_config_in& config)
		: host(config.host),
		  port(config.port),
		  mongodb_uri(config.mongodb_uri),
		  redis_ip(config.redis_ip),
		  redis_port(config.redis_port),
		  zk_uri(config.zk_uri),
		  root_dir(config.root_dir),
		  command_server(config.command_server),
		  command_port(config.command_port),
		  command_feedback_port(config.command_feedback_port),
		  ref(config.ref),
		  router_port(config.router_port),
		  router_port_return(config.router_port_return),
		  app_id(config.app_id),
		  user_arg(config.user_arg) {}

	void print() {
		debug_print(LOG_LEVEL_0, __LOGTAG__, "server_config_in values:\n{");
		debug_raw(LOG_LEVEL_0, qstring::format_string("Host: %s", host.c_str()).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Port: %s", port.c_str()).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("MongoDB URI: %s", mongodb_uri.c_str()).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Redis IP: %s", redis_ip.c_str()).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Redis Port: %u", redis_port).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Zookeeper URI: %s", zk_uri.c_str()).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Root Directory: %s", root_dir.string().c_str()).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Run Thread ID: %lu", (unsigned long) run_thread_id).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Router: %s", router ? "Set" : "Not Set").c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Command Server: %s", command_server ? "YES" : "NO").c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Command Port: %u", command_port).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Command Feedback Port: %u", command_feedback_port).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Bridge Command Center Ref: %s", ref ? "Set" : "Not Set").c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Router Port: %s", router_port.c_str()).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Router Port Return: %u", router_port_return).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("App ID: %s\n}", app_id.c_str()).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("User Arg: %s", user_arg ? "Set" : "Not Set").c_str());
	}

	qstring host = "localhost";
	qstring port = "4004";
	qstring mongodb_uri = "mongodb://localhost:27017";	//"mongodb://192.168.0.230:6006"
	qstring redis_ip = "127.0.0.1";
	uint16_t redis_port = 6379;
	qstring zk_uri = "127.0.0.1:2181";
	fs::path root_dir = ".";
	pthread_t run_thread_id = 0;
	struct addrinfo* router = nullptr;	// only for slaves
	bool command_server = false;
	uint16_t command_port = 4010;
	uint16_t command_feedback_port = 4011;	// this has to be re-assigned based on availabaility
	bridge_command_center* ref = nullptr;
	qstring router_port;
	uint16_t router_port_return = 4005;
	qstring app_id = "default";
	void* user_arg = nullptr;
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
