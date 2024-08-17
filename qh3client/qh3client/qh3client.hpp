//
//  Copyright 2024 homenet25
//  qh3client.hpp
//  qh3client
//
//  Created by Arun A on 30/10/23.
//

#ifndef qh3client_hpp
#define qh3client_hpp

extern "C" {
#include <ev.h>
#include <fcntl.h>
#include <quiche.h>
}

#include "../../networkcommon/source/essentials.hpp"

#include <map>
#include <string>

#define LOCAL_CONN_ID_LEN 16

#undef ORIGINAL_CLIENT_ADDR_SZ
#define ORIGINAL_CLIENT_ADDR_SZ (3 * sizeof(uint16_t))
#define MAX_DATAGRAM_SIZE 1350 - ORIGINAL_CLIENT_ADDR_SZ  // last 6 bytes is reserved for original client adress verification
#define CONNECTION_ESTABLISHMENT_TIMEOUT 5.0	// in seconds
#undef __LOGTAG__
#define __LOGTAG__ "qh3client"

namespace client {
class bridge_h3client_connection;
struct conn_io_qh3_client {
	conn_io_qh3_client() { response = conn_io_req_res::create(); }
	~conn_io_qh3_client() { GX_DELETE(response); }
	ev_timer timer;
	ev_timer connection_establishment_timeout_timer; // Timer for connection establishment timeout
	const char* host = nullptr;

	int sock;
	struct sockaddr_storage local_addr;
	socklen_t local_addr_len;

	Connection* conn = nullptr;
	Connection* http3 = nullptr;
	bridge_h3client_connection* bridge = nullptr;

	bool req_sent = false;
	bool settings_received = false;
	uint8_t buf[65535];
	uint8_t out[MAX_DATAGRAM_SIZE + ORIGINAL_CLIENT_ADDR_SZ];
	conn_io_req_res* response = nullptr;
	bool res_received = false;
	ev_tstamp creation_time = 0;
};

class bridge_h3client_connection {
   public:
	virtual void flush_egress(struct ev_loop* loop, struct conn_io_qh3_client* conn_io) = 0;
	inline virtual struct ev_loop* get_mainloop() = 0;
	inline virtual const struct conn_io_req_res* get_getorpost_http_request() = 0;
	virtual int64_t send_get_http_request(const conn_io_req_res* data_getorpost_, struct conn_io_qh3_client* conn_io) = 0;
	virtual int64_t send_post_http_request(const conn_io_req_res* data_getorpost_, struct conn_io_qh3_client* conn_io) = 0;
};

typedef std::function<void(conn_io_req_res* request, conn_io_req_res* response, void* client_specific_data, void* arg, bool success)> type_qh3client_helper_cb;

class qh3client : public bridge_h3client_connection {
   public:
	qh3client(const qstring& host, const qstring& port, void* arg);
	virtual ~qh3client();

	struct ev_loop* mainloop = nullptr;

	static void debug_log(const uint8_t* line, void* arg);
	void flush_egress(struct ev_loop* loop, struct conn_io_qh3_client* conn_io) final;
	inline struct ev_loop* get_mainloop() final { return mainloop; }
	inline const struct conn_io_req_res* get_getorpost_http_request() final { return http_request; }
	static int for_each_setting(uint64_t identifier, uint64_t value, void* argp);
	static int for_each_header(const uint8_t* name, size_t name_len, const uint8_t* value, size_t value_len, void* argp);
	static void recv_cb(EV_P_ ev_io* w, int revents);
	static void timeout_cb(EV_P_ ev_timer* w, int revents);
	static void connection_establishment_timeout_cb(EV_P_ ev_timer* w, int revents);
	int64_t send_get_http_request(const conn_io_req_res* data_getorpost_, struct conn_io_qh3_client* conn_io) final;
	int64_t send_post_http_request(const conn_io_req_res* data_getorpost_, struct conn_io_qh3_client* conn_io) final;
	virtual void on_prepare_client_send();
	int send_request(const conn_io_req_res* data_getorpost_, type_qh3client_helper_cb response_cb_);
	virtual void on_post_send_cleanup();
	virtual void* get_client_specific_data();
	const qstring host;
	const qstring port;
	const conn_io_req_res* http_request = nullptr;
	struct conn_io_qh3_client* conn_io = nullptr;
	void* arg = nullptr;

   private:
	type_qh3client_helper_cb response_cb = nullptr;
	int close_socket(int sock);
};
};	// namespace client
#endif /* qh3client_hpp */
