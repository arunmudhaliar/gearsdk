#ifndef NAPI_SERVERPLUGIN
#define NAPI_SERVERPLUGIN

#include <napi.h>

namespace napi_funcs {
    struct napi_qh3router_cb_data {
        napi_threadsafe_function pre_start_cb_ref;
        napi_threadsafe_function start_cb_ref;
        napi_threadsafe_function stop_cb_ref;
        napi_threadsafe_function error_cb_ref;
        
        void release(Napi::Env env) {
            napi_unref_threadsafe_function(env, pre_start_cb_ref);
            napi_unref_threadsafe_function(env, start_cb_ref);
            napi_unref_threadsafe_function(env, stop_cb_ref);
            napi_unref_threadsafe_function(env, error_cb_ref);
        }
    };

    struct napi_qh3server_cb_data {
        napi_threadsafe_function pre_start_cb_ref;
        napi_threadsafe_function start_cb_ref;
        napi_threadsafe_function stop_cb_ref;
        napi_threadsafe_function error_cb_ref;
        napi_threadsafe_function parse_cb_ref;
        
        void release(Napi::Env env) {
            napi_unref_threadsafe_function(env, pre_start_cb_ref);
            napi_unref_threadsafe_function(env, start_cb_ref);
            napi_unref_threadsafe_function(env, stop_cb_ref);
            napi_unref_threadsafe_function(env, error_cb_ref);
            napi_unref_threadsafe_function(env, parse_cb_ref);
        }
    };
}
#endif
