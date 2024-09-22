//
//  Copyright 2024 homenet25
//  http3_sample_client.cpp
//  qh3client
//
//  Created by Arun A on 03/11/23.
//

#include "http3_sample_client.hpp"

#include "../../common/crypto_helper.hpp"

#include <bson/bson.h>
#include <zlib.h>

using namespace client;

http3_sample_client::http3_sample_client(const qstring& host, const qstring& port) : host(host), port(port) {}

http3_sample_client::~http3_sample_client() {}

void http3_sample_client::set_server_info(const qstring& host, const qstring& port) {
	this->host = host;
	this->port = port;
}

void http3_sample_client::create_connections(int count) {
	debug_print(LOG_LEVEL_0, __LOGTAG__, "create_connections");
	rq_msg_user_get user_get_msg_rq;

	user_get_msg_rq.sys_name = essentials::get_sysname();
	user_get_msg_rq.node_name = essentials::get_device_name();
	user_get_msg_rq.arch = essentials::get_device_arch();
	user_get_msg_rq.release = essentials::get_device_release_str();

	qstring json_str;
	user_get_msg_rq.get_json_string(json_str);

	for (int x = 0; x < count; x++) {
		qh3client_helper::send_async_request<client::qh3client>(
			host, port, conn_io_req_res::create("/user_get", json_str), nullptr,
			[this, x](conn_io_req_res* request, conn_io_req_res* response, void* client_specific_data, void* arg, bool success) {
				//				bool validate = response->validate();
				//				assert(validate);
				//				if (!validate) {
				//					// debug_print_error(__LOGTAG__, "crc fail !!!");
				//				}
				conn_io_req_res::header* token_header = response->get_header("token");
				if (token_header == nullptr) {
					this->on_login_complete("", false);
					return;
				}

				const conn_io_req_res::payload& payload = response->data;
				bson_t bson;
				bson_error_t error;
				if (!bson_init_from_json(&bson, payload.buffer.c_str(), payload.buffer.length(), &error)) {
					debug_print_error(__LOGTAG__, "%s", error.message);
					return;
				}

				// parse
				bson_iter_t iter;
				bson_iter_t sub_iter;
				if (bson_iter_init(&iter, &bson) && bson_iter_find_descendant(&iter, "pid", &sub_iter)) {
					pid = bson_iter_utf8(&sub_iter, NULL);
				}
				bson_destroy(&bson);

				total_connections_returned_success++;
				debug_print(LOG_LEVEL_0, __LOGTAG__, "async returned %d - %s !!!", x, token_header->value.c_str());

				session_token = token_header->value;
				this->on_login_complete(token_header->value, token_header->value.length() > 0);
			},
			1, CONNECTION_ESTABLISHMENT_TIMEOUT, [&](void* arg) { total_connections_returned++; });
		total_connections_issued++;
	}
}

void http3_sample_client::on_login_complete(const qstring& token, bool result) {
	if (result == false) {
		debug_print_important(__LOGTAG__, "Login failed t:%s !!!", token.c_str());
		return;
	}

	/*
		conn_io_req_res* req = conn_io_req_res::create("/shutdown-test");
		qh3client_helper::send_async_request(host, port, req,
			[](conn_io_req_res* response) {
				const conn_io_req_res::payload& payload = response->get_payload();
				debug_print_important(__LOGTAG__, "async C returned %s !!!", payload.buffer.c_str());
			});
	*/

	bson_t parent;
	bson_init(&parent);
	bson_t meta;
	bson_append_document_begin(&parent, "user", strlen("user"), &meta);
	bson_append_utf8(&meta, "pid", strlen("pid"), pid.c_str(), (int) pid.length());
	bson_append_document_end(&parent, &meta);

	size_t length = 0;
	char* json_string_data = bson_as_json(&parent, &length);
	bson_destroy(&parent);

	conn_io_req_res* req = conn_io_req_res::create("/user_details", qstring(json_string_data, length));
	req->add_or_get_header("token", session_token);
	qh3client_helper::send_async_request<client::qh3client>(
		host, port, req, nullptr,
		[this](conn_io_req_res* request, conn_io_req_res* response, void* client_specific_data, void* arg, bool success) {
			total_connections_returned_success++;
			const conn_io_req_res::payload& payload = response->get_payload();
			debug_print_important(__LOGTAG__, "async B returned %s !!!", payload.buffer.c_str());
		},
		1, CONNECTION_ESTABLISHMENT_TIMEOUT, [&](void* arg) { total_connections_returned++; });
	total_connections_issued++;
	bson_free(json_string_data);
}
