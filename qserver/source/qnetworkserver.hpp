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
#include "../../networkcommon/source/qtextfilelogger.hpp"
#include "../../networkcommon/source/qthreadpool.hpp"
#include "../../qhiredis/source/qhiredis.hpp"
#include "../../qhiredis/source/qhiredis_async.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "qnetworkserver"

#define Q_LOCAL_CONN_ID_LEN 16
#define Q_MAX_DATAGRAM_SIZE 1350
#define MAX_TOKEN_LEN sizeof("quiche") - 1 + sizeof(struct sockaddr_storage) + MAX_CID_LEN

#define QTHREAD_POOL 1
#define QTHREAD_POOL_COUNT 2

// MARK: -
class conn_io;
struct connections {
	int sock;
	struct sockaddr* local_addr = nullptr;
	socklen_t local_addr_len;
	conn_io* h = nullptr;
	uint8_t buf[65535];
	uint8_t out[Q_MAX_DATAGRAM_SIZE];
};

// MARK: -
class bridge_qpeerconnection {
   public:
	virtual void flush_egress(struct ev_loop* loop, conn_io* qconnection) = 0;
	virtual void destroy_connection(struct ev_loop* loop, conn_io* qconnection) = 0;
	virtual void close_connection(conn_io* qconnection) = 0;
	virtual void onconnection_connect(conn_io* qconnection) = 0;
	virtual void onconnection_connected(conn_io* qconnection) = 0;
	virtual void onconnection_message(ssize_t recv_len, uint8_t* buf, conn_io* qconnection) = 0;
	virtual void onconnection_destroy(conn_io* qconnection) = 0;
	inline virtual struct ev_loop* get_mainloop() = 0;
};

// MARK: -
class conn_io {
   public:
	conn_io(bridge_qpeerconnection* bridge, uint8_t* scid, size_t scid_len, int sock);
	~conn_io();

	void sendmessage(const char* buf, size_t buflen, bool flush);
	void sendmessage(const qstring& buffer, bool flush);
	void close();

	bridge_qpeerconnection* bridge = nullptr;
	uint8_t cid[Q_LOCAL_CONN_ID_LEN];
	unsigned cid_hash_val = 0;
	ev_timer timer;
	int sock;
	Connection* conn = nullptr;
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
class qnetworkserver : protected bridge_qpeerconnection, protected interface_qhiredis_async {
   private:
	struct runserverconfig {
		qstring host;
		qstring port;
		qnetworkserver* thiz;
		int pthread_return_value;
		bool finished = false;
		int id = -1;
		fs::path root_dir;
		qstring redis_ip;
		uint16_t redis_port;
	};
	static int run_id;

   public:
	virtual ~qnetworkserver() {}
	int run(qstring host, qstring port, fs::path executable_path, const qstring& redis_ip, const uint16_t REDIS_PORT);
	void broadcast_message(const qstring& buffer, bool flush);
	void network_server_begin();
	void network_server_end();
	bool is_run();
	inline size_t get_connection_count() { return conns ? HASH_COUNT(conns->h) : 0; }

   protected:
	virtual void on_network_server_begin() = 0;
	virtual void on_network_server_init() = 0;
	virtual void on_network_server_end() = 0;
	virtual void on_heartbeat_check();
	void flush_egress(struct ev_loop* loop, conn_io* qconnection) final;
	void close_connection(conn_io* qconnection) final;
	void destroy_connection(struct ev_loop* loop, conn_io* qconnection) final;
	void onconnection_message(ssize_t recv_len, uint8_t* buf, conn_io* qconnection) override;
	void onconnection_connect(conn_io* qconnection) override;
	void onconnection_connected(conn_io* qconnection) override;
	void onconnection_destroy(conn_io* qconnection) override;
	void on_qhiredis_async_key_expired(const qstring& expired_key) override;
	inline struct ev_loop* get_mainloop() final { return mainloop; }
	void exit_services_gracefully();

	qtextfilelogger logger;
	qstring host_id;
	qstring port_id;
	qhiredis* hiredis = nullptr;
	qhiredis_async* hiredis_async = nullptr;

   private:
	static void debug_log(const uint8_t* line, void* argp);
	static void timeout_cb(EV_P_ ev_timer* w, int revents);
	void mint_token(const uint8_t* dcid, size_t dcid_len, struct sockaddr_storage* addr, socklen_t addr_len, uint8_t* token, size_t* token_len);
	bool validate_token(const uint8_t* token, size_t token_len, struct sockaddr_storage* addr, socklen_t addr_len, uint8_t* odcid, size_t* odcid_len);
	uint8_t* gen_cid(uint8_t* cid, size_t cid_len);
	conn_io* create_conn(uint8_t* scid, size_t scid_len, uint8_t* odcid, size_t odcid_len, struct sockaddr* local_addr, socklen_t local_addr_len, struct sockaddr_storage* peer_addr, socklen_t peer_addr_len);
	static void recv_cb(EV_P_ ev_io* w, int revents);
	void recv_cb_internal(EV_P_ ev_io* w, int revents);
	void force_disconnect_all();
	void heartbeat_check();
	static void heart_beat_check_cb(EV_P_ ev_timer* w, int revents);
	static void threadpool_mainthread_dispatcher_cb(EV_P_ ev_timer* w, int revents);
	static void* run_internal(void* data);

	Config* config = nullptr;
	struct ev_loop* mainloop = nullptr;
	struct connections* conns = nullptr;
	ev_timer heartbeat_check_timer;
	struct runserverconfig run_server_config;
	qmutex run_mutex;
	qmutex runconfig_mutex;
	pthread_t run_thread_id;

#if QTHREAD_POOL
	ev_timer threadpool_mainthread_dispatcher_timer;

   protected:
	qthreadpool<std::function<void()>> threadpool {QTHREAD_POOL_COUNT};
#endif
};

#endif /* qnetworkserver_hpp */
