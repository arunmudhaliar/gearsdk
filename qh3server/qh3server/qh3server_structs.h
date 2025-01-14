//
//  qh3server_structs.h
//  qh3server
//
//  Created by Arun A on 27/12/24.
//

#ifndef qh3server_structs_h
#define qh3server_structs_h

#define USE_UV_MAIN_LOOP 0

#include "../../common/sdktypes.hpp"
#include "../../networkcommon/source/essentials.hpp"

#include <quiche.h>
#include <uthash.h>

#undef __LOGTAG__
#define __LOGTAG__ "qh3server_structs"

#define LOCAL_CONN_ID_LEN 16
#undef ORIGINAL_CLIENT_ADDR_SZ
#define ORIGINAL_CLIENT_ADDR_SZ (3 * sizeof(uint16_t))
#define MAX_DATAGRAM_SIZE 1350 - ORIGINAL_CLIENT_ADDR_SZ  // last 6 bytes is reserved for original client address verification

#define MAX_TOKEN_LEN sizeof("quiche") - 1 + sizeof(struct sockaddr_storage) + QUICHE_MAX_CONN_ID_LEN

#define SEND_CHUNK_SIZE 256
#define DROP_CONNECTION_AFTER 45.0f	 // in seconds

#define DEFAULT_TIMER_ROUTER_HB_INTERVAL_IN_SECONDS 20.0f

#if USE_UV_MAIN_LOOP
#define EVENT_LOOP_TYPE uv_loop_t
#define EVENT_TIMER_TYPE uv_timer_t
#define TIMER_SCHEDuLER_TYPE qtimer_uv_scheduler
#define TIMER_TYPE qtimer_uv
#else
#define EVENT_LOOP_TYPE struct ev_loop
#define EVENT_TIMER_TYPE ev_timer
#define TIMER_SCHEDuLER_TYPE qtimer_scheduler
#define TIMER_TYPE qtimer
#endif

// MARK: -
class bridge_h3_connection {
   public:
	enum parse_return { parse_sync, parse_async };
	virtual ssize_t flush_egress(struct conn_io_qh3* conn_io) = 0;
	virtual void destroy_connection(struct conn_io_qh3* conn_io) = 0;
	inline virtual EVENT_LOOP_TYPE* get_mainloop() = 0;
	virtual void parse_header(const qstring& name, const qstring& value, struct conn_io_qh3* conn_io) = 0;
	virtual parse_return parse(struct conn_io_qh3* conn_io) = 0;
	virtual bool is_log_quiche() = 0;
	virtual float get_router_hb_interval_in_sec() = 0;
};

// MARK: -
struct connections {
	int sock;
#if USE_UV_MAIN_LOOP
	uv_poll_t poll_handle;
	uv_udp_t udp_handle;
#endif
	struct sockaddr* local_addr = nullptr;
	socklen_t local_addr_len;
	struct conn_io_qh3* h = nullptr;
	qstring server_port;
	qstring quic_alternate_protocol_str;
};

// MARK: -
struct conn_io_qh3 {
	explicit conn_io_qh3(bridge_h3_connection* bridge) : bridge(bridge) {
		http_request = conn_io_req_res::create();
		http_response = conn_io_req_res::create();
	}
	~conn_io_qh3() {
		if (bridge) {
#if USE_UV_MAIN_LOOP
			uv_timer_stop(&timer);
			if (!uv_is_closing((uv_handle_t*) &timer)) {
				uv_close((uv_handle_t*) &timer, nullptr);
				// Run the loop again to process the cleanup
				uv_run(bridge->get_mainloop(), UV_RUN_ONCE);
			}
#else
			ev_timer_stop(bridge->get_mainloop(), &timer);
#endif
		}
		if (conn) {
			bool is_closed = quiche_conn_is_closed(conn);
			if (!is_closed) {
				const char* reason = "closure by server";
				int close_result = quiche_conn_close(conn, true, 0, (const uint8_t*) reason, strlen(reason));
				if (close_result < 0) {
					bool is_draining = quiche_conn_is_draining(conn);
					if (!is_draining) {
						debug_print_error(__LOGTAG__, "failed to close connection, err %d, draining %d", close_result, is_draining);
					}
				}
			}
			quiche_conn_free(conn);
			conn = nullptr;
		}
		if (http3) {
			quiche_h3_conn_free(http3);
			http3 = nullptr;
		}
		GX_DELETE(http_response);
		GX_DELETE(http_request);
	}
	EVENT_TIMER_TYPE timer;
	int sock;
	uint8_t cid[LOCAL_CONN_ID_LEN];
	quiche_conn* conn = nullptr;
	quiche_h3_conn* http3 = nullptr;
	struct sockaddr_storage peer_addr;
	socklen_t peer_addr_len;
	UT_hash_handle hh;
	bridge_h3_connection* bridge = nullptr;
	conn_io_req_res* http_request = nullptr;
	conn_io_req_res* http_response = nullptr;
	uint64_t creation_time = {0};
	ssize_t total_sent_bytes = 0;
	int64_t stream_id = -1;
	qstring original_client_serialised_buffer;
	unsigned cid_hash_val = 0;
	uint8_t skip_destroy_counter = 0;
};

// MARK: -
struct routerinfo {
	routerinfo(struct addrinfo* router, uint16_t port_return) : port_return(port_return) {
		router_address = DEBUG_NEW qaddress(*router->ai_addr);
		router_address->serialise(serialised_buffer);
		router = (struct addrinfo*) malloc(sizeof(struct addrinfo));
		memcpy(router, router, sizeof(struct addrinfo));
	}
	~routerinfo() {
		free(router);
		router = nullptr;
		GX_DELETE(router_address);
	}
	struct addrinfo* router = nullptr;
	qaddress* router_address = nullptr;
	qstring serialised_buffer;
	uint16_t port_return = 4005;
};

class qh3server;
class observer_qh3server_events {
   public:
	struct qh3plugin_server_response_buffer {
		char* data = nullptr;
		int size = 0;
	};
	virtual ~observer_qh3server_events() {};
	virtual void on_server_pre_start(qh3server*) = 0;
	virtual void on_server_start(qh3server*, const char* ip, uint16_t port) = 0;
	virtual void on_server_stop(qh3server*) = 0;
	virtual void on_server_error(qh3server*, int error_code) = 0;
	virtual void on_serevr_parse(qh3server*, const conn_io_qh3* conn, const char* path, const char* buffer, unsigned long len, const char* headers_buffer, unsigned long headers_buffer_size) = 0;
};
#endif /* qh3server_structs_h */
