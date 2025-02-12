//
//  Copyright 2024 homenet25
//  qnetworkclient.hpp
//  networkclient
//
//  Created by Arun A on 12/10/23.
//

#ifndef qnetworkclient_hpp
#define qnetworkclient_hpp

extern "C" {
// #include <cstdlib>	// for malloc and free
#include <cstring>	// for memcpy
#include <ev.h>
#include <fcntl.h>
#include <quiche.h>
#include <uthash.h>
#if USE_PTHREAD
#include <pthread.h>
#endif
}
#include "../../common/sdktypes.hpp"
#include "../../networkcommon/source/essentials.hpp"

//#include <deque>
#include <queue>
#include <atomic>

#undef __LOGTAG__
#define __LOGTAG__ "qnetworkclient"

#define Q_LOCAL_CONN_ID_LEN 16
#define Q_MAX_DATAGRAM_SIZE 1350

#define SENDTO_INITIAL_RETRY_INTERVAL 2
#define MAX_SENDTO_RETRY_COUNT 4

#define HEARTBEAT_INTERVAL 5.0f
#define SEND_INTERVAL 0.2f
#define CLIENT_IDLE_TIMEOUT_MS 30000

namespace client {
struct qdata {
	qdata(const uint8_t* data, ssize_t sz, bool fin = false) : size(sz), fin(fin) {
		this->data = new uint8_t[sz];
		memcpy(this->data, data, sz);
	}
	~qdata() { GX_DELETE_ARY(this->data); }
	uint8_t* data = nullptr;
	ssize_t size = 0;
	bool fin = false;
};

struct socket_data {
	socket_data(int sockfd, const void* buf, size_t len, int flags, const struct sockaddr* dest_addr, socklen_t addrlen) : sockfd(sockfd), buf(buf), len(len), flags(flags), addrlen(addrlen) {
		this->dest_addr = (struct sockaddr*) malloc(addrlen);
		memcpy(this->dest_addr, dest_addr, addrlen);
	}

	~socket_data() {
		if (dest_addr) {
			free(dest_addr);
		}
	}

	// Class members
	int sockfd;
	const void* buf;			 // Pointer to data buffer
	size_t len;					 // Length of data
	int flags;					 // Flags for sendto
	struct sockaddr* dest_addr;	 // Dynamically allocated pointer for the destination address
	socklen_t addrlen;			 // Length of the address
};

class bridge_qcommand;
class conn_io_client {
   private:
	conn_io_client() {}

   public:
	conn_io_client(bridge_qcommand* bridge, int id);
	conn_io_client(bridge_qcommand* bridge, quiche_config* config, int id);
	~conn_io_client();

	int close_socket();
	void set_config(quiche_config* config) { this->config = config; }
	int id = -1;
	ev_timer timer;
	ev_timer send_timer;
	ev_io watcher;
	int sock = -1;
#if PLATFORM == PLATFORM_WINDOWS
	int win_sock_fd = -1;
#endif
	struct sockaddr_storage local_addr;
	socklen_t local_addr_len;
	quiche_conn* conn = nullptr;
	bridge_qcommand* bridge = nullptr;
	quiche_config* config = nullptr;
	struct addrinfo* peer = nullptr;
	int connect(qstring host, qstring port);
	ssize_t send_message(const char* buf, size_t buflen, bool fin);
	ssize_t send_message(const qstring& buffer, bool fin);
	int connection_active();
	void close_connection();  // Note: This function is not fully tested.
	void release();

	uint8_t recv_buf[65535];
	uint8_t egress_out[Q_MAX_DATAGRAM_SIZE];

    std::queue<qdata*> send_buffer;
	unsigned cid_hash_val = 0;
	bool issue_close = false;
    std::atomic<bool> fin_received = false;
    ev_tstamp last_flush_time;
};

class bridge_qconnection {
   public:
	virtual void onconnect(conn_io_client* qconnection) = 0;
	virtual void onmessage(ssize_t recv_len, uint8_t* buf, conn_io_client* qconnection) = 0;
	virtual void onreleaseconnection(conn_io_client* qconnection) = 0;
	virtual void onclose(conn_io_client* qconnection) = 0;
};

enum con_state { STATE_OPEN, STATE_CONNECT, STATE_CLOSE };

class bridge_qcommand {
   public:
	virtual ssize_t flushegress(struct ev_loop* loop, conn_io_client* qconnection) = 0;
	virtual int release_connection(struct ev_loop* loop, conn_io_client* qconnection) = 0;
	inline virtual struct ev_loop* getmainloop() = 0;

	virtual void event_connect(conn_io_client* qconnection) = 0;
	virtual void event_msg_received(ssize_t recv_len, uint8_t* buf, conn_io_client* qconnection, bool fin) = 0;
	virtual void event_close(conn_io_client* qconnection) = 0;
	virtual int send_message(const qstring& buffer, bool flush) = 0;
	virtual int send_message(const uint8_t* buffer, ssize_t size, bool flush) = 0;
	virtual int close() = 0;
	virtual con_state getstate() = 0;

#if USE_PTHREAD
	virtual qmutex* get_run_mutex() = 0;
	virtual qmutex* get_close_mutex() = 0;
	virtual qmutex* get_send_mutex() = 0;
	virtual qmutex* get_sendloop_mutex() = 0;
#endif
};

class qnetworkclient : public bridge_qcommand, public bridge_qconnection {
   private:
	struct run_config {
		qstring host;
		qstring port;
		qnetworkclient* thiz;
		int pthread_return_value;
		bool finished = false;
		int id = -1;
	};
	static int connection_id;
	struct ev_loop* mainloop = nullptr;
	struct run_config run_config_data;

	ev_timer sendto_retry_timer;
	int sendto_retry_count = 0;
	std::deque<socket_data*> pending_socket_data_buffer;
	ev_timer heart_beat_timer;

	void reset_sendto_retry_timer();
	void destroy_pending_socket_data();
	ssize_t socket_sendto(int sockfd, const void* buf, size_t len, int flags, const struct sockaddr* dest_addr, socklen_t addrlen);
#if USE_PTHREAD
	qmutex run_mutex;
	qmutex close_mutex;
	qmutex send_mutex;
	qmutex sendloop_mutex;
	qmutex runconfig_mutex;
	pthread_t run_thread_id;
#endif

	static void debug_log(const char* line, void* argp);
	static void recv_cb(EV_P_ ev_io* w, int revents);
	static void timeout_cb(EV_P_ ev_timer* w, int revents);
	static void send_cb(EV_P_ ev_timer* w, int revents);
	static void sendto_retry_cb(EV_P_ ev_timer* w, int revents);
	static void heart_beat_cb(EV_P_ ev_timer* w, int revents);
	static void* run_internal(void* data);

	void setstate(con_state state);
	con_state state = STATE_OPEN;

   protected:
	ssize_t flushegress(struct ev_loop* loop, conn_io_client* qconnection) final;
	int release_connection(struct ev_loop* loop, conn_io_client* qconnection) final;
	void onconnect(conn_io_client* qconnection) override;
	void onmessage(ssize_t recv_len, uint8_t* buf, conn_io_client* qconnection) override;
	void onreleaseconnection(conn_io_client* qconnection) override;
	void onclose(conn_io_client* qconnection) override;
	inline struct ev_loop* getmainloop() final { return mainloop; }
	void event_connect(conn_io_client* qconnection) final;
	void event_msg_received(ssize_t recv_len, uint8_t* buf, conn_io_client* qconnection, bool fin) final;
	void event_close(conn_io_client* qconnection) final;

#if USE_PTHREAD
	qmutex* get_run_mutex() final { return &run_mutex; }
	qmutex* get_close_mutex() final { return &close_mutex; }
	qmutex* get_sendloop_mutex() final { return &sendloop_mutex; }
	qmutex* get_send_mutex() final { return &send_mutex; }
#endif
	conn_io_client* qclient_connection = nullptr;

   public:
	qnetworkclient();
	~qnetworkclient();

	inline con_state getstate() final { return state; }
	inline bool isopen() { return state == STATE_OPEN; }
	inline bool isclosed() { return state == STATE_CLOSE; }

	int send_message(const qstring& buffer, bool flush) final;
	int send_message(const uint8_t* buffer, ssize_t size, bool flush) final;
	int close() final;
	bool is_runfinished();
	int run(qstring host, qstring port);
	void forcerelease();
    inline bool is_fin_received()  { return qclient_connection && qclient_connection->fin_received.load(); }
    
#if USE_PTHREAD
	inline qmutex& get_runconfig_mutex() { return runconfig_mutex; }
#endif
};
};	// namespace client
#endif /* qnetworkclient_hpp */
