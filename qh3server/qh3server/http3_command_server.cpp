//
//  Copyright 2024 homenet25
//  http3_command_server.cpp
//  qh3server
//
//  Created by Arun A on 31/10/23.
//

#include "http3_command_server.hpp"

#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using namespace rapidjson;
using namespace client;

http3_command_server::http3_command_server(const server_config_in& config) : qh3server(), bridge(config.ref), router_port(config.router_port) {
	UNUSED(bridge);
	hiredis = DEBUG_NEW qhiredis("cmd_server_hiredis", config.redis_ip, config.redis_port, "gsdkuser", "Fr0gmoon123");
}

http3_command_server::~http3_command_server() {
	GX_DELETE(hiredis);
}

void http3_command_server::on_run_started() {
	hiredis->set_hash_value(qstring::format_string("servers:%s", gsdk::device::public_ip), "command_center", qstring::format_string("%s:%s", host_id.c_str(), port_id.c_str()));
	check_and_update_is_log_quiche_flag();
}

bool http3_command_server::on_server_pre_init() {
	return hiredis->connect_redis() == 0;
}

void http3_command_server::on_server_uninitialise() {}

void http3_command_server::on_run_end() {}

bool http3_command_server::is_log_quiche() {
	return is_log_quiche_flag;
}

void http3_command_server::check_and_update_is_log_quiche_flag() {
	qstring is_log_quiche_value;
	hiredis->get_value("is_log_quiche", is_log_quiche_value);
	is_log_quiche_flag = is_log_quiche_value.compare("true") == 0;
}

float http3_command_server::get_router_hb_interval_in_sec() {
	return DEFAULT_TIMER_ROUTER_HB_INTERVAL_IN_SECONDS;
}

void http3_command_server::parse_header(const qstring& name, const qstring& value, struct conn_io_qh3* conn_io) {
	qh3server::parse_header(name, value, conn_io);
}

bridge_h3_connection::parse_return http3_command_server::parse(struct conn_io_qh3* conn_io) {
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

	// parse paths
	if (path_header->value.compare("/shutdown_test") == 0) {
		parse_shutdown_test(path_header, conn_io);
	}
	if (path_header->value.compare("/shutdown_cmd_center") == 0) {
		parse_shutdown_command_center(path_header, conn_io);
	} else if (path_header->value.compare("/whoami") == 0) {
		parse_whoami(path_header, conn_io);
	}
	return parse_sync;
}

void http3_command_server::parse_shutdown_command_center(conn_io_req_res::header* path_header, struct conn_io_qh3* conn_io) {
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

#if USE_UV_MAIN_LOOP
	uv_stop(conn_io->bridge->get_mainloop());
#else
	ev_break(conn_io->bridge->get_mainloop(), EVBREAK_ONE);
#endif
}

void http3_command_server::parse_shutdown_test(conn_io_req_res::header* path_header, struct conn_io_qh3* conn_io) {
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
	send_shutdown_to_all();
	conn_io->http_response->set_payload(qstring::format_string("{ %d-shutdown-all }", getpid()));
}

void http3_command_server::parse_whoami(conn_io_req_res::header* path_header, struct conn_io_qh3* conn_io) {
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
	qstring payload;  // = qstring::format_string("{\"name\" :
					  // \"%d-http3_command_server\"}", getpid());
	construct_response_whoami(payload);
	conn_io->http_response->set_payload(payload);
	//	QH3_INFO(const_logtag, "%s - whoami - %s", path_header->value.c_str(), payload.c_str());
	qh3server::get_stats_loggeer()->server_count("parse", 1, "", "", "", "command", get_server_name(), has_crc_header ? "" : "no-crc", port_id_cstr, path_header->value.c_str());
}

void http3_command_server::construct_response_whoami(qstring& response_string) {
	response_string.clear();
	Document doc;
	doc.SetObject();

	// Create a JSON object to hold the data
	Document::AllocatorType& allocator = doc.GetAllocator();
	// Add "name" key-value
	const qstring& server_id = qstring::format_string("%d-http3_command_server", getpid());
	doc.AddMember("name", Value().SetString(server_id.c_str(), allocator), allocator);
	Value servers(kArrayType);

	hiredis->iterate_hash(qstring::format_string("servers:%s", gsdk::device::public_ip), &doc, [&allocator, &servers](const char* field, const char* value, void* arg) {
		UNUSED(arg);
		Value server_obj(kObjectType);
		// Convert the key and value to `Value` type
		Value f(field, allocator);
		Value v(value, allocator);
		server_obj.AddMember(f, v, allocator);
		servers.PushBack(server_obj, allocator);
	});
	doc.AddMember("servers", servers, allocator);

	Value gservers(kArrayType);
	hiredis->iterate_hash("gservers", &doc, [&allocator, &gservers](const char* field, const char* value, void* arg) {
		UNUSED(arg);
		Value gserver_obj(kObjectType);
		// Convert the key and value to `Value` type
		Value f(field, allocator);
		Value v(value, allocator);
		gserver_obj.AddMember(f, v, allocator);
		gservers.PushBack(gserver_obj, allocator);
	});
	doc.AddMember("gservers", gservers, allocator);

	// Convert JSON document to string
	StringBuffer buffer;
	Writer<StringBuffer> writer(buffer);
	doc.Accept(writer);
	response_string = buffer.GetString();
}

void http3_command_server::send_shutdown_to_all() {
	conn_io_req_res* req = conn_io_req_res::create("/shutdown_test", "");
	qh3client_helper::send_async_request<client::qh3client>(
		host_id, router_port, req, nullptr,
		[](conn_io_req_res* request, conn_io_req_res* response, void* client_specific_data, void* arg, bool success) {
			UNUSED(response);
			UNUSED(client_specific_data);
			UNUSED(arg);
			UNUSED(success);
			debug_print(LOG_LEVEL_0, __LOGTAG__, "shutdown-return");
		},
		1);
}

void http3_command_server::command_feedback_recv_cb(EV_P_ ev_io* w, int revents) {
	UNUSED(loop);
	UNUSED(revents);
	route* route_client = (route*) w->data;
	bridge_command_center* bridge = (bridge_command_center*) route_client->arg;
	static uint8_t buf_r[65535];
	while (1) {
		struct sockaddr_storage peer_addr;
		socklen_t peer_addr_len = sizeof(peer_addr);
		memset(&peer_addr, 0, peer_addr_len);

		ssize_t read = recvfrom(route_client->bridge_sock, buf_r, sizeof(buf_r), 0, (struct sockaddr*) &peer_addr, &peer_addr_len);

		if (read < 0) {
			if ((errno == EWOULDBLOCK) || (errno == EAGAIN)) {
				debug_print(LOG_LEVEL_6, __LOGTAG__, "recv would block");
				break;
			}

			debug_print_error(__LOGTAG__, "failed to read");
			return;
		}

		qstring cmd(buf_r, read);
		debug_print(LOG_LEVEL_6, __LOGTAG__, "RECEIVED cmd feedback from client - %s !!!", cmd.c_str());
		bridge->cmd_feedback_from_client((struct sockaddr*) &peer_addr, cmd.c_str());
	}
}
