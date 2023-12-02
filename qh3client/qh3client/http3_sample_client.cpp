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

int http3_sample_client::live_connections = 0;
int http3_sample_client::total_connections_returned = 0;

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
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "check client");
        if (live_connections<50) {
            DEBUG_PRINT_IMPORTANT(__LOGTAG__, "issue create_connections %d", total_connections_returned);
            create_connections();
        }
    }, 5);
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
    bson_append_document_begin (&parent, "details", strlen("details"), &meta);
    bson_append_utf8(&meta, "sys_name", strlen("sys_name"), sys_name.c_str(), (int)sys_name.length());
    bson_append_utf8(&meta, "node_name", strlen("node_name"), device_name.c_str(), (int)device_name.length());
    bson_append_utf8(&meta, "release", strlen("release"), release_str.c_str(), (int)release_str.length());
    bson_append_utf8(&meta, "arch", strlen("arch"), arch.c_str(), (int)arch.length());
    bson_append_document_end (&parent, &meta);
    
    size_t length = 0;
    char* json_string_data = bson_as_json(&parent, &length);
    bson_destroy(&parent);

    for (int x=0;x<500;x++) {
        qh3client_helper::send_async_request(host, port, conn_io_req_res::create("/user_get", qstring(json_string_data)),
                                    [this, x](conn_io_req_res* response) {
                                        conn_io_req_res::header *header = response->get_header("token");
                                        if (header == nullptr) {
                                            this->on_login_complete("", false);
                                            return;
                                        }
                                        
                                        live_connections--;
                                        total_connections_returned++;
                                        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "async returned %d - %s !!!", x, header->value.c_str());
            
                                        //this->on_login_complete(header->value, true);
                                    });
        live_connections++;
    }
    bson_free(json_string_data);
}

void http3_sample_client::on_login_complete(const qstring& token, bool result) {
    if (result == false) {
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "Login failed !!!");
        return;
    }
    session_token = token;
    
//    bson_t parent;
//    bson_init(&parent);
//    bson_t meta;
//    bson_append_document_begin (&parent, "details", strlen("details"), &meta);
//    bson_append_utf8(&meta, "sys_name", strlen("sys_name"), sys_name.c_str(), (int)sys_name.length());
//    bson_append_utf8(&meta, "node_name", strlen("node_name"), device_name.c_str(), (int)device_name.length());
//    bson_append_utf8(&meta, "release", strlen("release"), release_str.c_str(), (int)release_str.length());
//    bson_append_utf8(&meta, "arch", strlen("arch"), arch.c_str(), (int)arch.length());
//    bson_append_document_end (&parent, &meta);
//
//    size_t length = 0;
//    char* json_string_data = bson_as_json(&parent, &length);
//    bson_destroy(&parent);
    
    qh3client_helper::send_async_request(host, port, conn_io_req_res::create("/user_details"),
        [this](conn_io_req_res* response) {
            conn_io_req_res::header *header = response->get_header("token");
            if (header == nullptr) {
                return;
            }
            
            live_connections--;
            total_connections_returned++;
            DEBUG_PRINT_IMPORTANT(__LOGTAG__, "async 2 returned %d - %s !!!", header->value.c_str());

            this->on_login_complete(header->value, true);
        });
    live_connections++;
}
