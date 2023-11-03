//
//  qh3client_helper.cpp
//  qh3client
//
//  Created by Arun A on 04/11/23.
//

#include "qh3client_helper.hpp"

int qh3client_helper::send_request(const std::string host, const std::string port,
                                   const getorpost_reqdata& data_getorpost_,
                                   std::vector<conn_io_response>* response) {
    qh3_req_obj* req_obj = new qh3_req_obj(host, port, data_getorpost_, response);
    if (pthread_create(&req_obj->run_thread_id, nullptr, qh3client_helper::run_internal, (void *)req_obj) < 0)
    {
        DEBUG_PRINT_ERROR(__LOGTAG__, "could not create thread: %s - %d", strerror(errno), errno);
        return -1;
    }
    pthread_join(req_obj->run_thread_id, nullptr);
    return 0;
}

void *qh3client_helper::run_internal(void *data) {
    qh3_req_obj* req_obj = (qh3_req_obj*)data;
    qh3client* new_client = new qh3client(req_obj->host, req_obj->port);
    new_client->send_request(req_obj->data, req_obj->response);
    GX_DELETE(new_client);
    GX_DELETE(req_obj);
    pthread_exit(0);
}

