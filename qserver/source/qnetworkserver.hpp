//
//  Copyright 2024 homenet25
//  qnetworkserver.hpp
//  NetworkServer
//
//  Created by Arun A on 12/10/23.
//

#ifndef qnetworkserver_hpp
#define qnetworkserver_hpp

extern "C" {
#include <ev.h>
#include <quiche.h>
#include <uthash.h>
}

#include "../../common/sdktypes.hpp"
#include "../../networkcommon/source/essentials.hpp"
#include "../../networkcommon/source/qcustomlogger.hpp"
#include "../../networkcommon/source/qstatslogger.hpp"
#include "../../networkcommon/source/qthreadpool.hpp"
#include "../../qhiredis/source/qhiredis.hpp"
#include "../../qhiredis/source/qhiredis_async.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "qnetworkserver"

#define Q_LOCAL_CONN_ID_LEN 16
#define Q_MAX_DATAGRAM_SIZE 1350
#define MAX_TOKEN_LEN sizeof("quiche") - 1 + sizeof(struct sockaddr_storage) + QUICHE_MAX_CONN_ID_LEN

#define QTHREADPOOL 0
#define QTHREADPOOL_THREAD_COUNT 4

#define Q_INFO(tag, ...) LOG_FILE(qnetworkserver::get_file_logger(), qcustomlogger::INFO_LOG, tag, __VA_ARGS__)
#define Q_DEBUG(tag, ...) LOG_FILE(qnetworkserver::get_file_logger(), qcustomlogger::DEBUG_LOG, tag, __VA_ARGS__)
#define Q_WARN(tag, ...) LOG_FILE(qnetworkserver::get_file_logger(), qcustomlogger::WARN_LOG, tag, __VA_ARGS__)
#define Q_ERROR(tag, ...) LOG_FILE(qnetworkserver::get_file_logger(), qcustomlogger::ERROR_LOG, tag, __VA_ARGS__)

#define Q_INFO_WITH_PID(pid, tag, ...) LOG_FILE_WITH_PID(qnetworkserver::get_file_logger(), qcustomlogger::INFO_LOG, tag, pid, __VA_ARGS__)
#define Q_DEBUG_WITH_PID(pid, tag, ...) LOG_FILE_WITH_PID(qnetworkserver::get_file_logger(), qcustomlogger::DEBUG_LOG, tag, pid, __VA_ARGS__)
#define Q_WARN_WITH_PID(pid, tag, ...) LOG_FILE_WITH_PID(qnetworkserver::get_file_logger(), qcustomlogger::WARN_LOG, tag, pid, __VA_ARGS__)
#define Q_ERROR_WITH_PID(pid, tag, ...) LOG_FILE_WITH_PID(qnetworkserver::get_file_logger(), qcustomlogger::ERROR_LOG, tag, pid, __VA_ARGS__)

#define Q_INFO_WITH_ROOID(pid, roomid, tag, ...) LOG_FILE_WITH_ROOMID(qnetworkserver::get_file_logger(), qcustomlogger::INFO_LOG, tag, pid, roomid, __VA_ARGS__)
#define Q_DEBUG_WITH_ROOID(pid, roomid, tag, ...) LOG_FILE_WITH_ROOMID(qnetworkserver::get_file_logger(), qcustomlogger::DEBUG_LOG, tag, pid, roomid, __VA_ARGS__)
#define Q_WARN_WITH_ROOID(pid, roomid, tag, ...) LOG_FILE_WITH_ROOMID(qnetworkserver::get_file_logger(), qcustomlogger::WARN_LOG, tag, pid, roomid, __VA_ARGS__)
#define Q_ERROR_WITH_ROOID(pid, roomid, tag, ...) LOG_FILE_WITH_ROOMID(qnetworkserver::get_file_logger(), qcustomlogger::ERROR_LOG, tag, pid, roomid, __VA_ARGS__)

class qnetworkserver;
// MARK: -
class observer_qserver_events {
   public:
	virtual ~observer_qserver_events() { debug_print(LOG_LEVEL_0, __LOGTAG__, "observer_qserver_events destroyed"); };
	virtual void on_server_pre_start(qnetworkserver*) = 0;
	virtual void on_server_start(qnetworkserver*, const char* ip, uint16_t port) = 0;
	virtual void on_server_stop(qnetworkserver*) = 0;
	virtual void on_server_error(qnetworkserver*, int error_code) = 0;

	// room events
	virtual void room_event_create(void* server, int room) = 0;
	virtual void room_event_start(void* server, int room) = 0;
	virtual void room_event_player_added(void* server, int room, const qstring& pid, unsigned cid_hash) = 0;
	virtual void room_event_message(void* server, int room, const qstring& pid, unsigned cid_hash, const qstring& msg) = 0;
	virtual void room_event_player_removed(void* server, int room, const qstring& pid, unsigned cid_hash) = 0;
	virtual void room_event_end(void* server, int room) = 0;
	virtual void room_event_countdown_to_start(void* server, int room, int count, int max_count) = 0;
	virtual void room_event_countdown_cancelled(void* server, int room) = 0;
};

// MARK: -
class qconn_io;
struct qconnections {
	int sock;
	struct sockaddr* local_addr = nullptr;
	socklen_t local_addr_len;
	qconn_io* h = nullptr;
	uint8_t buf[65535];
	uint8_t out[Q_MAX_DATAGRAM_SIZE];
};

// MARK: -
class bridge_qpeerconnection {
   public:
	virtual void flush_egress(struct ev_loop* loop, qconn_io* qconnection) = 0;
	virtual void destroy_connection(struct ev_loop* loop, qconn_io* qconnection) = 0;
	virtual void close_connection(qconn_io* qconnection) = 0;
	virtual void onconnection_connect(qconn_io* qconnection) = 0;
	virtual void onconnection_connected(qconn_io* qconnection) = 0;
	virtual void onconnection_message(ssize_t recv_len, uint8_t* buf, qconn_io* qconnection) = 0;
	virtual void onconnection_destroy(qconn_io* qconnection) = 0;
	inline virtual struct ev_loop* get_mainloop() = 0;
	inline virtual observer_qserver_events* get_observer() = 0;
	virtual bool is_log_quiche() = 0;
};

// MARK: -
class qconn_io {
   public:
	qconn_io(bridge_qpeerconnection* bridge, uint8_t* scid, size_t scid_len, int sock);
	~qconn_io();

	void sendmessage(const char* buf, size_t buflen, bool flush);
	void sendmessage(const qstring& buffer, bool flush);
	void close();

	bridge_qpeerconnection* bridge = nullptr;
	uint8_t cid[Q_LOCAL_CONN_ID_LEN];
	unsigned cid_hash_val = 0;
	ev_timer timer;
	int sock;
	quiche_conn* conn = nullptr;
	struct sockaddr_storage peer_addr;
	socklen_t peer_addr_len;
	UT_hash_handle hh;
	int itrmsg = 0;
	bool connection_established = false;
	uint8_t egress_out[Q_MAX_DATAGRAM_SIZE];
	uint64_t last_stream_s = 0;
	int user_data = 0;
	ev_tstamp last_heartbeat_time;
};

// MARK: -
class qnetworkserver : protected bridge_qpeerconnection {
   public:
	struct runserverconfig {
		qstring host;
		qstring port;
		int pthread_return_value;
		int id = -1;
		fs::path root_dir;
		qstring redis_ip;
		uint16_t redis_port;
		qstring app_id;
		observer_qserver_events* observer = nullptr;
		pthread_t run_thread_id;
		qstring zk_uri;
	};
	qnetworkserver() {};
	virtual ~qnetworkserver() {}
	int run(qstring host, qstring port, fs::path executable_path, const qstring& redis_ip, const uint16_t REDIS_PORT, const qstring& app_id, observer_qserver_events* observer = nullptr);
	void broadcast_message(const qstring& buffer, bool flush);
	bool network_server_begin();
	void network_server_end();
	inline size_t get_connection_count() { return conns ? HASH_COUNT(conns->h) : 0; }
	inline bool is_log_quiche() override { return false; }
    qcustomlogger* get_file_logger() { return &logger; }
    qstatslogger* get_stats_loggeer() { return &stats_logger; }
    
   protected:
	virtual bool on_network_server_begin() = 0;
	virtual void on_network_server_init() = 0;
	virtual void on_network_server_end() = 0;
	virtual void on_heartbeat_check();
	void flush_egress(struct ev_loop* loop, qconn_io* qconnection) final;
	void close_connection(qconn_io* qconnection) final;
	void destroy_connection(struct ev_loop* loop, qconn_io* qconnection) final;
	void onconnection_message(ssize_t recv_len, uint8_t* buf, qconn_io* qconnection) override;
	void onconnection_connect(qconn_io* qconnection) override;
	void onconnection_connected(qconn_io* qconnection) override;
	void onconnection_destroy(qconn_io* qconnection) override;

	inline struct ev_loop* get_mainloop() final { return mainloop; }
	inline observer_qserver_events* get_observer() final { return server_event_observer; }
	void exit_services_gracefully();
	const struct runserverconfig& get_run_server_config() { return run_server_config; }
    
	static int run_id;
	qcustomlogger logger;
	qstatslogger stats_logger;

	qstring host_id;
	qstring port_id;
	observer_qserver_events* server_event_observer = nullptr;

   private:
	static void debug_quiche_log(const char* line, void* argp);
	static void timeout_cb(EV_P_ ev_timer* w, int revents);
	void mint_token(const uint8_t* dcid, size_t dcid_len, struct sockaddr_storage* addr, socklen_t addr_len, uint8_t* token, size_t* token_len);
	bool validate_token(const uint8_t* token, size_t token_len, struct sockaddr_storage* addr, socklen_t addr_len, uint8_t* odcid, size_t* odcid_len);
	uint8_t* gen_cid(uint8_t* cid, size_t cid_len);
	qconn_io* create_conn(uint8_t* scid, size_t scid_len, uint8_t* odcid, size_t odcid_len, struct sockaddr* local_addr, socklen_t local_addr_len, struct sockaddr_storage* peer_addr, socklen_t peer_addr_len);
	static void recv_cb(EV_P_ ev_io* w, int revents);
	void recv_cb_internal(EV_P_ ev_io* w, int revents);
	void force_disconnect_all();
	void heartbeat_check();
	static void heart_beat_check_cb(EV_P_ ev_timer* w, int revents);
	static void threadpool_mainthread_dispatcher_cb(EV_P_ ev_timer* w, int revents);

	quiche_config* config = nullptr;
	struct ev_loop* mainloop = nullptr;
	struct qconnections* conns = nullptr;
	ev_timer heartbeat_check_timer;
	struct runserverconfig run_server_config;

#if QTHREADPOOL
	ev_timer threadpool_mainthread_dispatcher_timer;

   protected:
	struct thread_pool_context {
		qhiredis* hiredis = nullptr;
	};
	static bool init_threadpool_context(thread_pool_context& context, const void*);
	static bool cleanup_threadpool_context(thread_pool_context& context);
	qthreadpool<thread_pool_context> threadpool {QTHREADPOOL_THREAD_COUNT, init_threadpool_context, cleanup_threadpool_context};
#endif
};

#endif /* qnetworkserver_hpp */
