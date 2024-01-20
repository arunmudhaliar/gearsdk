//
//  http3_sample_client.cpp
//  qh3client
//
//  Created by Arun A on 03/11/23.
//

#include "http3_sample_client.hpp"
#include <bson/bson.h>
#include "../../common/crypto_helper.hpp"
#include <zlib.h>

//std::atomic<int> http3_sample_client::live_connections = 0;
//std::atomic<int> http3_sample_client::total_connections_returned = 0;

http3_sample_client::http3_sample_client(const qstring& host, const qstring& port) :
    host(host), port(port) {
}

http3_sample_client::~http3_sample_client() {
}

void http3_sample_client::init_connection() {
    //    qh3client_helper::send_request(host, port, getorpost_reqdata("/whoami", "{}"),
    //        [this](conn_io_response* response) {
    //            if (response->responses.size()){
    //                DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__,"response %.*s", (int) response->responses[0]->len, response->responses[0]->buf);
    //            }
    //        });
        //user_get

    create_connections();

    keep_alive_loop = schedule_repeat_timer([this](qtimer& timer) {
        UNUSED(timer);
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "check client, issued %d, returned %d, returned success %d, live %d",
                              total_connections_issued.load(), total_connections_returned.load(),
                              total_connections_returned_success.load(), live_connections.load());
        if (live_connections<30) {
            DEBUG_PRINT_IMPORTANT(__LOGTAG__, "issue create_connections");
            create_connections();
            //shutdown_mainloop();
        }
        }, 3);
    //
    //    ev_run(loop, 0);
}

void http3_sample_client::create_connections() {
    qstring sys_name(essentials::get_sysname());
    qstring device_name(essentials::get_device_name());
    qstring arch(essentials::get_device_arch());
    qstring release_str(essentials::get_device_release_str());

    bson_t parent;
    bson_init(&parent);
    bson_t meta;
    bson_append_document_begin(&parent, "details", strlen("details"), &meta);
    bson_append_utf8(&meta, "sys_name", strlen("sys_name"), sys_name.c_str(), (int)sys_name.length());
    bson_append_utf8(&meta, "node_name", strlen("node_name"), device_name.c_str(), (int)device_name.length());
    bson_append_utf8(&meta, "release", strlen("release"), release_str.c_str(), (int)release_str.length());
    bson_append_utf8(&meta, "arch", strlen("arch"), arch.c_str(), (int)arch.length());
    bson_append_document_end(&parent, &meta);

    size_t length = 0;
    char* json_string_data = bson_as_json(&parent, &length);
    bson_destroy(&parent);

    for (int x = 0;x < 200;x++) {
        qh3client_helper::send_async_request(host, port, conn_io_req_res::create("/user_get", qstring(json_string_data, length)),
            [this, x](conn_io_req_res* response) {
                bool validate = response->validate();
//                assert(validate);
                if (!validate) {
                    //DEBUG_PRINT_ERROR(__LOGTAG__, "crc fail !!!");
                }
                conn_io_req_res::header* token_header = response->get_header("token");
                live_connections--;
                total_connections_returned++;
                if (token_header == nullptr) {
                    this->on_login_complete("", false);
                    return;
                }

                const conn_io_req_res::payload& payload = response->data;
                bson_t bson;
                bson_error_t error;
                if (!bson_init_from_json(&bson, payload.buffer.c_str(), payload.buffer.length(), &error)) {
                    DEBUG_PRINT_ERROR(__LOGTAG__, "%s", error.message);
                    return;
                }

                // parse
                bson_iter_t iter;
                bson_iter_t sub_iter;
                if (bson_iter_init(&iter, &bson) && bson_iter_find_descendant(&iter, "user.pid", &sub_iter)) {
                    pid = bson_iter_utf8(&sub_iter, NULL);
                }
                bson_destroy(&bson);

                total_connections_returned_success++;
                DEBUG_PRINT(LOG_LEVEL_4, __LOGTAG__, "async returned %d - %s !!!", x, token_header->value.c_str());

                session_token = token_header->value;
                this->on_login_complete(token_header->value, token_header->value.length() > 0);
            }, false);
        live_connections++;
        total_connections_issued++;
    }
    bson_free(json_string_data);
}

void http3_sample_client::on_login_complete(const qstring& token, bool result) {
    return;
    
    if (result == false) {
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "Login failed t:%s !!!", token.c_str());
        return;
    }

/*
    conn_io_req_res* req = conn_io_req_res::create("/shutdown-test");
    qh3client_helper::send_async_request(host, port, req,
        [](conn_io_req_res* response) {
            const conn_io_req_res::payload& payload = response->get_payload();
            DEBUG_PRINT_IMPORTANT(__LOGTAG__, "async C returned %s !!!", payload.buffer.c_str());
        });
*/

    bson_t parent;
    bson_init(&parent);
    bson_t meta;
    bson_append_document_begin (&parent, "user", strlen("user"), &meta);
    bson_append_utf8(&meta, "pid", strlen("pid"), pid.c_str(), (int)pid.length());
    bson_append_document_end (&parent, &meta);

    size_t length = 0;
    char* json_string_data = bson_as_json(&parent, &length);
    bson_destroy(&parent);

    conn_io_req_res* req = conn_io_req_res::create("/user_details", qstring(json_string_data, length));
    req->add_or_get_header("token", session_token);
    qh3client_helper::send_async_request(host, port, req,
        [this](conn_io_req_res* response) {
            live_connections--;
            total_connections_returned_success++;
            total_connections_returned++;
            const conn_io_req_res::payload& payload = response->get_payload();
            DEBUG_PRINT_IMPORTANT(__LOGTAG__, "async B returned %s !!!", payload.buffer.c_str());
        });
    live_connections++;
    total_connections_issued++;
    bson_free(json_string_data);
}
