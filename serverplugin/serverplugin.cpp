//
//  serverplugin.cpp
//  qh3server
//
//  Created by Arun A on 22/12/24.
//

#include "serverplugin.h"

#include "../common/signal_handler/signal_handler.hpp"
#include "../servercommon/source/servercommon.hpp"
#include "../qh3server/qh3server/http3_command_server.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "serverplugin"

// qh3plugin_router_event_listener
void gsdk::server::qh3plugin_router_event_listener::on_router_pre_start(qh3router* router) {
	if (cb_on_router_pre_start) {
		cb_on_router_pre_start(router, router->get_server_config().user_arg);
	}
}

void gsdk::server::qh3plugin_router_event_listener::on_router_start(qh3router* router) {
	if (cb_on_router_start) {
		cb_on_router_start(router, router->get_server_config().user_arg);
	}
}

void gsdk::server::qh3plugin_router_event_listener::on_router_stop(qh3router* router) {
	if (cb_on_router_stop) {
		cb_on_router_stop(router, router->get_server_config().user_arg);
	}
}

void gsdk::server::qh3plugin_router_event_listener::on_router_error(qh3router* router, int error_code) {
	if (cb_on_router_error) {
		cb_on_router_error(router, router->get_server_config().user_arg, error_code);
	}
}

// qh3plugin_router_event_listener
void gsdk::server::qh3plugin_server_event_listener::on_server_pre_start(qh3server* server) {
	if (cb_on_server_pre_start) {
		cb_on_server_pre_start(server);
	}
}

void gsdk::server::qh3plugin_server_event_listener::on_server_start(qh3server* server, const char* ip, uint16_t port) {
	if (cb_on_server_start) {
		cb_on_server_start(server, ip, port);
	}
}

void gsdk::server::qh3plugin_server_event_listener::on_server_stop(qh3server* server) {
	if (cb_on_server_stop) {
		cb_on_server_stop(server);
	}
}

void gsdk::server::qh3plugin_server_event_listener::on_server_error(qh3server* server, int error_code) {
	if (cb_on_server_error) {
		cb_on_server_error(server, error_code);
	}
}

void gsdk::server::qh3plugin_server_event_listener::on_serevr_parse(qh3server* server, const conn_io_qh3* conn, const char* path, const char* buffer, unsigned long len, const char* headers_buffer, unsigned long headers_buffer_size) {
	if (cb_on_server_parse) {
		cb_on_server_parse(server, (uint8_t *)conn->cid, sizeof(conn->cid), path, buffer, len, headers_buffer, headers_buffer_size);
	}
}

// qh3plugin_server
gsdk::server::qh3plugin_server::qh3plugin_server(const server_config_in& config) : qh3server() {}

void gsdk::server::qh3plugin_server::parse_header(const qstring& name, const qstring& value, struct conn_io_qh3* conn_io) {
	qh3server::parse_header(name, value, conn_io);
}

bridge_h3_connection::parse_return gsdk::server::qh3plugin_server::parse(struct conn_io_qh3* conn_io) {
	const char* const_logtag = logtag.c_str();
	const char* port_id_cstr = port_id.c_str();
	conn_io_req_res::header* path_header = conn_io->http_request->get_header(":path");
	if (path_header == nullptr) {
		debug_print_error(const_logtag, "path_header == null, returning. !!!");
		qh3server::get_stats_loggeer()->server_count("parse", 1, "", "", "", "error", get_server_name(), "", port_id_cstr, "path_not_found");
		return parse_sync;
	}

	if (path_header->value.length() <= 1) {
		debug_print_warn(const_logtag, "path is very short - %s, returning. !!!", path_header->value.c_str());
		qh3server::get_stats_loggeer()->server_count("parse", 1, "", "", "", "warn", get_server_name(), path_header->value.c_str(), port_id_cstr, "short_path");
		return parse_sync;
	}

	if (server_event_observer) {
		qstring headers_json_string;
		conn_io->http_request->headers_to_json(headers_json_string);
		server_event_observer->on_serevr_parse(this, conn_io, path_header->value.c_str(), conn_io->http_request->get_payload().buffer.c_str(), conn_io->http_request->get_payload().buffer.length(), headers_json_string.c_str(),
											   headers_json_string.length());
		return parse_async;
		//		if (return_buffer) {
		//			conn_io->http_response->set_payload(return_buffer);
		//			debug_print(LOG_LEVEL_0, __LOGTAG__, "response buffer - %s", return_buffer);
		//			free((void*) return_buffer);
		//			return_buffer = nullptr;
		//		}
	}
	return parse_sync;
}

bool gsdk::server::qh3plugin_server::on_server_pre_init() {
	return true;
}

void gsdk::server::qh3plugin_server::on_server_uninitialise() {}

void gsdk::server::qh3plugin_server::on_run_started() {}

void gsdk::server::qh3plugin_server::on_run_end() {}

EXPORT void gsdk::server::setup_signal_handler() {
	signal_handler::setup_signal_handler();
}

EXPORT void gsdk::server::pre_init_serverplugin_sdk() {
	init_gsdk();
	gsdk::servercommon::init_server_common();
}

EXPORT void gsdk::server::spawn_qh3router(const char* router_address, const char* mongodb_uri, const char* redis_address, const char* zk_uri, const char* root_dir, uint16_t command_port, uint16_t router_port_return, const char* app_id,
										  qh3plugin_router_event_listener::type_on_router_pre_start pre_start_cb, qh3plugin_router_event_listener::type_on_router_start start_cb, qh3plugin_router_event_listener::type_on_router_stop stop_cb,
										  qh3plugin_router_event_listener::type_on_router_error error_cb, void* user_arg) {
	qaddress router_addr(router_address);
	qaddress redis_addr(redis_address);
	server_config_in* config = DEBUG_NEW server_config_in(router_addr.ip, qstring::format_string("%d", router_addr.port), mongodb_uri, redis_addr.ip, redis_addr.port, fs::path(root_dir), nullptr, command_port, router_addr.port, zk_uri,
														  router_port_return, app_id, user_arg);
	qh3plugin_router_event_listener* listener = DEBUG_NEW qh3plugin_router_event_listener(pre_start_cb, start_cb, stop_cb, error_cb);
	std::tuple<server_config_in*, qh3plugin_router_event_listener*>* tuple_in = DEBUG_NEW std::tuple<server_config_in*, qh3plugin_router_event_listener*>(config, listener);
	if (pthread_create(&config->run_thread_id, nullptr, gsdk::server::spawn_qh3router_internal, (void*) tuple_in) < 0) {
		debug_print_error(__LOGTAG__, "spawn_qh3router - could not create thread: %s - %d", strerror(errno), errno);
		GX_DELETE(tuple_in);
		GX_DELETE(config);
		GX_DELETE(listener);
	}
}
void* gsdk::server::spawn_qh3router_internal(void* data) {
	std::tuple<server_config_in*, qh3plugin_router_event_listener*>* tuple_in = (std::tuple<server_config_in*, qh3plugin_router_event_listener*>*) data;
	server_config_in* config = (server_config_in*) std::get<0>(*tuple_in);
	qh3plugin_router_event_listener* listener = (qh3plugin_router_event_listener*) std::get<1>(*tuple_in);

	config->print();
	http3_sample_router router(*config);
	router.run<http3_command_server, qh3plugin_server>(qh3router::qh3router_run_flag::SKIP_DEFAULT_QH3SERVER_SPAWN, listener);
	GX_DELETE(tuple_in);
	GX_DELETE(listener);
	GX_DELETE(config);
	pthread_exit(0);
}

void* gsdk::server::spawn_qh3server_internal(void* data) {
	std::tuple<server_config_in*, observer_qh3server_events*>* tuple_in = (std::tuple<server_config_in*, observer_qh3server_events*>*) data;
	server_config_in* config = (server_config_in*) std::get<0>(*tuple_in);
	qh3plugin_server_event_listener* listener = (qh3plugin_server_event_listener*) std::get<1>(*tuple_in);
	port_range range;
	int index = 0;
	int free_port = qh3router::next_available_port(config->host, range, index);
	if (free_port == 0) {
		debug_print_error(__LOGTAG__, "NO PORT AVAILABLE !!!");
		GX_DELETE(tuple_in);
		GX_DELETE(listener);
		GX_DELETE(config);
		pthread_exit(0);
	}
	PTHREAD_NAME(qh3plugin_server::get_server_name());
	qh3plugin_server* new_server = DEBUG_NEW qh3plugin_server(*config);
	new_server->run(config->host, qstring::format_string("%d", free_port), config->root_dir, config->router, config->command_feedback_port, config->router_port_return, config->app_id, listener);
	GX_DELETE(new_server);
	GX_DELETE(tuple_in);
	GX_DELETE(listener);
	GX_DELETE(config);
	pthread_exit(0);
}

EXPORT void gsdk::server::spawn_qh3server(qh3router* router, const char* server_address, const char* mongodb_uri, const char* redis_address, const char* zk_uri, const char* root_dir, uint16_t command_port, uint16_t router_port_return,
										  const char* app_id, qh3plugin_server_event_listener::type_on_server_pre_start pre_start_cb, qh3plugin_server_event_listener::type_on_server_start start_cb,
										  qh3plugin_server_event_listener::type_on_server_stop stop_cb, qh3plugin_server_event_listener::type_on_server_error error_cb, qh3plugin_server_event_listener::type_on_server_parse parse_cb) {
	qaddress server_addr(server_address);
	qaddress redis_addr(redis_address);
	server_config_in* config = new server_config_in(server_addr.ip, qstring::format_string("%d", server_addr.port), mongodb_uri, redis_addr.ip, redis_addr.port, fs::path(root_dir), (router) ? router->get_router_addrinfo() : nullptr,
													command_port, server_addr.port, zk_uri, router_port_return, app_id);

	qh3plugin_server_event_listener* listener = DEBUG_NEW qh3plugin_server_event_listener(pre_start_cb, start_cb, stop_cb, error_cb, parse_cb);
	std::tuple<server_config_in*, qh3plugin_server_event_listener*>* tuple_in = DEBUG_NEW std::tuple<server_config_in*, qh3plugin_server_event_listener*>(config, listener);
	if (pthread_create(&config->run_thread_id, nullptr, gsdk::server::spawn_qh3server_internal, (void*) tuple_in) < 0) {
		debug_print_error(__LOGTAG__, "spawn_qh3server - could not create thread: %s - %d", strerror(errno), errno);
		GX_DELETE(tuple_in);
		GX_DELETE(config);
		GX_DELETE(listener);
	}

	/*
	bool fork_result = false;  // only valid inside FORK_QH3_SERVER preprocessor
	pid_t parent_process_id = getpid();
	pid_t child_process_id = -1;
	qh3plugin_server_event_listener* listener = DEBUG_NEW qh3plugin_server_event_listener(pre_start_cb, start_cb, stop_cb, error_cb);
	int result = qh3router::spawn_qh3server<qh3plugin_server>(config.host, qstring::format_string("%d", free_port), config, child_process_id, fork_result, router, listener);
#if FORK_QH3_SERVER
	if (result != 0) {
		debug_print_error(__LOGTAG__, "spawn_qh3server failed. returning !!!");
		return;
	}
	if (!fork_result) {
		if (parent_process_id == getpid()) {
			debug_print_error(__LOGTAG__, "spawn_qh3server forking failed. rturning !!!", parent_process_id);
			return;
		}
	}
#else
	if (result != 0) {
		debug_print_error(__LOGTAG__, "spawn_qh3server failed. !!!");
	}
#endif
	 */
}

EXPORT void gsdk::server::qh3server_try_send_response(qh3server* server, uint8_t *cid, uint16_t cid_len, const char* payload, size_t len, const char* user_data, size_t user_data_len) {
    struct conn_io_qh3* conn_io = server->get_conn(cid, cid_len);
    if (conn_io != nullptr) {
        conn_io->http_response->set_payload(qstring(payload, len));
        server->try_send_response(conn_io);
    }
}

EXPORT unsigned int gsdk::server::get_live_connection_count(qh3server* server) {
	return server->get_live_connection_count();
}

EXPORT unsigned long gsdk::server::get_crc32(const char* guid, int guid_len) {
	if (guid == nullptr) {
		debug_warn(LOG_LEVEL_0, __LOGTAG__, "get_crc32 failed - guid is null");
		return 0;
	}
	return essentials::get_crc((const uint8_t*) guid, guid_len);
}

EXPORT unsigned long gsdk::server::mod_crc32(uLong adler, const Bytef* buf, z_size_t len) {
	if (buf == nullptr) {
		return 0;
	}
	return essentials::mod_crc32_z(adler, buf, len);
}

EXPORT const char* gsdk::server::get_device_public_ip() {
	return gsdk::device::public_ip;
}

EXPORT uint64_t gsdk::server::qh3server_logfile(qh3server* server, qlogfile::log_lvls lvl, qcustomlogger::elog_type type, const char* tag, const char* pid, const char* roomid, const char* message) {
    return server->get_file_logger()->log(lvl, type, tag, pid, roomid, message);
}

EXPORT size_t gsdk::server::qh3server_stats_count(qh3server* server, const char* counter, long count_val, const char* session, const char* pid, const char* version, const char* epic, const char* myth, const char* legend,
                           const char* story, const char* message) {
    return server->get_stats_loggeer()->server_count(counter, count_val, session, pid, version, epic, myth, legend, story, message);
}
