//
//  qh3client_helper.cpp
//  qh3client
//
//  Created by Arun A on 04/11/23.
//

#include "qh3client_helper.hpp"

int qh3client_helper::send_request(const qstring host, const qstring port,
    const conn_io_req_res* data_getorpost_, type_qh3client_helper_cb async_cb) {
    qh3_req_obj* req_obj = DEBUG_NEW qh3_req_obj(host, port, data_getorpost_);
    req_obj->async_cb = async_cb;
    if (pthread_create(&req_obj->run_thread_id, nullptr, qh3client_helper::run_internal, (void*)req_obj) < 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "could not create thread: %s - %d", strerror(errno), errno);
        return -1;
    }
    pthread_join(req_obj->run_thread_id, nullptr);
    return 0;
}

int qh3client_helper::send_async_request(const qstring host, const qstring port,
    const conn_io_req_res* data_getorpost_, type_qh3client_helper_cb async_cb) {
    qh3_req_obj* req_obj = DEBUG_NEW qh3_req_obj(host, port, data_getorpost_);
    req_obj->async_cb = async_cb;
    if (pthread_create(&req_obj->run_thread_id, nullptr, qh3client_helper::run_internal, (void*)req_obj) < 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "could not create thread: %s - %d", strerror(errno), errno);
        return -1;
    }
    return 0;
}
//

void* qh3client_helper::run_internal(void* data) {
    qh3_req_obj* req_obj = (qh3_req_obj*)data;
    qh3client* new_client = DEBUG_NEW qh3client(req_obj->host, req_obj->port);
    new_client->send_request(req_obj->data);
    if (req_obj->async_cb && new_client->conn_io && new_client->conn_io->response) {
        req_obj->async_cb(new_client->conn_io->response);
    }
    GX_DELETE(new_client);
    GX_DELETE(req_obj);
    pthread_exit(0);
}



