#ifndef NAPI_GSRVERPLUGIN
#define NAPI_GSRVERPLUGIN

#include <napi.h>

namespace napi_gserver_funcs {
    Napi::Value napi_spawn_qserver(const Napi::CallbackInfo& info);
    Napi::Value napi_room_broadcast_except(const Napi::CallbackInfo& info);
    Napi::Value napi_room_broadcast(const Napi::CallbackInfo& info);
    Napi::Value napi_room_send_to(const Napi::CallbackInfo& info);
}
#endif
