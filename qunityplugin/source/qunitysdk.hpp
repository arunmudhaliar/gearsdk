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

namespace qunitysdk {
extern "C" {
    typedef void (*type_qh3client_plugin_helper_cb)(const char* payload, void* arg, int result);
    static int send_async_request(const char* host, const char* port,
                                  const char* path, const char* payload, void* arg, type_qh3client_plugin_helper_cb callback);
}
};

#endif /* qunityplugin_hpp */
