//
//  qunityplugin.hpp
//  qunityplugin
//
//  Created by Arun A on 24/01/24.
//

#ifndef qunityplugin_hpp
#define qunityplugin_hpp

#include "../../qh3client/qh3client/qh3client.hpp"
#include "../../qh3client/qh3client/qh3client_helper.hpp"
#include "../../qclient/source/qnetworkclient.hpp"

#include <map>
#include <algorithm>

namespace qunitysdk {
using namespace client;

class qsocket : public qnetworkclient {
public:
    typedef void (*type_qsocket_destroy_qsocket)(qsocket*);
    
    qsocket(type_qsocket_destroy_qsocket cb);
    virtual ~qsocket();
    
    typedef void (*type_qsocket_onconnect)();
    typedef void (*type_qsocket_onmessage)(unsigned long recv_len, uint8_t* buf);
    typedef void (*type_qsocket_onreleaseconnection)();
    typedef void (*type_qsocket_onclose)();
    bool connect(const char* host, const char* port, void* arg,
                 type_qsocket_onconnect cb_connect, type_qsocket_onmessage cb_message,
                 type_qsocket_onreleaseconnection cb_release_connection, type_qsocket_onclose cb_close);
    
protected:
    void onconnect(conn_io_client* qconnection) override;
    void onmessage(ssize_t recv_len, uint8_t* buf, conn_io_client* qconnection) override;
    void onreleaseconnection(conn_io_client* qconnection) override;
    void onclose(conn_io_client* qconnection) override;
    
private:
    type_qsocket_onconnect cb_connect = nullptr;
    type_qsocket_onmessage cb_message = nullptr;
    type_qsocket_onreleaseconnection cb_release_connection = nullptr;
    type_qsocket_onclose cb_close = nullptr;
    type_qsocket_destroy_qsocket cb_destroy_qsocket = nullptr;
};

std::map<unsigned long, qsocket*> qsockets;

extern "C" {
    typedef void (*type_qh3client_plugin_helper_cb)(const char* payload, void* arg, int result);
    static int send_async_request(const char* host, const char* port,
                                  const char* path, const char* payload, void* arg, type_qh3client_plugin_helper_cb callback);

    bool qsocket_connect(const char* guid, int guid_len, const char* host, const char* port, void* arg,
                         qsocket::type_qsocket_onconnect cb_connect, qsocket::type_qsocket_onmessage cb_message,
                         qsocket::type_qsocket_onreleaseconnection cb_release_connection, qsocket::type_qsocket_onclose cb_close);
    bool qsocket_is_run_finished(const char* guid, int guid_len);
    int qsocket_sendMessage(const char* guid, int guid_len, const char* buffer, unsigned long size, bool flush);

    void destroy_finished_qsockets();
    void destroy_qsocket(qsocket* qs);

}
};

#endif /* qunityplugin_hpp */
