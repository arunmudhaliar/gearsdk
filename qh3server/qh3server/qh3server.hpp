//
//  Copyright 2024 homenet25
//  qh3server.hpp
//  qh3server
//
//  Created by Arun A on 30/10/23.
//

#ifndef qh3server_hpp
#define qh3server_hpp

extern "C" {
#include <fcntl.h>
#include <quiche.h>
#include <uthash.h>
#if USE_UV_MAIN_LOOP
#include <uv.h>
#else
#include <ev.h>
#endif
}

#include "../../common/sdktypes.hpp"
#include "../../networkcommon/source/essentials.hpp"
#include "../../networkcommon/source/qcustomlogger.hpp"
#include "../../networkcommon/source/qstatslogger.hpp"
#include "../../networkcommon/source/serverrunconfig.hpp"
#include "qh3server_structs.h"

#undef __LOGTAG__
#define __LOGTAG__ "qh3server"

#define QH3_INFO(tag, ...) LOG_FILE(qh3server::get_file_logger(), qcustomlogger::INFO_LOG, tag, __VA_ARGS__)
#define QH3_DEBUG(tag, ...) LOG_FILE(qh3server::get_file_logger(), qcustomlogger::DEBUG_LOG, tag, __VA_ARGS__)
#define QH3_WARN(tag, ...) LOG_FILE(qh3server::get_file_logger(), qcustomlogger::WARN_LOG, tag, __VA_ARGS__)
#define QH3_ERROR(tag, ...) LOG_FILE(qh3server::get_file_logger(), qcustomlogger::ERROR_LOG, tag, __VA_ARGS__)

#define QH3_INFO_WITH_PID(pid, tag, ...) LOG_FILE_WITH_PID(qh3server::get_file_logger(), qcustomlogger::INFO_LOG, tag, pid, __VA_ARGS__)
#define QH3_DEBUG_WITH_PID(pid, tag, ...) LOG_FILE_WITH_PID(qh3server::get_file_logger(), qcustomlogger::DEBUG_LOG, tag, pid, __VA_ARGS__)
#define QH3_WARN_WITH_PID(pid, tag, ...) LOG_FILE_WITH_PID(qh3server::get_file_logger(), qcustomlogger::WARN_LOG, tag, pid, __VA_ARGS__)
#define QH3_ERROR_WITH_PID(pid, tag, ...) LOG_FILE_WITH_PID(qh3server::get_file_logger(), qcustomlogger::ERROR_LOG, tag, pid, __VA_ARGS__)

// trouble shoot
// https://www.chromium.org/for-testers/providing-network-details/

// MARK: -
class qh3server : public bridge_h3_connection {
   private:
	quiche_config* config = nullptr;
	quiche_h3_config* http3_config = nullptr;
	struct connections* conns = nullptr;
	EVENT_LOOP_TYPE* mainloop = nullptr;
	static void debug_quiche_log(const char* line, void* argp);
	ssize_t flush_egress(struct conn_io_qh3* conn_io) final;
	void destroy_connection(struct conn_io_qh3* conn_io) final;
	inline bool is_log_quiche() override { return false; }
	float get_router_hb_interval_in_sec() override { return DEFAULT_TIMER_ROUTER_HB_INTERVAL_IN_SECONDS; }
	parse_return parse(struct conn_io_qh3* conn_io) override;

	void mint_token(const uint8_t* dcid, size_t dcid_len, struct sockaddr_storage* addr, socklen_t addr_len, uint8_t* token, size_t* token_len);
	bool validate_token(const uint8_t* token, size_t token_len, struct sockaddr_storage* addr, socklen_t addr_len, uint8_t* odcid, size_t* odcid_len);
	static uint8_t* gen_cid(uint8_t* cid, size_t cid_len);
	struct conn_io_qh3* create_conn(uint8_t* scid, size_t scid_len, uint8_t* odcid, size_t odcid_len, struct sockaddr* local_addr, socklen_t local_addr_len, struct sockaddr_storage* peer_addr, socklen_t peer_addr_len,
									struct sockaddr_storage* peer_original_client_addr);
	static int for_each_header(uint8_t* name, size_t name_len, uint8_t* value, size_t value_len, void* argp);
#if USE_UV_MAIN_LOOP
	static void recv_cb(uv_poll_t* handle, int status, int events);
	static void timeout_cb(uv_timer_t* handle);
#else
	static void recv_cb(EV_P_ ev_io* w, int revents);
	static void timeout_cb(EV_P_ ev_timer* w, int revents);
	static void libev_idle_cb(EV_P_ ev_idle* w, int revents);
#endif

	void send_in_chunks(struct conn_io_qh3* conn_io);
	void send_response(struct conn_io_qh3* conn_io);

	void destroy_pending_connections();
	TIMER_TYPE* router_hb_loop(TIMER_SCHEDuLER_TYPE& router_hb_scheduler, const qstring& host, const qstring& port, int sock, uint16_t command_center_feedback_port);
	TIMER_TYPE* dangling_connections_check_loop(TIMER_SCHEDuLER_TYPE& close_dangling_connections_scheduler, float interval);
	void stop_services_and_report(int sock, uint16_t command_center_feedback_port, float interval);

	uint8_t out[MAX_DATAGRAM_SIZE + ORIGINAL_CLIENT_ADDR_SZ];
	uint8_t buf[65535];
	routerinfo* relay_through_router_info = nullptr;  // only valid for servers else NULL

   protected:
	inline EVENT_LOOP_TYPE* get_mainloop() final { return mainloop; }
	virtual bool on_server_pre_init() = 0;
	virtual void on_server_uninitialise() = 0;
	virtual void on_run_started() = 0;
	virtual void on_run_end() = 0;
	void parse_header(const qstring& name, const qstring& value, struct conn_io_qh3* conn_io) override;
	void try_report_router(const qstring& cmd_string, int sock, uint16_t command_center_feedback_port);

	qcustomlogger* logger = nullptr;
	qstatslogger* stats_logger = nullptr;
	qstring logtag = __LOGTAG__;
	qstring port_id;
	qstring host_id;
	fs::path app_directory = ".";
	observer_qh3server_events* server_event_observer = nullptr;	 // used for ts callbacks
	void* user_arg = nullptr;
	struct st_qh3server_config_in run_server_config;

   public:
	qh3server();
	virtual ~qh3server();
	qcustomlogger* get_file_logger() { return logger; }
	qstatslogger* get_stats_loggeer() { return stats_logger; }
	unsigned int get_live_connection_count() { return HASH_COUNT(conns->h); }
	void try_send_response(struct conn_io_qh3* conn_io);
	struct conn_io_qh3* get_conn(uint8_t* dcid, uint16_t dcid_len);
	observer_qh3server_events* get_server_observer() { return server_event_observer; }
	void* get_user_arg() { return user_arg; }
	void shutdown();
	int run(const st_qh3server_config_in& in_config, observer_qh3server_events* event_observer = nullptr, void* user_arg = nullptr);
};

#endif /* qh3server_hpp */
