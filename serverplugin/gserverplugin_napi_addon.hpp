#ifndef NAPI_GSRVERPLUGIN
#define NAPI_GSRVERPLUGIN

#include <napi.h>

namespace napi_gserver_funcs {
    Napi::Value napi_spawn_qserver(const Napi::CallbackInfo& info);
}
#endif
