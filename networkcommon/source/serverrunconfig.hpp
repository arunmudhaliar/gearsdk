//
//  serverrunconfig.hpp
//  networkcommon
//
//  Created by Arun A on 10/02/25.
//

#ifndef serverrunconfig_hpp
#define serverrunconfig_hpp

#include "essentials.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "serverrunconfig"

struct server_config_in {
	server_config_in() {}

	server_config_in(const qstring& host, const qstring& port, const qstring& mongodb_uri, const qstring& redis_ip, uint16_t redis_port_, const fs::path& root_dir, const fs::path& inf_file, struct addrinfo* router, uint16_t command_port,
					 const qstring& router_port, const qstring& zk_uri, uint16_t router_port_return, const qstring& app_id, void* user_arg = nullptr)
		: host(host),
		  port(port),
		  mongodb_uri(mongodb_uri),
		  redis_ip(redis_ip),
		  redis_port(redis_port_),
		  zk_uri(zk_uri),
		  root_dir(root_dir),
		  inf_file(inf_file),
		  router(router),
		  command_port(command_port),
		  router_port(router_port),
		  router_port_return(router_port_return),
		  app_id(app_id),
		  user_arg(user_arg) {}

	// for stateful server (qnetworkserver)
	server_config_in(const qstring& host, const qstring& port, const qstring& mongodb_uri, const qstring& redis_ip, uint16_t redis_port_, const fs::path& root_dir, const fs::path& inf_file, const qstring& zk_uri, const qstring& app_id,
					 void* user_arg = nullptr)
		: host(host), port(port), mongodb_uri(mongodb_uri), redis_ip(redis_ip), redis_port(redis_port_), zk_uri(zk_uri), root_dir(root_dir), inf_file(inf_file), app_id(app_id), user_arg(user_arg) {}

	server_config_in(const server_config_in& config)
		: host(config.host),
		  port(config.port),
		  mongodb_uri(config.mongodb_uri),
		  redis_ip(config.redis_ip),
		  redis_port(config.redis_port),
		  zk_uri(config.zk_uri),
		  root_dir(config.root_dir),
		  inf_file(config.inf_file),
		  run_thread_id(config.run_thread_id),
		  command_server(config.command_server),
		  command_port(config.command_port),
		  command_feedback_port(config.command_feedback_port),
		  router_port(config.router_port),
		  router_port_return(config.router_port_return),
		  app_id(config.app_id),
		  user_arg(config.user_arg) {}

	// Copy Assignment Operator
	server_config_in& operator=(const server_config_in& config) {
		if (this == &config)
			return *this;  // Prevent self-assignment
		host = config.host;
		port = config.port;
		mongodb_uri = config.mongodb_uri;
		redis_ip = config.redis_ip;
		redis_port = config.redis_port;
		zk_uri = config.zk_uri;
		root_dir = config.root_dir;
		inf_file = config.inf_file;
		run_thread_id = config.run_thread_id;
		command_server = config.command_server;
		command_port = config.command_port;
		command_feedback_port = config.command_feedback_port;
		router_port = config.router_port;
		router_port_return = config.router_port_return;
		app_id = config.app_id;
		user_arg = config.user_arg;
		return *this;
	}

	void print() {
		debug_print(LOG_LEVEL_0, __LOGTAG__, "server_config_in values:\n{");
		debug_raw(LOG_LEVEL_0, qstring::format_string("Host: %s", host.c_str()).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Port: %s", port.c_str()).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("MongoDB URI: %s", mongodb_uri.c_str()).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Redis IP: %s", redis_ip.c_str()).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Redis Port: %u", redis_port).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Zookeeper URI: %s", zk_uri.c_str()).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Root Directory: %s", root_dir.string().c_str()).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Inf file: %s", inf_file.string().c_str()).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Run Thread ID: %lu", (unsigned long) run_thread_id).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Router: %s", router ? "Set" : "Not Set").c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Command Server: %s", command_server ? "YES" : "NO").c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Command Port: %u", command_port).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Command Feedback Port: %u", command_feedback_port).c_str());
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
	fs::path inf_file = "./serverconfig.rel.inf";
	pthread_t run_thread_id = 0;
	struct addrinfo* router = nullptr;	// only for slaves
	bool command_server = false;
	uint16_t command_port = 4010;
	uint16_t command_feedback_port = 4011;	// this has to be re-assigned based on availabaility
											//	bridge_command_center* ref = nullptr;
	qstring router_port;
	uint16_t router_port_return = 4005;
	qstring app_id = "default";
	void* user_arg = nullptr;
};

struct st_qh3server_config_in {
	st_qh3server_config_in() {}

	st_qh3server_config_in(const qstring& host, const qstring& port, const fs::path& root_dir, const fs::path& inf_file, struct addrinfo* router, uint16_t command_port, const qstring& router_port, uint16_t router_port_return,
						   const qstring& app_id, void* user_arg = nullptr)
		: host(host), port(port), root_dir(root_dir), inf_file(inf_file), router(router), command_port(command_port), router_port(router_port), router_port_return(router_port_return), app_id(app_id), user_arg(user_arg) {}

	st_qh3server_config_in(const server_config_in& config)
		: host(config.host),
		  port(config.port),
		  root_dir(config.root_dir),
		  inf_file(config.inf_file),
		  run_thread_id(config.run_thread_id),
		  command_server(config.command_server),
		  command_port(config.command_port),
		  command_feedback_port(config.command_feedback_port),
		  router_port(config.router_port),
		  router_port_return(config.router_port_return),
		  app_id(config.app_id),
		  user_arg(config.user_arg) {}

	st_qh3server_config_in(const st_qh3server_config_in& config)
		: host(config.host),
		  port(config.port),
		  root_dir(config.root_dir),
		  inf_file(config.inf_file),
		  run_thread_id(config.run_thread_id),
		  command_server(config.command_server),
		  command_port(config.command_port),
		  command_feedback_port(config.command_feedback_port),
		  router_port(config.router_port),
		  router_port_return(config.router_port_return),
		  app_id(config.app_id),
		  user_arg(config.user_arg) {}

	// Copy Assignment Operator
	st_qh3server_config_in& operator=(const st_qh3server_config_in& config) {
		if (this == &config)
			return *this;  // Prevent self-assignment
		host = config.host;
		port = config.port;
		root_dir = config.root_dir;
		inf_file = config.inf_file;
		run_thread_id = config.run_thread_id;
		command_server = config.command_server;
		command_port = config.command_port;
		command_feedback_port = config.command_feedback_port;
		router_port = config.router_port;
		router_port_return = config.router_port_return;
		app_id = config.app_id;
		user_arg = config.user_arg;
		return *this;
	}

	void print() {
		debug_print(LOG_LEVEL_0, __LOGTAG__, "qh3server_config_in values:\n{");
		debug_raw(LOG_LEVEL_0, qstring::format_string("Host: %s", host.c_str()).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Port: %s", port.c_str()).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Root Directory: %s", root_dir.string().c_str()).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Inf file: %s", inf_file.string().c_str()).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Run Thread ID: %lu", (unsigned long) run_thread_id).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Router: %s", router ? "Set" : "Not Set").c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Command Server: %s", command_server ? "YES" : "NO").c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Command Port: %u", command_port).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Command Feedback Port: %u", command_feedback_port).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Router Port: %s", router_port.c_str()).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("Router Port Return: %u", router_port_return).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("App ID: %s\n}", app_id.c_str()).c_str());
		debug_raw(LOG_LEVEL_0, qstring::format_string("User Arg: %s", user_arg ? "Set" : "Not Set").c_str());
	}

	qstring host = "localhost";
	qstring port = "4004";
	fs::path root_dir = ".";
	fs::path inf_file = "./serverconfig.rel.inf";
	pthread_t run_thread_id = 0;
	struct addrinfo* router = nullptr;	// only for slaves
	bool command_server = false;
	uint16_t command_port = 4010;
	uint16_t command_feedback_port = 4011;	// this has to be re-assigned based on availabaility
											//    bridge_command_center* ref = nullptr;
	qstring router_port;
	uint16_t router_port_return = 4005;
	qstring app_id = "default";
	void* user_arg = nullptr;
};

struct st_services_config {
	qstring redis_ip = "127.0.0.1";
	uint16_t redis_port = 6379;
	qstring mongodb_uri = "mongodb://localhost:27017";	//"mongodb://192.168.0.230:6006"
	qstring zk_uri = "127.0.0.1:2181";
	st_services_config(const qstring& redis_ip, uint16_t redis_port) : redis_ip(redis_ip), redis_port(redis_port) {}

	st_services_config(const qstring& redis_ip, uint16_t redis_port, const qstring& mongodb_uri, const qstring& zk_uri) : redis_ip(redis_ip), redis_port(redis_port), mongodb_uri(mongodb_uri), zk_uri(zk_uri) {}

	st_services_config(const server_config_in& config) {
		redis_ip = config.redis_ip;
		redis_port = config.redis_port;
		mongodb_uri = config.mongodb_uri;
		zk_uri = config.zk_uri;
	}

	st_services_config(const st_services_config& config) {
		redis_ip = config.redis_ip;
		redis_port = config.redis_port;
		mongodb_uri = config.mongodb_uri;
		zk_uri = config.zk_uri;
	}

	// Copy Assignment Operator
	st_services_config& operator=(const st_services_config& config) {
		if (this == &config)
			return *this;  // Prevent self-assignment
		redis_ip = config.redis_ip;
		redis_port = config.redis_port;
		mongodb_uri = config.mongodb_uri;
		zk_uri = config.zk_uri;
		return *this;
	}
};
#endif /* serverrunconfig_h */
