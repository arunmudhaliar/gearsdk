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
    
    typedef void (*type_qsocket_onconnect)(unsigned long guid_crc);
    typedef void (*type_qsocket_onmessage)(unsigned long guid_crc, unsigned long recv_len, uint8_t* buf);
    typedef void (*type_qsocket_onreleaseconnection)(unsigned long guid_crc);
    typedef void (*type_qsocket_onclose)(unsigned long guid_crc);
    bool connect(const char* host, const char* port, void* arg,
                 type_qsocket_onconnect cb_connect, type_qsocket_onmessage cb_message,
                 type_qsocket_onreleaseconnection cb_release_connection, type_qsocket_onclose cb_close);
    void clear_callbacks();
    void set_guid_crc(unsigned long guid_crc)   {
        this->guid_crc = guid_crc;
    }
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
    unsigned long guid_crc = 0;
};

std::map<unsigned long, qsocket*> qsockets;

extern "C" {
    __attribute__((unused)) static void pre_init_sdk();

    typedef void (*type_qh3client_plugin_helper_cb)(const char* payload, void* arg, int result);
    __attribute__((unused)) static int send_async_request(const char* host, const char* port,
                                  const char* path, const char* payload, void* arg, type_qh3client_plugin_helper_cb callback);
    void destroy_qsocket(qsocket* qs);
    __attribute__((unused)) static bool qsocket_connect(unsigned long guid_crc, const char* host, const char* port, void* arg,
                             qsocket::type_qsocket_onconnect cb_connect, qsocket::type_qsocket_onmessage cb_message,
                             qsocket::type_qsocket_onreleaseconnection cb_release_connection, qsocket::type_qsocket_onclose cb_close);
    __attribute__((unused)) static bool qsocket_is_run_finished(unsigned long guid_crc);
    __attribute__((unused)) static int qsocket_sendMessage(unsigned long guid_crc, const char* buffer, unsigned long size, bool flush);
    __attribute__((unused)) static int qsocket_close(unsigned long guid_crc);

    __attribute__((unused)) static void destroy_finished_qsockets();

    __attribute__((unused)) static void qsocket_print_info();
    __attribute__((unused)) static unsigned long get_crc32(const char* guid, int guid_len);
}
};

#endif /* qunityplugin_hpp */
