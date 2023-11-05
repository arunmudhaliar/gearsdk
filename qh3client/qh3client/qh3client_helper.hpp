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

typedef std::function<void(std::vector<conn_io_response>* response)> type_qh3client_helper_cb;

class qh3client_helper {
public:
    struct qh3_req_obj {
        qh3_req_obj(const std::string host, const std::string port,
                       const getorpost_reqdata& data, std::vector<conn_io_response>* response) :
         host(host), port(port), data(data), response(response) {
        }
        std::string host;
        std::string port;
        getorpost_reqdata data;
        std::vector<conn_io_response>* response = nullptr;
        pthread_t run_thread_id;
        type_qh3client_helper_cb async_cb = nullptr;
    };
    
    static int send_request(const std::string host, const std::string port,
                            const getorpost_reqdata& data_getorpost_,
                            std::vector<conn_io_response>* response);
    static int send_async_request(const std::string host, const std::string port,
                            const getorpost_reqdata& data_getorpost_,
                            std::vector<conn_io_response>* response, type_qh3client_helper_cb);
    
private:
    static void *run_internal(void *data);
};
#endif /* qh3client_helper_hpp */
