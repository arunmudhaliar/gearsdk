//
//  http3_sample_server.hpp
//  qh3server
//
//  Created by Arun A on 31/10/23.
//

#ifndef http3_sample_server_hpp
#define http3_sample_server_hpp

#include "../../common/serverinforeader.hpp"
#include "../../networkcommon/source/message.hpp"
#include "../../networkcommon/source/qtextfile.hpp"
#include "../../networkcommon/source/serverconfig.hpp"
#include "../../qhiredis/source/qhiredis.hpp"
#include "../../qzookeeper/source/qzookeeper.hpp"
#include "../../servercommon/source/qmongo/qmongo.hpp"
#include "qh3router_structs.h"
#include "qh3server.hpp"

#define DEFAULT_USER_TOKEN_EXPIRY_TIME 300	// in seconds

#undef __LOGTAG__
#define __LOGTAG__ "http3_sample_server"

class http3_sample_server : public qh3server, interface_qmongo_connection, observer_serverconfig {
   protected:
	void parse_header(const qstring& name, const qstring& value, struct conn_io_qh3* conn_io) override;
	parse_return parse(struct conn_io_qh3* conn_io) override;
	inline bool is_log_quiche() override;

	bool on_server_pre_init() override;
	void on_server_uninitialise() override;
	void on_run_started() override;
	void on_run_end() override;

	float get_router_hb_interval_in_sec() override;

	// mongo callbacks
	void on_mongo_connect() override final;
	void on_mongo_create_index_keys(const qstring& collection_name, bson_t* indexkey, mongoc_index_opt_t* opt) override final;

	void refresh_roomconfig_meta();
	void configchanged(const qstring& path, const qstring& data) override;

	qmongo* mongo = nullptr;
	qhiredis* hiredis = nullptr;
	qzookeeper* qzk = nullptr;
	serverconfig* zkconfig = nullptr;
	msg_room_config_list* room_config_list = nullptr;

   public:
	http3_sample_server(const st_qh3server_config_in& config, const st_services_config& services_config);
	virtual ~http3_sample_server();

	void test_mongo_db();
	static inline const char* get_server_name() { return "http3_sample_server"; }

   private:
	void check_and_update_is_log_quiche_flag();
	int validate_token(conn_io_req_res::header* path_header, struct conn_io_qh3* conn_io);

	void parse_shutdown_test(conn_io_req_res::header* path_header, struct conn_io_qh3* conn_io);
	void parse_whoami(conn_io_req_res::header* path_header, struct conn_io_qh3* conn_io);
	void parse_user_get(conn_io_req_res::header* path_header, struct conn_io_qh3* conn_io);
	void parse_user_details(conn_io_req_res::header* path_header, struct conn_io_qh3* conn_io);
	void parse_get_gservers(conn_io_req_res::header* path_header, struct conn_io_qh3* conn_io);
	void parse_ping(conn_io_req_res::header* path_header, struct conn_io_qh3* conn_io);

	void get_gservers(res_msg_gservers& res_get_gservers);

	message_parser msg_parser;
	bool is_log_quiche_flag = false;
	st_qh3server_config_in config_copy;
	st_services_config services_config_copy;
#if TEST_RESPONSE
	qstring test_response;
#endif
};
#endif /* http3_sample_server_hpp */
