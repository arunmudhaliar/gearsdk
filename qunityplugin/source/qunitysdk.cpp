//
//  qunityplugin.cpp
//  qunityplugin
//
//  Created by Arun A on 24/01/24.
//

#include "qunitysdk.hpp"

using namespace qunitysdk;
using namespace client;

extern "C" {
int send_async_request(const char* host, const char* port,
                                     const char* path, const char* payload, void* arg, type_qh3client_plugin_helper_cb callback) {
    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "host %s, port %s, path %s, payload %s", host, port, path, payload);
    return qh3client_helper::send_async_request<client::qh3client>(host, port, conn_io_req_res::create(path, payload), arg,
            [callback](conn_io_req_res *response, void* client_specific_data, void* arg) {
                const conn_io_req_res::payload &payload = response->data;
                callback(payload.buffer.c_str(), arg, 0);
                DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "payload %s", payload.buffer.c_str());
            });
}
}
