//
//  qh3client_helper.hpp
//  qh3client
//
//  Created by Arun A on 04/11/23.
//

#ifndef qh3client_helper_hpp
#define qh3client_helper_hpp

#include "qh3client.hpp"
#include <pthread.h>
#include <functional>

typedef std::function<void(conn_io_req_res* response, void* client_specific_data, void* arg)> type_qh3client_helper_cb;

class qh3client_helper {
public:
    struct qh3_req_obj {
        qh3_req_obj(const qstring host, const qstring port,
            const conn_io_req_res* data) :
            host(host), port(port), data(data) {
        }
        ~qh3_req_obj() {
            GX_DELETE(data);
        }
        qstring host;
        qstring port;
        const conn_io_req_res* data = nullptr;
        pthread_t run_thread_id;
        type_qh3client_helper_cb async_cb = nullptr;
        void* arg = nullptr;
    };

    template<typename T> static int send_request(const qstring host, const qstring port,
        const conn_io_req_res* data_getorpost_, type_qh3client_helper_cb async_cb);
    template<typename T> static int send_async_request(const qstring host, const qstring port,
        const conn_io_req_res* data_getorpost_, void* arg, type_qh3client_helper_cb async_cb);

private:
    template<typename T> static void* run_internal(void* data);
};
#endif /* qh3client_helper_hpp */
