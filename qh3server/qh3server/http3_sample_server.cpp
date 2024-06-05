//
//  Copyright 2024 homenet25
//  http3_sample_server.cpp
//  qh3server
//
//  Created by Arun A on 31/10/23.
//

#include "http3_sample_server.hpp"

#include "../../common/crypto_helper.hpp"
#include "../../common/gxcrc32.h"
#include "../../networkcommon/source/qbuffer.hpp"

http3_sample_server::http3_sample_server(const qstring& mongodb_uri, const qstring& redis_ip, uint16_t redis_port, const qstring& zk_uri) : zk_uri(zk_uri) {
	mongo = DEBUG_NEW qmongo(this, "qh3", "db_name", mongodb_uri);
	hiredis = DEBUG_NEW qhiredis(redis_ip, redis_port);
}

http3_sample_server::~http3_sample_server() {
	GX_DELETE(hiredis);
	GX_DELETE(mongo);
	GX_DELETE(room_config_list);
}

bool http3_sample_server::on_server_pre_init() {
#if ENABLE_ZK
	GX_DELETE(qzk);
	qzk = DEBUG_NEW qzookeeper();
	int zk_result = qzk->connect(zk_uri);
	if (zk_result != 0) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "zk failed to connect !!!, Exiting.");
		GX_DELETE(qzk);
		return false;
	}

	GX_DELETE(zkconfig);
	zkconfig = DEBUG_NEW serverconfig(qzk);
#if DEV_BUILD
	fs::path config_path(app_directory / "configs/dev/runtime-config.json");
	zkconfig->load(config_path, qzk, "/qh3server");
	zkconfig->load(config_path, qzk, "/qh3router");
#elif PROD_BUILD
	fs::path config_path(app_directory / "configs/prod/runtime-config.json");
	zkconfig->load(config_path, qzk, "/qh3server");
	zkconfig->load(config_path, qzk, "/qh3router");
#else
	fs::path config_path(app_directory / "configs/dev/runtime-config.json");
	zkconfig->load(config_path, qzk, "/qh3server");
	zkconfig->load(config_path, qzk, "/qh3router");
#endif
#endif

	if (mongo->connect() != 0) {
		return false;
	}

	if (hiredis->connect_redis() != 0) {
		return false;
	}

	//	msg_parser.register_message_type<rq_msg_user_get>();
	//	msg_parser.register_message_type<msg_room_config_list>();
	//	msg_parser.register_message_type<msg_room_config>();
	qstring room_config_list_str(zkconfig->get_string("gserver/roomconfig", ""));
	GX_DELETE(room_config_list);
	room_config_list = msg_parser.parse<msg_room_config_list>(room_config_list_str.length(), (uint8_t*) room_config_list_str.c_str());
	DEBUG_ASSERT(__LOGTAG__, (room_config_list != nullptr), "Invalid room configs !!!");
	return true;
}

void http3_sample_server::on_run_started() {
	hiredis->set_hash_value(qstring::format_string("servers:%s", gsdk::server::machine_public_ip), qstring::format_string("server-%s", port_id.c_str()), qstring::format_string("%s:%s", host_id.c_str(), port_id.c_str()));
}

void http3_sample_server::on_run_end() {
	GX_DELETE(zkconfig);
#if ENABLE_ZK
	qzk->shutdown();
	DEBUG_PRINT_IMPORTANT(__LOGTAG__, "waiting for http3_sample_server services to finish !!!");
	struct ev_loop* wait_loop = ev_loop_new();
	qtimer_sceduler wait_scheduler;
	wait_scheduler.set_ev_lopp(wait_loop);
	qtimer* wait_timer = wait_scheduler.schedule_repeat_timer(
		[this, wait_loop](qtimer& timer) {
			UNUSED(timer);
			if (!qzk->is_running()) {
				DEBUG_PRINT_IMPORTANT(__LOGTAG__, "qzk service finished !!!");
				ev_break(wait_loop, EVBREAK_ONE);
			}
		},
		3);
	ev_run(wait_loop, 0);
	wait_scheduler.cancel_and_destroy_timer(wait_timer);
	ev_loop_destroy(wait_loop);
	GX_DELETE(qzk);
#endif
}

bool http3_sample_server::is_log_quiche() {
	if (hiredis == nullptr) {
		return false;
	}
	qstring is_log_quiche;
	if (hiredis->get_value("is_log_quiche", is_log_quiche) == 0) {
		return is_log_quiche == "true";
	}
	return false;
}

float http3_sample_server::get_router_hb_interval_in_sec() {
	int timer_router_hb_interval_in_sec = zkconfig->get_int32("router/timer_router_hb_interval_in_sec", DEFAULT_TIMER_ROUTER_HB_INTERVAL_IN_SECONDS);
	return timer_router_hb_interval_in_sec;
}

void http3_sample_server::parse_header(const qstring& name, const qstring& value, struct conn_io_qh3* conn_io) {
	qh3server::parse_header(name, value, conn_io);
}

void http3_sample_server::parse(struct conn_io_qh3* conn_io) {
	const char* const_logtag = logtag.c_str();
	const char* port_id_cstr = port_id.c_str();
	conn_io_req_res::header* path_header = conn_io->http_request->get_header(":path");
	if (path_header == nullptr) {
		DEBUG_PRINT_ERROR(const_logtag, "path_header == null, returning. !!!");
		qh3server::get_stats_loggeer()->server_count("parse", 1, "", "", "", "error", "http3_sample_server", "", port_id_cstr, "path_not_found");
		return;
	}

	if (path_header->value.length() <= 1) {
		DEBUG_PRINT_WARN(const_logtag, "path is very short - %s, returning. !!!", path_header->value.c_str());
		qh3server::get_stats_loggeer()->server_count("parse", 1, "", "", "", "warn", "http3_sample_server", path_header->value.c_str(), port_id_cstr, "short_path");
		return;
	}

	// parse paths
	if (path_header->value.compare("/shutdown_test") == 0) {
		parse_shutdown_test(path_header, conn_io);
	} else if (path_header->value.compare("/whoami") == 0) {
		parse_whoami(path_header, conn_io);
	} else if (path_header->value.compare("/user_get") == 0) {
		parse_user_get(path_header, conn_io);
	} else if (path_header->value.compare("/user_details") == 0) {
		parse_user_details(path_header, conn_io);
	} else if (path_header->value.compare("/get_gservers") == 0) {
		parse_get_gservers(path_header, conn_io);
	} else if (path_header->value.compare("/ping") == 0) {
		parse_ping(path_header, conn_io);
	}
}

void http3_sample_server::parse_ping(conn_io_req_res::header* path_header, struct conn_io_qh3* conn_io) {
	const char* const_logtag = logtag.c_str();
	const char* port_id_cstr = port_id.c_str();
	bool has_crc_header = conn_io->http_request->has_crc_header();
	if (has_crc_header) {
		bool validate = conn_io->http_request->validate();
		if (!validate) {
			qh3server::get_stats_loggeer()->server_count("parse", 1, "", "", "", "error", "http3_sample_server", path_header->value.c_str(), port_id_cstr, "crc_fail");
		}
		assert(validate);
	} else {
		// may be called from a browser
		DEBUG_PRINT_IMPORTANT2(const_logtag,
							   "May be '%s' requested from browser. So crc "
							   "validation not possible !!!",
							   path_header->value.c_str());
	}
	const char* res_string = "{\"pong\" : \"http3_sample_server\"}";
	conn_io->http_response->set_payload(qstring(res_string, strlen(res_string)));
	qh3server::get_file_logger()->log(qlogfile::level_0, const_logtag, "%s - ping - %s", path_header->value.c_str(), res_string);
	qh3server::get_stats_loggeer()->server_count("parse", 1, "", "", "", "command", "http3_sample_server", has_crc_header ? "" : "no-crc", port_id_cstr, path_header->value.c_str());
}

void http3_sample_server::parse_shutdown_test(conn_io_req_res::header* path_header, struct conn_io_qh3* conn_io) {
	const char* const_logtag = logtag.c_str();
	const char* port_id_cstr = port_id.c_str();
	bool has_crc_header = conn_io->http_request->has_crc_header();
	if (has_crc_header) {
		bool validate = conn_io->http_request->validate();
		assert(validate);
	} else {
		// may be called from a browser
		DEBUG_PRINT_IMPORTANT2(const_logtag,
							   "May be '%s' requested from browser. So crc "
							   "validation not possible !!!",
							   path_header->value.c_str());
	}
	qh3server::get_stats_loggeer()->server_count("parse", 1, "", "", "", "command", "http3_sample_server", has_crc_header ? "" : "no-crc", port_id_cstr, path_header->value.c_str());
	ev_break(conn_io->bridge->get_mainloop(), EVBREAK_ONE);
}

void http3_sample_server::parse_whoami(conn_io_req_res::header* path_header, struct conn_io_qh3* conn_io) {
	const char* const_logtag = logtag.c_str();
	const char* port_id_cstr = port_id.c_str();
	bool has_crc_header = conn_io->http_request->has_crc_header();
	if (has_crc_header) {
		bool validate = conn_io->http_request->validate();
		if (!validate) {
			qh3server::get_stats_loggeer()->server_count("parse", 1, "", "", "", "error", "http3_sample_server", path_header->value.c_str(), port_id_cstr, "crc_fail");
		}
		assert(validate);
	} else {
		// may be called from a browser
		DEBUG_PRINT_IMPORTANT2(const_logtag,
							   "May be '%s' requested from browser. So crc "
							   "validation not possible !!!",
							   path_header->value.c_str());
	}
	const char* res_string = "{\"name\" : \"http3_sample_server\"}";
	conn_io->http_response->set_payload(qstring(res_string, strlen(res_string)));
	qh3server::get_file_logger()->log(qlogfile::level_0, const_logtag, "%s - whoami - %s", path_header->value.c_str(), res_string);
	qh3server::get_stats_loggeer()->server_count("parse", 1, "", "", "", "command", "http3_sample_server", has_crc_header ? "" : "no-crc", port_id_cstr, path_header->value.c_str());
}

void http3_sample_server::parse_user_get(conn_io_req_res::header* path_header, struct conn_io_qh3* conn_io) {
	EV_START_RECORD(parse_start_time);
	const char* const_logtag = logtag.c_str();
	const char* port_id_cstr = port_id.c_str();
	const conn_io_req_res::payload& payload = conn_io->http_request->get_payload();
	bool validate = conn_io->http_request->validate();
	assert(validate);
	if (!validate) {
		qh3server::get_stats_loggeer()->server_count("parse", 1, "", "", "", "error", "http3_sample_server", path_header->value.c_str(), port_id_cstr, "crc_fail");
	}

	EV_PRINT_IF_ELAPSED_AND_CLEAR(parse_start_time, __LOGTAG__, "user_get : post validate t:%lu ms", 5);

	DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "%.*s", payload.buffer.length(), payload.buffer.c_str());

	rq_msg_user_get* user_get_msg_rq = msg_parser.parse<rq_msg_user_get>(payload.buffer.length(), (uint8_t*) payload.buffer.c_str());
	if (user_get_msg_rq == nullptr) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "Parse failed %.*s", payload.buffer.length(), payload.buffer.c_str());
		qh3server::get_file_logger()->log(qlogfile::level_0, const_logtag, "ERROR - Parse failed %s, req:%.*s, returning. !!!", path_header->value.c_str(), payload.buffer.length(), payload.buffer.c_str());
		qh3server::get_stats_loggeer()->server_count("parse", 1, "", "", "", "error", "http3_sample_server", path_header->value.c_str(), port_id_cstr, "payload_deserialise_fail");
		return;
	}

	EV_PRINT_IF_ELAPSED_AND_CLEAR(parse_start_time, __LOGTAG__, "user_get : post  msg_parser.parse t:%lu ms", 5);

	unsigned long crc = crc32(0L, Z_NULL, 0);
	crc = essentials::mod_crc32_z(crc, (uint8_t*) user_get_msg_rq->sys_name.c_str(), user_get_msg_rq->sys_name.length());
	crc = essentials::mod_crc32_z(crc, (uint8_t*) user_get_msg_rq->node_name.c_str(), user_get_msg_rq->node_name.length());
	crc = essentials::mod_crc32_z(crc, (uint8_t*) user_get_msg_rq->release.c_str(), user_get_msg_rq->release.length());
	crc = essentials::mod_crc32_z(crc, (uint8_t*) user_get_msg_rq->arch.c_str(), user_get_msg_rq->arch.length());

	EV_PRINT_IF_ELAPSED_AND_CLEAR(parse_start_time, __LOGTAG__, "user_get : post crc calculation t:%lu ms", 5);

	qbuffer buffer;
	buffer.allocate(128);
	qbuffer_writer writer;

	writer.write(buffer, crc);
	DEBUG_PRINT(LOG_LEVEL_4, const_logtag, "%s - user id : %x", path_header->value.c_str(), crc);

	// response packet
	res_msg_user_get user_get_msg_respose;
	user_get_msg_respose.pid.format("%lx", crc);
	user_get_msg_respose.user_name.format("guest-%lx", crc);
	time_t givemetime = time(NULL);
	user_get_msg_respose.last_login = strtok(ctime(&givemetime), "\n");
	writer.write(buffer, user_get_msg_respose.last_login);

	// check if redis has token
	qstring token_in_redis;
	qstring redis_format_pid(qstring::format_string("tokens:%s", user_get_msg_respose.pid.c_str()));
	hiredis->get_value(redis_format_pid, token_in_redis);
	if (token_in_redis.length() != 0) {
		user_get_msg_respose.token = token_in_redis;
		DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "token '%s' retreived from redis for user id : %x", user_get_msg_respose.token.c_str(), crc);
	} else {
		// calcualte sha
		crypto_helper::sha256_data sha_data((const char*) buffer.data, (int) buffer.index);
		crypto_helper::sha256(sha_data);
		// session token header
		user_get_msg_respose.token.bin_copy((const uint8_t*) sha_data.out, strlen(sha_data.out));
		DEBUG_PRINT(LOG_LEVEL_2, __LOGTAG__, "new token '%s' for user id : %x", user_get_msg_respose.token.c_str(), crc);
	}

	EV_PRINT_IF_ELAPSED_AND_CLEAR(parse_start_time, __LOGTAG__, "user_get : post token fetch t:%lu ms", 50);

	// set session token on redis
	int32_t user_token_expiry_time = zkconfig->get_int32("server_config/user_token_expiry_time", DEFAULT_USER_TOKEN_EXPIRY_TIME);
	hiredis->set_value(redis_format_pid, user_get_msg_respose.token, user_token_expiry_time);
	//

	conn_io->http_response->add_or_get_header("token", user_get_msg_respose.token);

	// fill gservers
	get_gservers(user_get_msg_respose.gservers);

	EV_PRINT_IF_ELAPSED_AND_CLEAR(parse_start_time, __LOGTAG__, "user_get : post get_gservers t:%lu ms", 50);

	// try find the user. (This needs to improve)
	bool found = false;
	bson_t find_query;
	bson_init(&find_query);
	bson_append_utf8(&find_query, "user.pid", strlen("user.pid"), user_get_msg_respose.pid.c_str(), (int) user_get_msg_respose.pid.length());
	mongoc_cursor_t* cursor = mongo->find("users", find_query);
	const bson_t* doc = nullptr;
	while (mongoc_cursor_next(cursor, &doc)) {
		found = true;
		user_get_msg_respose.room_list = DEBUG_NEW msg_room_config_list(*room_config_list);
		qstring response_json;
		user_get_msg_respose.get_json_string(response_json);
		conn_io->http_response->set_payload(response_json);
		qh3server::get_file_logger()->log(qlogfile::level_0, const_logtag, "%s - user-found - %s", path_header->value.c_str(), response_json.c_str());
	}
	mongoc_cursor_destroy(cursor);

	EV_PRINT_IF_ELAPSED_AND_CLEAR(parse_start_time, __LOGTAG__, "user_get : post mongo find user t:%lu ms", 50);

	// if not found try insert. (This needs to improve)
	if (!found) {
		// mongo
		bson_t res_bson;
		bson_init(&res_bson);
		bson_t meta;
		bson_init(&meta);
		bson_append_document_begin(&res_bson, "user", 4, &meta);
		bson_append_utf8(&meta, "pid", strlen("pid"), user_get_msg_respose.pid.c_str(), (int) user_get_msg_respose.pid.length());
		bson_append_utf8(&meta, "name", strlen("name"), user_get_msg_respose.user_name.c_str(), (int) user_get_msg_respose.user_name.length());
		bson_append_utf8(&meta, "sys_name", strlen("sys_name"), user_get_msg_rq->sys_name.c_str(), (int) user_get_msg_rq->sys_name.length());
		bson_append_utf8(&meta, "node_name", strlen("node_name"), user_get_msg_rq->node_name.c_str(), (int) user_get_msg_rq->node_name.length());
		bson_append_utf8(&meta, "arch", strlen("arch"), user_get_msg_rq->arch.c_str(), (int) user_get_msg_rq->arch.length());
		bson_append_utf8(&meta, "last_login", strlen("last_login"), user_get_msg_respose.last_login.c_str(), (int) user_get_msg_respose.last_login.length());
		bson_append_document_end(&res_bson, &meta);
		if (mongo->insert("users", res_bson) == EXIT_SUCCESS) {
			user_get_msg_respose.room_list = DEBUG_NEW msg_room_config_list(*room_config_list);
			qstring response_json;
			user_get_msg_respose.get_json_string(response_json);
			conn_io->http_response->set_payload(response_json);
			qh3server::get_file_logger()->log(qlogfile::level_0, const_logtag, "%s - new-user - %s", path_header->value.c_str(), response_json.c_str());
		} else {
			qh3server::get_file_logger()->log(qlogfile::level_0, const_logtag, "%s - new-user failed", path_header->value.c_str());
		}
		bson_destroy(&meta);
		bson_destroy(&res_bson);
		EV_PRINT_IF_ELAPSED_AND_CLEAR(parse_start_time, __LOGTAG__, "user_get : post mongo create user t:%lu ms", 50);
	}

	bson_t* update = BCON_NEW("$set", "{", "user.last_login", BCON_UTF8(user_get_msg_respose.last_login.c_str()), "}");
	if (mongo->update("users", find_query, *update) == EXIT_SUCCESS) {
		qh3server::get_file_logger()->log(qlogfile::level_0, const_logtag, "user-last-login - %s, pid:%s", user_get_msg_respose.last_login.c_str(), user_get_msg_respose.pid.c_str());
	} else {
		qh3server::get_file_logger()->log(qlogfile::level_0, const_logtag, "%s - %s", path_header->value.c_str(), user_get_msg_respose.pid.c_str());
	}
	bson_destroy(update);
	bson_destroy(&find_query);
	//

	GX_DELETE(user_get_msg_rq);

	EV_PRINT_IF_ELAPSED_AND_CLEAR(parse_start_time, __LOGTAG__, "user_get : post mongo update user t:%lu ms", 50);

	qh3server::get_stats_loggeer()->server_count("parse", 1, "", "", "", "hit", "http3_sample_server", path_header->value.c_str(), port_id_cstr);
	DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "%s - FINISHED", __FUNCTION__);
}

int http3_sample_server::validte_token(conn_io_req_res::header* path_header, struct conn_io_qh3* conn_io) {
	const char* const_logtag = logtag.c_str();
	const char* port_id_cstr = port_id.c_str();
	const conn_io_req_res::payload& payload = conn_io->http_request->get_payload();
	bool validate = conn_io->http_request->validate();
	assert(validate);
	if (!validate) {
		qh3server::get_stats_loggeer()->server_count("validte_token", 1, "", "", "", "error", "http3_sample_server", path_header->value.c_str(), port_id_cstr, "crc_fail");
	}
	conn_io_req_res::header* token_header = conn_io->http_request->get_header("token");
	if (token_header == nullptr) {
		DEBUG_PRINT_ERROR(const_logtag, "No token header in %s", path_header->value.c_str());
		qh3server::get_file_logger()->log(qlogfile::level_0, const_logtag, "No token header in %s", path_header->value.c_str());
		qh3server::get_stats_loggeer()->server_count("validte_token", 1, "", "", "", "error", "http3_sample_server", path_header->value.c_str(), port_id_cstr, "no_token");
		return -1;
	}

	bson_t bson;
	bson_error_t error;
	if (!bson_init_from_json(&bson, payload.buffer.c_str(), payload.buffer.length(), &error)) {
		DEBUG_PRINT_ERROR(const_logtag, "%s", error.message);
		qh3server::get_file_logger()->log(qlogfile::level_0, const_logtag, "%s - ERROR - %s", path_header->value.c_str(), error.message);
		qh3server::get_stats_loggeer()->server_count("validte_token", 1, "", "", "", "error", "http3_sample_server", path_header->value.c_str(), port_id_cstr, "payload_deserialise_fail");
		return -2;
	}

	// parse
	bson_iter_t iter;
	bson_iter_t sub_iter;
	qstring pid;
	if (bson_iter_init(&iter, &bson) && bson_iter_find_descendant(&iter, "user.pid", &sub_iter)) {
		pid = bson_iter_utf8(&sub_iter, NULL);
	} else {
		bson_destroy(&bson);
		DEBUG_PRINT_ERROR(const_logtag, "user.pid parse failed %s", path_header->value.c_str());
		qh3server::get_file_logger()->log(qlogfile::level_0, const_logtag, "user.pid parse failed %s", path_header->value.c_str());
		qh3server::get_stats_loggeer()->server_count("validte_token", 1, "", "", "", "error", "http3_sample_server", path_header->value.c_str(), port_id_cstr, "user.pid_parse_fail");
		return -3;
	}
	bson_destroy(&bson);

	// check with redis
	qstring token_in_redis;
	qstring redis_format_pid(qstring::format_string("tokens:%s", pid.c_str()));
	hiredis->get_value(redis_format_pid, token_in_redis);
	qh3server::get_stats_loggeer()->server_count("validte_token", 1, token_in_redis, pid, "", token_header->value, "http3_sample_server", path_header->value.c_str(), port_id_cstr, "user.token_check");
	if (token_in_redis != token_header->value) {
		DEBUG_PRINT(LOG_LEVEL_4, const_logtag, "NOT a Valid user %s != %s !!!", token_in_redis.c_str(), token_header->value.c_str());
		return -4;
	}

	DEBUG_PRINT(LOG_LEVEL_4, const_logtag, "Valid user");
	return 0;
}

void http3_sample_server::parse_user_details(conn_io_req_res::header* path_header, struct conn_io_qh3* conn_io) {
	int result = validte_token(path_header, conn_io);
	if (result != 0) {
		return;
	}
#if TEST_RESPONSE
	if (test_response.length() == 0) {
		qtextfile::get_content("./128KB.json", test_response);
	}
	conn_io->http_response->set_payload(test_response);
#endif
}

void http3_sample_server::parse_get_gservers(conn_io_req_res::header* path_header, struct conn_io_qh3* conn_io) {
	int result = validte_token(path_header, conn_io);
	if (result != 0) {
		return;
	}

	res_msg_gservers res_get_gservers;
	get_gservers(res_get_gservers);

	qstring json_payload;
	res_get_gservers.get_json_string(json_payload);
	conn_io->http_response->set_payload(json_payload);
}

void http3_sample_server::get_gservers(res_msg_gservers& res_get_gservers) {
	hiredis->scan("gservers", &res_get_gservers, [](const char* key, const char* field, const char* value, void* arg) {
		DEBUG_PRINT(LOG_LEVEL_4, __LOGTAG__, "%s - %s:%s", key, field, value);
		res_msg_gservers* res_get_gservers = (res_msg_gservers*) arg;
		res_get_gservers->gservers[key].push_back(value);
	});
}

void http3_sample_server::test_mongo_db() {
	qmongo mongo(this, "qh3", "test", "mongodb://192.168.0.230:6006");
	bson_t bson;
	bson_t meta;
	bson_init(&bson);
	bson_append_document_begin(&bson, "meta", 4, &meta);
	bson_append_utf8(&meta, "pid", 3, "mypid1", 6);
	bson_append_utf8(&meta, "name", 4, "myname", 6);
	bson_append_int32(&meta, "age", 3, 40);
	bson_append_utf8(&meta, "loc", 3, "palakkad", 8);
	bson_append_document_end(&bson, &meta);
	mongo.insert("users", bson);
	bson_destroy(&bson);
}

void http3_sample_server::on_mongo_connect() {}

void http3_sample_server::on_mongo_create_index_keys(const qstring& collection_name, bson_t* indexkey, mongoc_index_opt_t* opt) {
	UNUSED(collection_name);
	BSON_APPEND_INT32(indexkey, "user.pid", 1);
	char* idx2_name = mongoc_collection_keys_to_index_string(indexkey);
	assert(strcmp(idx2_name, "user.pid_1") == 0);
	if (opt) {
		opt->unique = true;
		//        BSON_APPEND_BOOL (opt, "unique", true);
	}
}
