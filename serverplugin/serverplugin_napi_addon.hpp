#ifndef NAPI_SERVERPLUGIN
#define NAPI_SERVERPLUGIN

#include <napi.h>

namespace napi_funcs {
    struct qh3router_spawn_qh3router_cb_data {
        napi_threadsafe_function preStartCbRef;
        napi_threadsafe_function startCbRef;
        napi_threadsafe_function stopCbRef;
        napi_threadsafe_function errorCbRef;
        
        void release(Napi::Env env) {
            napi_unref_threadsafe_function(env, preStartCbRef);
            napi_unref_threadsafe_function(env, startCbRef);
            napi_unref_threadsafe_function(env, stopCbRef);
            napi_unref_threadsafe_function(env, errorCbRef);
        }
    };

    struct qh3server_spawn_qh3server_cb_data {
        napi_threadsafe_function preStartCbRef;
        napi_threadsafe_function startCbRef;
        napi_threadsafe_function stopCbRef;
        napi_threadsafe_function errorCbRef;
        napi_threadsafe_function parseCbRef;
        
        void release(Napi::Env env) {
            napi_unref_threadsafe_function(env, preStartCbRef);
            napi_unref_threadsafe_function(env, startCbRef);
            napi_unref_threadsafe_function(env, stopCbRef);
            napi_unref_threadsafe_function(env, errorCbRef);
            napi_unref_threadsafe_function(env, parseCbRef);
        }
    };
}
#endif
