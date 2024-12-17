//
//  Copyright 2024 homenet25
//  http3_sample_server.cpp
//  qh3server
//
//  Created by Arun A on 31/10/23.
//

#include "http3_sample_server.hpp"

#include "../../common/crypto_helper.hpp"
#include "../../networkcommon/source/qbuffer.hpp"

http3_sample_server::http3_sample_server(const qstring& mongodb_uri, const qstring& redis_ip, uint16_t redis_port, const qstring& zk_uri) : qh3server(), zk_uri(zk_uri) {
	mongo = DEBUG_NEW qmongo(this, "qh3", "gsdk_mongodb", mongodb_uri);
	hiredis = DEBUG_NEW qhiredis("server_hiredis", redis_ip, redis_port, "gsdkuser", "Fr0gmoon123");
}

http3_sample_server::~http3_sample_server() {
	GX_DELETE(hiredis);
	GX_DELETE(mongo);
	GX_DELETE(room_config_list);
}

void http3_sample_server::configchanged(const qstring& path, const qstring& data) {
	if (path.compare("/qh3server/gserver/roomconfig") == 0) {
		refresh_roomconfig_meta();
	}
}

bool http3_sample_server::on_server_pre_init() {
#if ENABLE_ZK
	GX_DELETE(qzk);
	qzk = DEBUG_NEW qzookeeper(qstring::format_string("zk-%s", port_id.c_str()));
	int zk_result = qzk->connect(zk_uri);
	if (zk_result != 0) {
		debug_print_error(__LOGTAG__, "zk failed to connect !!!, Exiting.");
		GX_DELETE(qzk);
		return false;
	}
#endif

	GX_DELETE(zkconfig);
	zkconfig = DEBUG_NEW serverconfig(qzk, this);
#if PROD_BUILD
	fs::path config_path(app_directory / "configs/prod/runtime-config.json");
#else
	fs::path config_path(app_directory / "configs/dev/runtime-config.json");
#endif
	if (!zkconfig->load(config_path, qzk, "/qh3server")) {
		debug_print_error(__LOGTAG__, "zkconfig load error - %s.", config_path.c_str());
		GX_DELETE(zkconfig);
		return false;
	}
	if (!zkconfig->load(config_path, qzk, "/qh3router")) {
		debug_print_error(__LOGTAG__, "zkconfig load error - %s.", config_path.c_str());
		GX_DELETE(zkconfig);
		return false;
	}

	if (mongo->connect() != 0) {
		return false;
	}

	if (hiredis->connect_redis() != 0) {
		return false;
	}

	refresh_roomconfig_meta();
	return true;
}

void http3_sample_server::refresh_roomconfig_meta() {
	qstring room_config_list_str(zkconfig->get_string("gserver/roomconfig", ""));
	GX_DELETE(room_config_list);
	room_config_list = msg_parser.parse<msg_room_config_list>(room_config_list_str.length(), (uint8_t*) room_config_list_str.c_str());
	DEBUG_ASSERT(__LOGTAG__, (room_config_list != nullptr), "Invalid room configs !!!");
}

void http3_sample_server::on_run_started() {
	hiredis->set_hash_value(qstring::format_string("servers:%s", gsdk::server::machine_public_ip), qstring::format_string("server-%s", port_id.c_str()), qstring::format_string("%s:%s", host_id.c_str(), port_id.c_str()));
	check_and_update_is_log_quiche_flag();
}

void http3_sample_server::on_run_end() {}

void http3_sample_server::on_server_uninitialise() {
	GX_DELETE(zkconfig);
#if ENABLE_ZK
	if (qzk != nullptr) {
		qzk->shutdown();
		debug_print_important(__LOGTAG__, "waiting for zk services to finish !!!");
		struct ev_loop* wait_loop = ev_loop_new();
		qtimer_scheduler wait_scheduler;
		wait_scheduler.set_loop(wait_loop);
		qtimer* wait_timer = wait_scheduler.schedule_repeat_timer(
			[this, wait_loop](qtimer& timer) {
				UNUSED(timer);
				if (!qzk->is_running()) {
					debug_print_important(__LOGTAG__, "qzk service finished !!!");
					ev_break(wait_loop, EVBREAK_ONE);
				}
			},
			3);
		ev_run(wait_loop, 0);
		wait_scheduler.cancel_and_destroy_timer(wait_timer);
		ev_loop_destroy(wait_loop);
		GX_DELETE(qzk);
	}
#endif
}

bool http3_sample_server::is_log_quiche() {
	return is_log_quiche_flag;
}

void http3_sample_server::check_and_update_is_log_quiche_flag() {
	qstring is_log_quiche_value;
	hiredis->get_value("is_log_quiche", is_log_quiche_value);
	is_log_quiche_flag = is_log_quiche_value.compare("true") == 0;
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
		debug_print_error(const_logtag, "path_header == null, returning. !!!");
		qh3server::get_stats_loggeer()->server_count("parse", 1, "", "", "", "error", get_server_name(), "", port_id_cstr, "path_not_found");
		return;
	}

	if (path_header->value.length() <= 1) {
		debug_print_warn(const_logtag, "path is very short - %s, returning. !!!", path_header->value.c_str());
		qh3server::get_stats_loggeer()->server_count("parse", 1, "", "", "", "warn", get_server_name(), path_header->value.c_str(), port_id_cstr, "short_path");
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
	QH3_INFO(const_logtag, "request:%s, payload:%s", path_header->value.c_str(), conn_io->http_response->get_payload().buffer.c_str());
}

void http3_sample_server::parse_ping(conn_io_req_res::header* path_header, struct conn_io_qh3* conn_io) {
	const char* const_logtag = logtag.c_str();
	const char* port_id_cstr = port_id.c_str();
	bool has_crc_header = conn_io->http_request->has_crc_header();
	if (has_crc_header) {
		bool validate = conn_io->http_request->validate();
		if (!validate) {
			qh3server::get_stats_loggeer()->server_count("parse", 1, "", "", "", "error", get_server_name(), path_header->value.c_str(), port_id_cstr, "crc_fail");
		}
		assert(validate);
	} else {
		// may be called from a browser
		debug_print_important2(const_logtag,
							   "May be '%s' requested from browser. So crc "
							   "validation not possible !!!",
							   path_header->value.c_str());
	}
	const char* res_string = "{\"pong\" : \"http3_sample_server\"}";
	conn_io->http_response->set_payload(qstring(res_string, strlen(res_string)));
	QH3_INFO(const_logtag, "%s - ping - %s", path_header->value.c_str(), res_string);
	qh3server::get_stats_loggeer()->server_count("parse", 1, "", "", "", "command", get_server_name(), has_crc_header ? "" : "no-crc", port_id_cstr, path_header->value.c_str());
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
		debug_print_important2(const_logtag,
							   "May be '%s' requested from browser. So crc "
							   "validation not possible !!!",
							   path_header->value.c_str());
	}
	qh3server::get_stats_loggeer()->server_count("parse", 1, "", "", "", "command", get_server_name(), has_crc_header ? "" : "no-crc", port_id_cstr, path_header->value.c_str());
	debug_print_important(const_logtag, "uv_stop called on main thread %s:%s", host_id.c_str(), port_id.c_str());
#if USE_UV_MAIN_LOOP
	uv_stop(conn_io->bridge->get_mainloop());
//	uv_run(conn_io->bridge->get_mainloop(), UV_RUN_ONCE);
#else
	ev_break(conn_io->bridge->get_mainloop(), EVBREAK_ONE);
#endif
}

void http3_sample_server::parse_whoami(conn_io_req_res::header* path_header, struct conn_io_qh3* conn_io) {
	const char* const_logtag = logtag.c_str();
	const char* port_id_cstr = port_id.c_str();
	bool has_crc_header = conn_io->http_request->has_crc_header();
	if (has_crc_header) {
		bool validate = conn_io->http_request->validate();
		if (!validate) {
			qh3server::get_stats_loggeer()->server_count("parse", 1, "", "", "", "error", get_server_name(), path_header->value.c_str(), port_id_cstr, "crc_fail");
		}
		assert(validate);
	} else {
		// may be called from a browser
		debug_print_important2(const_logtag,
							   "May be '%s' requested from browser. So crc "
							   "validation not possible !!!",
							   path_header->value.c_str());
	}
	char res_string[256];  // Allocate a buffer large enough to hold the final string
	memset(res_string, 0, sizeof(res_string));
	snprintf(res_string, sizeof(res_string), "{\"name\" : \"%s\", \"active_connections\" : %u}", get_server_name(), get_live_connection_count());

	conn_io->http_response->set_payload(qstring(res_string, strlen(res_string)));
	//	QH3_INFO(const_logtag, "%s - whoami - %s", path_header->value.c_str(), res_string);
	qh3server::get_stats_loggeer()->server_count("parse", 1, "", "", "", "command", get_server_name(), has_crc_header ? "" : "no-crc", port_id_cstr, path_header->value.c_str());
}

void http3_sample_server::parse_user_get(conn_io_req_res::header* path_header, struct conn_io_qh3* conn_io) {
	EV_START_RECORD(parse_start_time);
	const char* const_logtag = logtag.c_str();
	const char* port_id_cstr = port_id.c_str();
	const conn_io_req_res::payload& payload = conn_io->http_request->get_payload();
	bool validate = conn_io->http_request->validate();
	assert(validate);
	if (!validate) {
		qh3server::get_stats_loggeer()->server_count("parse", 1, "", "", "", "error", get_server_name(), path_header->value.c_str(), port_id_cstr, "crc_fail");
	}

	EV_PRINT_IF_ELAPSED_AND_CLEAR(parse_start_time, __LOGTAG__, "user_get : post validate t:%lu ms", 5);
	//	debug_print(LOG_LEVEL_2, __LOGTAG__, "%.*s", payload.buffer.length(), payload.buffer.c_str());
	rq_msg_user_get* user_get_msg_rq = msg_parser.parse<rq_msg_user_get>(payload.buffer.length(), (uint8_t*) payload.buffer.c_str());
	if (user_get_msg_rq == nullptr) {
		debug_print_error(__LOGTAG__, "Parse failed %.*s", payload.buffer.length(), payload.buffer.c_str());
		QH3_INFO(const_logtag, "ERROR - Parse failed %s, req:%.*s, returning. !!!", path_header->value.c_str(), payload.buffer.length(), payload.buffer.c_str());
		qh3server::get_stats_loggeer()->server_count("parse", 1, "", "", "", "error", get_server_name(), path_header->value.c_str(), port_id_cstr, "payload_deserialise_fail");
		return;
	}

	EV_PRINT_IF_ELAPSED_AND_CLEAR(parse_start_time, __LOGTAG__, "user_get : post  msg_parser.parse t:%lu ms", 5);
	unsigned long crc = crc32(0L, Z_NULL, 0);
//	crc = essentials::mod_crc32_z(crc, (uint8_t*) user_get_msg_rq->sys_name.c_str(), user_get_msg_rq->sys_name.length());
//	crc = essentials::mod_crc32_z(crc, (uint8_t*) user_get_msg_rq->node_name.c_str(), user_get_msg_rq->node_name.length());
//	crc = essentials::mod_crc32_z(crc, (uint8_t*) user_get_msg_rq->release.c_str(), user_get_msg_rq->release.length());
//  crc = essentials::mod_crc32_z(crc, (uint8_t*) user_get_msg_rq->locale.c_str(), user_get_msg_rq->locale.length());
	crc = essentials::mod_crc32_z(crc, (uint8_t*) user_get_msg_rq->arch.c_str(), user_get_msg_rq->arch.length());
    crc = essentials::mod_crc32_z(crc, (uint8_t*) user_get_msg_rq->duid.c_str(), user_get_msg_rq->duid.length());

	EV_PRINT_IF_ELAPSED_AND_CLEAR(parse_start_time, __LOGTAG__, "user_get : post crc calculation t:%lu ms", 5);
	qbuffer buffer;
	buffer.allocate(128);
	qbuffer_writer writer;
	writer.write(buffer, crc);
	debug_print(LOG_LEVEL_4, const_logtag, "%s - user id : %x", path_header->value.c_str(), crc);

	// response packet
	res_msg_user_get user_get_msg_respose;
	user_get_msg_respose.pid.format("%lx", crc);
	user_get_msg_respose.user_name.format("guest-%lx", crc);
	time_t last_login_utc_time_value;
	user_get_msg_respose.last_login = essentials::get_time_utc_readable(last_login_utc_time_value);
	writer.write(buffer, user_get_msg_respose.last_login);

	// check if redis has token
	qstring token_in_redis;
	qstring redis_format_pid(qstring::format_string("tokens:%s", user_get_msg_respose.pid.c_str()));
	hiredis->get_value(redis_format_pid, token_in_redis);
	if (token_in_redis.length() != 0) {
		user_get_msg_respose.token = token_in_redis;
		debug_print(LOG_LEVEL_4, __LOGTAG__, "token '%s' retreived from redis for user id : %x", user_get_msg_respose.token.c_str(), crc);
	} else {
		// calcualte sha
		crypto_helper::sha256_data sha_data((const char*) buffer.data, (int) buffer.index);
		crypto_helper::sha256(sha_data);
		// session token header
		user_get_msg_respose.token.bin_copy((const uint8_t*) sha_data.out, strlen(sha_data.out));
		debug_print(LOG_LEVEL_4, __LOGTAG__, "new token '%s' for user id : %x", user_get_msg_respose.token.c_str(), crc);
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
	int query_result = mongo->find_and_upsert(
		"users", [pid = user_get_msg_respose.pid](bson_t& find_query) { bson_append_utf8(&find_query, "user.pid", strlen("user.pid"), pid.c_str(), (int) pid.length()); },
		[last_login = user_get_msg_respose.last_login, last_login_utc_time_value](bson_t& update_query) {
			bson_append_utf8(&update_query, "user.last_login", strlen("user.last_login"), last_login.c_str(), (int) last_login.length());
			// Append the "last_login" as a time_t value (stored as an integer)
			bson_append_int64(&update_query, "user.last_login_timestamp", strlen("user.last_login_timestamp"), (int64_t) last_login_utc_time_value);
		},
		[pid = user_get_msg_respose.pid, user_name = user_get_msg_respose.user_name, user_get_msg_rq](bson_t& insert_query) {
			bson_append_utf8(&insert_query, "user.pid", strlen("user.pid"), pid.c_str(), (int) pid.length());
			bson_append_utf8(&insert_query, "user.name", strlen("user.name"), user_name.c_str(), (int) user_name.length());
			bson_append_utf8(&insert_query, "user.device.sys_name", strlen("user.device.sys_name"), user_get_msg_rq->sys_name.c_str(), (int) user_get_msg_rq->sys_name.length());
			bson_append_utf8(&insert_query, "user.device.node_name", strlen("user.device.node_name"), user_get_msg_rq->node_name.c_str(), (int) user_get_msg_rq->node_name.length());
			bson_append_utf8(&insert_query, "user.device.arch", strlen("user.device.arch"), user_get_msg_rq->arch.c_str(), (int) user_get_msg_rq->arch.length());
		});
	if (query_result == EXIT_SUCCESS) {
		user_get_msg_respose.room_list = DEBUG_NEW msg_room_config_list(*room_config_list);
		qstring response_json;
		user_get_msg_respose.get_json_string(response_json);
		conn_io->http_response->set_payload(response_json);
		//		QH3_INFO_WITH_PID(user_get_msg_respose.pid.c_str(), const_logtag, "user-last-login - %s", user_get_msg_respose.last_login.c_str());
	} else {
		QH3_INFO(const_logtag, "%s - user_get failed", path_header->value.c_str());
	}
	GX_DELETE(user_get_msg_rq);
	EV_PRINT_IF_ELAPSED_AND_CLEAR(parse_start_time, __LOGTAG__, "user_get : post mongo find_and_upsert user t:%lu ms", 50);

	qh3server::get_stats_loggeer()->server_count("parse", 1, "", "", "", "hit", get_server_name(), path_header->value.c_str(), port_id_cstr);
	debug_print(LOG_LEVEL_4, __LOGTAG__, "%s - FINISHED", __FUNCTION__);
}

int http3_sample_server::validate_token(conn_io_req_res::header* path_header, struct conn_io_qh3* conn_io) {
	const char* const_logtag = logtag.c_str();
	const char* port_id_cstr = port_id.c_str();
	const conn_io_req_res::payload& payload = conn_io->http_request->get_payload();
	bool validate = conn_io->http_request->validate();
	assert(validate);
	if (!validate) {
		qh3server::get_stats_loggeer()->server_count("validate_token", 1, "", "", "", "error", get_server_name(), path_header->value.c_str(), port_id_cstr, "crc_fail");
	}
	conn_io_req_res::header* token_header = conn_io->http_request->get_header("token");
	if (token_header == nullptr) {
		debug_print_error(const_logtag, "No token header in %s", path_header->value.c_str());
		QH3_INFO(const_logtag, "No token header in %s", path_header->value.c_str());
		qh3server::get_stats_loggeer()->server_count("validate_token", 1, "", "", "", "error", get_server_name(), path_header->value.c_str(), port_id_cstr, "no_token");
		return -1;
	}

	bson_t bson;
	bson_error_t error;
	if (!bson_init_from_json(&bson, payload.buffer.c_str(), payload.buffer.length(), &error)) {
		debug_print_error(const_logtag, "%s", error.message);
		QH3_INFO(const_logtag, "%s - ERROR - %s", path_header->value.c_str(), error.message);
		qh3server::get_stats_loggeer()->server_count("validate_token", 1, "", "", "", "error", get_server_name(), path_header->value.c_str(), port_id_cstr, "payload_deserialise_fail");
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
		debug_print_error(const_logtag, "user.pid parse failed %s", path_header->value.c_str());
		QH3_INFO(const_logtag, "user.pid parse failed %s", path_header->value.c_str());
		qh3server::get_stats_loggeer()->server_count("validate_token", 1, "", "", "", "error", get_server_name(), path_header->value.c_str(), port_id_cstr, "user.pid_parse_fail");
		return -3;
	}
	bson_destroy(&bson);

	// check with redis
	qstring token_in_redis;
	qstring redis_format_pid(qstring::format_string("tokens:%s", pid.c_str()));
	hiredis->get_value(redis_format_pid, token_in_redis);
	qh3server::get_stats_loggeer()->server_count("validate_token", 1, token_in_redis, pid, "", token_header->value, get_server_name(), path_header->value.c_str(), port_id_cstr, "user.token_check");
	if (token_in_redis != token_header->value) {
		debug_print(LOG_LEVEL_4, const_logtag, "NOT a Valid user %s != %s !!!", token_in_redis.c_str(), token_header->value.c_str());
		return -4;
	}

	debug_print(LOG_LEVEL_4, const_logtag, "Valid user");
	return 0;
}

void http3_sample_server::parse_user_details(conn_io_req_res::header* path_header, struct conn_io_qh3* conn_io) {
	int result = validate_token(path_header, conn_io);
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
	int result = validate_token(path_header, conn_io);
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
		debug_print(LOG_LEVEL_4, __LOGTAG__, "%s - %s:%s", key, field, value);
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
