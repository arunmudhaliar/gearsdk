//
//  qh3server.hpp
//  qh3server
//
//  Created by Arun A on 30/10/23.
//

#ifndef qh3server_hpp
#define qh3server_hpp

#include <string>

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <ev.h>
#include <quiche.h>
#include <uthash.h>

#include "../../common/sdktypes.hpp"
#include "../../networkcommon/source/essentials.hpp"
#include "../../networkcommon/source/qstatslogger.hpp"
#include "../../networkcommon/source/qtextfilelogger.hpp"

// #if PLATFORM == PLATFORM_MAC
// namespace fs = std::__fs::filesystem;
// #elif PLATFORM == PLATFORM_LINUX
// namespace fs = std::filesystem;
// #else
// namespace fs = std::__fs::filesystem;
// #endif

#undef __LOGTAG__
#define __LOGTAG__ "qh3server"

#define LOCAL_CONN_ID_LEN 16
#undef ORIGINAL_CLIENT_ADDR_SZ
#define ORIGINAL_CLIENT_ADDR_SZ (3 * sizeof(uint16_t))
#define MAX_DATAGRAM_SIZE 1350 - ORIGINAL_CLIENT_ADDR_SZ // last 6 bytes is reserved for original client adress verification

#define MAX_TOKEN_LEN                     \
	sizeof("quiche") - 1 +                \
		sizeof(struct sockaddr_storage) + \
		MAX_CID_LEN

#define SEND_CHUNK_SIZE 256
#define DROP_CONNECTION_AFTER 45.0f // in seconds

// trouble shoot
// https://www.chromium.org/for-testers/providing-network-details/

class bridge_h3_connection {
  public:
	virtual ssize_t flush_egress(struct ev_loop* loop, struct conn_io_qh3* conn_io) = 0;
	virtual void destroy_connection(struct ev_loop* loop, struct conn_io_qh3* conn_io) = 0;
	inline virtual struct ev_loop* get_mainloop() = 0;
	virtual void parse_header(const qstring& name, const qstring& value, struct conn_io_qh3* conn_io) = 0;
	virtual void parse(struct conn_io_qh3* conn_io) = 0;
	virtual bool is_log_quiche() = 0; // NOTE : TODO - This is polling which is not a recommended solution. Need to use event based system.
};

struct connections {
	int sock;
	struct sockaddr* local_addr = nullptr;
	socklen_t local_addr_len;
	struct conn_io_qh3* h = nullptr;
	qstring server_port;
	qstring quic_alternate_protocol_str;
};

struct conn_io_qh3 {
	conn_io_qh3() {
		http_request = conn_io_req_res::create();
		http_response = conn_io_req_res::create();
	}
	~conn_io_qh3() {
		GX_DELETE(http_response);
		GX_DELETE(http_request);
	}
	ev_timer timer;
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
	ev_tstamp creation_time = 0;
	ssize_t total_sent_bytes = 0;
	int64_t stream_id = -1;
	qstring original_client_serialised_buffer;
};

struct routerinfo {
	routerinfo(struct addrinfo* router_, uint16_t port_return)
		: port_return(port_return) {
		router_address = DEBUG_NEW qaddress(*router_->ai_addr);
		router_address->serialise(serialised_buffer);
		router = (struct addrinfo*) malloc(sizeof(struct addrinfo));
		memcpy(router, router_, sizeof(struct addrinfo));
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

class qh3server : public bridge_h3_connection {
  private:
	Config* config = nullptr;
	Config* http3_config = nullptr;
	struct connections* conns = nullptr;
	struct ev_loop* mainloop = nullptr;

	static void debug_log(const uint8_t* line, void* argp);
	ssize_t flush_egress(struct ev_loop* loop, struct conn_io_qh3* conn_io) override final;
	void destroy_connection(struct ev_loop* loop, struct conn_io_qh3* conn_io) override final;
	inline virtual struct ev_loop* get_mainloop() override final {
		return mainloop;
	}
	inline virtual bool is_log_quiche() override {
		return false;
	}

	void parse(struct conn_io_qh3* conn_io) override;

	void mint_token(const uint8_t* dcid, size_t dcid_len,
					struct sockaddr_storage* addr, socklen_t addr_len,
					uint8_t* token, size_t* token_len);
	bool validate_token(const uint8_t* token, size_t token_len,
						struct sockaddr_storage* addr, socklen_t addr_len,
						uint8_t* odcid, size_t* odcid_len);
	static uint8_t* gen_cid(uint8_t* cid, size_t cid_len);
	struct conn_io_qh3* create_conn(uint8_t* scid, size_t scid_len,
									uint8_t* odcid, size_t odcid_len,
									struct sockaddr* local_addr,
									socklen_t local_addr_len,
									struct sockaddr_storage* peer_addr,
									socklen_t peer_addr_len,
									struct sockaddr_storage* peer_original_client_addr);
	static int for_each_header(const uint8_t* name, size_t name_len,
							   const uint8_t* value, size_t value_len,
							   void* argp);
	static void recv_cb(EV_P_ ev_io* w, int revents);
	static void timeout_cb(EV_P_ ev_timer* w, int revents);

	void send_in_chunks(struct conn_io_qh3* conn_io);

	uint8_t out[MAX_DATAGRAM_SIZE + ORIGINAL_CLIENT_ADDR_SZ];
	uint8_t buf[65535];
	routerinfo* relay_through_router_info = nullptr; // only valid for servers else NULL

  protected:
	virtual bool on_server_pre_init() = 0;
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

	int run(const qstring& host, const qstring& port, fs::path& rootDir, struct addrinfo* router, uint16_t command_center_feedback_port, uint16_t router_port_return);
};

#endif /* qh3server_hpp */
