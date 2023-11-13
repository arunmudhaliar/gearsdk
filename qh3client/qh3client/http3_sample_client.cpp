//
//  http3_sample_client.cpp
//  qh3client
//
//  Created by Arun A on 03/11/23.
//

#include "http3_sample_client.hpp"
#include<sys/utsname.h>
#include <bson/bson.h>
#include "../../common/crypto_helper.hpp"
#include <zlib.h>

http3_sample_client::http3_sample_client() {
    
}

http3_sample_client::~http3_sample_client() {
}

void http3_sample_client::init_connection() {
    std::string host = "localhost";
//    std::string host = "192.168.0.230";
    std::string port = "4004";
//    qh3client_helper::send_request(host, port, getorpost_reqdata("/whoami", "{}"),
//        [this](conn_io_response* response) {
//            if (response->responses.size()){
//                DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__,"response %.*s", (int) response->responses[0]->len, response->responses[0]->buf);
//            }
//        });
    //user_get

    struct utsname device_details;
    errno =0;
    if(uname(&device_details)!=0)
    {
       perror("uname doesn't return 0, so there is an error");
       exit(EXIT_FAILURE);
    }
    
    bson_t parent;
    bson_init(&parent);
    bson_t meta;
    bson_append_document_begin (&parent, "details", strlen("details"), &meta);
    bson_append_utf8(&meta, "sys_name", strlen("sys_name"), device_details.sysname, (int)strlen(device_details.sysname));
    bson_append_utf8(&meta, "node_name", strlen("node_name"), device_details.nodename, (int)strlen(device_details.nodename));
    bson_append_utf8(&meta, "release", strlen("release"), device_details.release, (int)strlen(device_details.release));
    bson_append_utf8(&meta, "arch", strlen("arch"), device_details.machine, (int)strlen(device_details.machine));
    bson_append_document_end (&parent, &meta);
    
    size_t length = 0;
    char* json_string_data = bson_as_json(&parent, &length);
    bson_destroy(&parent);
        
    for (int x=0;x<1;x++) {
        qh3client_helper::send_async_request(host, port, getorpost_reqdata("/user_get", json_string_data),
                                    [this, x](conn_io_response* response) {
                                        conn_io_response::header *header = response->get_header((const uint8_t *)"token", strlen("token"));
                                        if (header == nullptr) {
                                            return;
                                        }
                                        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "async returned %d - %s !!!", x, header->value);
                                    });
    }
    bson_free(json_string_data);
    
    struct ev_loop* loop = ev_default_loop(0);
    ev_tstamp creation_time = ev_now(loop);
    set_ev_lopp(loop);
    keep_alive_loop = schedule_repeat_timer([loop, creation_time](qtimer& timer){
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "client alive - t:%5.2fs", ev_now(loop) - creation_time);
    }, 600);
    
    ev_run(loop, 0);
}
