//
//  http3_command_server.hpp
//  qh3server
//
//  Created by Arun A on 31/10/23.
//

#ifndef http3_command_server_hpp
#define http3_command_server_hpp

#include "../../common/crypto_helper.hpp"
#include "../../networkcommon/source/qbuffer.hpp"
#include "../../qhiredis/source/qhiredis.hpp"
#include "qh3server.hpp"
#include "qh3simple_router_structs.h"
#include <qh3client_helper.hpp>

#undef __LOGTAG__
#define __LOGTAG__ "http3_command_server"

class http3_command_server : public qh3server {
  protected:
	void parse_header(const qstring& name, const qstring& value, struct conn_io_qh3* conn_io) override;
	void parse(struct conn_io_qh3* conn_io) override;
	inline bool is_log_quiche() override;

	bool on_server_pre_init() override;
	void on_run_started() override;
	void on_run_end() override;

	qhiredis* hiredis = nullptr;

  public:
	http3_command_server(const qstring& redis_url, uint16_t redis_port, bridge_command_center* bridge, qstring router_port);
	~http3_command_server();
	static void command_feedback_recv_cb(EV_P_ ev_io* w, int revents);

  private:
	void parse_shutdown_test(conn_io_req_res::header* path_header, struct conn_io_qh3* conn_io);
	void parse_shutdown_command_center(conn_io_req_res::header* path_header, struct conn_io_qh3* conn_io);
	void parse_whoami(conn_io_req_res::header* path_header, struct conn_io_qh3* conn_io);

	// response
	void construct_response_whoami(qstring& response_string);

	// commands
	void send_shutdown_to_all();

	bridge_command_center* bridge;
	qstring router_port;
};
#endif /* http3_command_server_hpp */
