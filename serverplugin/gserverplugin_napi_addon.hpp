#ifndef NAPI_GSERVERPLUGIN
#define NAPI_GSERVERPLUGIN

#include <napi.h>

namespace napi_gserver_funcs {
    struct qserver_spawn_qserver_cb_data {
        napi_threadsafe_function preStartCbRef;
        napi_threadsafe_function startCbRef;
        napi_threadsafe_function stopCbRef;
        napi_threadsafe_function errorCbRef;
        napi_threadsafe_function room_event_create_CbRef;
        napi_threadsafe_function room_event_start_CbRef;
        napi_threadsafe_function room_event_player_added_CbRef;
        napi_threadsafe_function room_event_message_CbRef;
        napi_threadsafe_function room_player_removed_CbRef;
        napi_threadsafe_function room_event_end_CbRef;
        napi_threadsafe_function room_event_countdown_to_start_CbRef;
        napi_threadsafe_function room_event_countdown_cancelled_CbRef;
        
        void release(Napi::Env env) {
            napi_unref_threadsafe_function(env, preStartCbRef);
            napi_unref_threadsafe_function(env, startCbRef);
            napi_unref_threadsafe_function(env, stopCbRef);
            napi_unref_threadsafe_function(env, errorCbRef);
            napi_unref_threadsafe_function(env, room_event_create_CbRef);
            napi_unref_threadsafe_function(env, room_event_start_CbRef);
            napi_unref_threadsafe_function(env, room_event_player_added_CbRef);
            napi_unref_threadsafe_function(env, room_event_message_CbRef);
            napi_unref_threadsafe_function(env, room_player_removed_CbRef);
            napi_unref_threadsafe_function(env, room_event_end_CbRef);
            napi_unref_threadsafe_function(env, room_event_countdown_to_start_CbRef);
            napi_unref_threadsafe_function(env, room_event_countdown_cancelled_CbRef);
        }
    };

    Napi::Value napi_spawn_qserver(const Napi::CallbackInfo& info);
    Napi::Value napi_room_broadcast_except(const Napi::CallbackInfo& info);
    Napi::Value napi_room_broadcast(const Napi::CallbackInfo& info);
    Napi::Value napi_room_send_to(const Napi::CallbackInfo& info);
    Napi::Value napi_qserver_release_callbacks(const Napi::CallbackInfo& info);
}
#endif
