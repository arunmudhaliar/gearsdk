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
#include <uv.h>
}

#include "../../common/sdktypes.hpp"
#include "../../networkcommon/source/essentials.hpp"
#include "../../networkcommon/source/qstatslogger.hpp"
#include "../../networkcommon/source/qtextfilelogger.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "qh3server"

#define LOCAL_CONN_ID_LEN 16
#undef ORIGINAL_CLIENT_ADDR_SZ
#define ORIGINAL_CLIENT_ADDR_SZ (3 * sizeof(uint16_t))
#define MAX_DATAGRAM_SIZE 1350 - ORIGINAL_CLIENT_ADDR_SZ  // last 6 bytes is reserved for original client address verification

#define MAX_TOKEN_LEN sizeof("quiche") - 1 + sizeof(struct sockaddr_storage) + MAX_CID_LEN

#define SEND_CHUNK_SIZE 256
#define DROP_CONNECTION_AFTER 45.0f	 // in seconds

#define DEFAULT_TIMER_ROUTER_HB_INTERVAL_IN_SECONDS 20.0f
// trouble shoot
// https://www.chromium.org/for-testers/providing-network-details/

// MARK: -
class bridge_h3_connection {
   public:
	virtual ssize_t flush_egress(struct conn_io_qh3* conn_io) = 0;
	virtual void destroy_connection(struct conn_io_qh3* conn_io) = 0;
	inline virtual uv_loop_t* get_mainloop() = 0;
	virtual void parse_header(const qstring& name, const qstring& value, struct conn_io_qh3* conn_io) = 0;
	virtual void parse(struct conn_io_qh3* conn_io) = 0;
	virtual bool is_log_quiche() = 0;
	virtual float get_router_hb_interval_in_sec() = 0;
};

// MARK: -
struct connections {
	int sock;
	uv_poll_t poll_handle;
	uv_udp_t udp_handle;
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
			uv_timer_stop(&timer);
			if (!uv_is_closing((uv_handle_t*) &timer)) {
				uv_close((uv_handle_t*) &timer, nullptr);
				// Run the loop again to process the cleanup
				uv_run(bridge->get_mainloop(), UV_RUN_ONCE);
			}
		}
		if (conn) {
			bool is_closed = quiche_conn_is_closed(conn);
			if (!is_closed) {
				int close_result = quiche_conn_close(conn, true, 0, NULL, 0);
				if (close_result < 0) {
					debug_print_error(__LOGTAG__, "failed to close connection, err %d", close_result);
				}
			}
			quiche_conn_free(conn);
			conn = nullptr;
		}
		GX_DELETE(http_response);
		GX_DELETE(http_request);
	}
	uv_timer_t timer;
	int sock;
	uint8_t cid[LOCAL_CONN_ID_LEN];
	Connection* conn = nullptr;
	Connection* http3 = nullptr;
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

// MARK: -
class qh3server : public bridge_h3_connection {
   private:
	Config* config = nullptr;
	Config* http3_config = nullptr;
	struct connections* conns = nullptr;
	uv_loop_t* mainloop = nullptr;

	static void debug_log(const uint8_t* line, void* argp);
	ssize_t flush_egress(struct conn_io_qh3* conn_io) final;
	void destroy_connection(struct conn_io_qh3* conn_io) final;
	inline uv_loop_t* get_mainloop() final { return mainloop; }
	inline bool is_log_quiche() override { return false; }
	float get_router_hb_interval_in_sec() override { return DEFAULT_TIMER_ROUTER_HB_INTERVAL_IN_SECONDS; }
	void parse(struct conn_io_qh3* conn_io) override;

	void mint_token(const uint8_t* dcid, size_t dcid_len, struct sockaddr_storage* addr, socklen_t addr_len, uint8_t* token, size_t* token_len);
	bool validate_token(const uint8_t* token, size_t token_len, struct sockaddr_storage* addr, socklen_t addr_len, uint8_t* odcid, size_t* odcid_len);
	static uint8_t* gen_cid(uint8_t* cid, size_t cid_len);
	struct conn_io_qh3* create_conn(uint8_t* scid, size_t scid_len, uint8_t* odcid, size_t odcid_len, struct sockaddr* local_addr, socklen_t local_addr_len, struct sockaddr_storage* peer_addr, socklen_t peer_addr_len,
									struct sockaddr_storage* peer_original_client_addr);
	static int for_each_header(const uint8_t* name, size_t name_len, const uint8_t* value, size_t value_len, void* argp);
	static void recv_cb(uv_poll_t* handle, int status, int events);
	static void timeout_cb(uv_timer_t* handle);

	void send_in_chunks(struct conn_io_qh3* conn_io);

	qtimer_uv* router_hb_loop(qtimer_uv_scheduler& router_hb_scheduler, const qstring& host, const qstring& port, int sock, uint16_t command_center_feedback_port);

	void stop_services_and_report(int sock, uint16_t command_center_feedback_port);

	uint8_t out[MAX_DATAGRAM_SIZE + ORIGINAL_CLIENT_ADDR_SZ];
	uint8_t buf[65535];
	routerinfo* relay_through_router_info = nullptr;  // only valid for servers else NULL

   protected:
	virtual bool on_server_pre_init() = 0;
	virtual void on_server_uninitialise() = 0;
	virtual void on_run_started() = 0;
	virtual void on_run_end() = 0;
	void parse_header(const qstring& name, const qstring& value, struct conn_io_qh3* conn_io) override;
	qtextfilelogger* logger = nullptr;
	qstatslogger* stats_logger = nullptr;
	qstring logtag = __LOGTAG__;
	qstring port_id;
	qstring host_id;
	fs::path app_directory = ".";

   public:
	virtual ~qh3server();
	qtextfilelogger* get_file_logger() { return logger; }
	qstatslogger* get_stats_loggeer() { return stats_logger; }

	int run(const qstring& host, const qstring& port, const fs::path& root_dir, struct addrinfo* router, uint16_t command_center_feedback_port, uint16_t router_port_return);
};

#endif /* qh3server_hpp */
