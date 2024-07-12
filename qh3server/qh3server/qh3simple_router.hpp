//
//  Copyright 2024 homenet25
//  qh3simple_router.hpp
//  qh3server
//
//  Created by Arun A on 21/12/23.
//

#ifndef qh3simple_router_hpp
#define qh3simple_router_hpp

#include "../../networkcommon/source/serverconfig.hpp"
#include "../../qhiredis/source/qhiredis.hpp"
#include "../../qhiredis/source/qhiredis_async.hpp"
#include "../../qzookeeper/source/qzookeeper.hpp"
#include "qh3simple_router_structs.h"

#include <qh3client_helper.hpp>

#define MAX_ROUTES 16

#undef __LOGTAG__
#define __LOGTAG__ "qh3simple_router"

#define TIMEOUT_UNRESPONSIVE_ROUTE_IN_SECONDS 10
#define TIMER_UNRESPONSIVE_ROUTE_CHECK_IN_SECONDS 15
#define EXPIRE_TIMER_UNRESPONSIVE_ROUTE_ZK_CHECK_IN_SECONDS 20
#define NO_OF_SERVERS_TO_SPAWN 5

class qh3simple_router : public bridge_command_center, protected interface_qhiredis_async {
   public:
	qh3simple_router(const server_config_in& config);
	virtual ~qh3simple_router();
	template <typename U, typename V>
	int run();

	const std::vector<route*>& get_routes() override final { return routes; }
	void cmd_feedback_from_client(struct sockaddr* client_addr, const qstring& cmd) override final;

   protected:
	void on_qhiredis_async_key_expired(const qstring& expired_key) override;

   private:
	static void recv_cb(EV_P_ ev_io* w, int revents);
	static void recv_return_cb(EV_P_ ev_io* w, int revents);
	struct ev_loop* mainloop = nullptr;
	int sock = -1;
	int sock_return = -1;

	// qh3
	template <typename U, typename V>
	route* spawn_qh3server_command_server(const qstring& host, const qstring& port, const server_config_in& config);
	template <typename U, typename V>
	int spawn_qh3server(const qstring& host, const qstring& port, const server_config_in& config, pid_t& child_process_id, bool& fork_result);
	template <typename U, typename V>
	static void* spawn_qh3server_internal(void* data);

	static int next_available_port(const qstring& host, port_range& range, int& index);	 // index starts with 0
	static int is_port_available(const qstring& host, int port_number);

	bool is_route_available(const route* r);
	route* is_in_active_routes(const qstring& host, const qstring& port) const;
	route* is_in_unresponsive_routes(const qstring& host, const qstring& port) const;
	qtimer* check_and_remove_unresponsive_routes(qtimer_sceduler& scheduler);
	route* remove_from_active_routes(route* r);
	route* remove_from_unresponsive_routes(route* r);
	void push_to_unresponsive_routes(route* r);
	void push_to_routes(route* r);
	void shutdown_zk();

	route* command_feedback_route = nullptr;
	route* command_route = nullptr;
	std::vector<route*> routes;
	std::vector<route*> unresponsive_routes;
#if FORK_QH3_SERVER
	std::vector<pid_t> server_process_ids;
#endif
	int server_counter = 0;
	port_range range;
	server_config_in config;  // default config
	struct addrinfo* router = nullptr;
	struct addrinfo* router_return = nullptr;
	qhiredis* hiredis = nullptr;
	qhiredis_async* hiredis_async = nullptr;
	qzookeeper* qzk = nullptr;
	serverconfig* zkconfig = nullptr;
};

#include "qh3simple_router_extended.cpp"

class http3_sample_router : public qh3simple_router {
   public:
	http3_sample_router(const server_config_in& config) : qh3simple_router(config) {}
	~http3_sample_router() {}
};
#endif /* qh3simple_router_hpp */
