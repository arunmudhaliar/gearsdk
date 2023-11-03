//
//  http3_sample_client.cpp
//  qh3client
//
//  Created by Arun A on 03/11/23.
//

#include "http3_sample_client.hpp"
#include<sys/utsname.h>
#include <bson/bson.h>

http3_sample_client::http3_sample_client() {
    
}

http3_sample_client::~http3_sample_client() {
}

void http3_sample_client::init_connection() {
//    send_request(getorpost_reqdata("/whoami", "{}"));
    std::string host = "localhost";
    std::string port = "4004";
    std::vector<conn_io_response> response;
    qh3client_helper::send_request(host, port, getorpost_reqdata("/whoami", "{}"), &response);
    //user_get
    
    struct utsname device_details;
    errno =0;
    if(uname(&device_details)!=0)
    {
       perror("uname doesn't return 0, so there is an error");
       exit(EXIT_FAILURE);
    }
    
    bson_t parent;// = BSON_INITIALIZER;
    bson_init(&parent);
    bson_t meta;
    bson_append_document_begin (&parent, "details", strlen("details"), &meta);
    bson_append_utf8(&meta, "sys_name", strlen("sys_name"), device_details.sysname, (int)strlen(device_details.sysname));
    bson_append_utf8(&meta, "node_name", strlen("node_name"), device_details.nodename, (int)strlen(device_details.nodename));
    bson_append_utf8(&meta, "release", strlen("release"), device_details.release, (int)strlen(device_details.release));
    bson_append_utf8(&meta, "arch", strlen("arch"), device_details.machine, (int)strlen(device_details.machine));
    bson_append_document_end (&parent, &meta);
    
    char* json_string = bson_as_json(&parent, nullptr);
    qh3client_helper::send_request(host, port, getorpost_reqdata("/user_get", json_string), &response);
    bson_free(json_string);
    bson_destroy(&parent);
}
