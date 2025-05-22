#ifndef NAPI_GSERVERPLUGIN
#define NAPI_GSERVERPLUGIN

#include <napi.h>

namespace napi_gserver_funcs {
    struct qserver_spawn_qserver_cb_data {
        napi_threadsafe_function pre_start_cb_ref;
        napi_threadsafe_function start_cb_ref;
        napi_threadsafe_function stop_cb_ref;
        napi_threadsafe_function error_cb_ref;
        napi_threadsafe_function room_event_create_cb_ref;
        napi_threadsafe_function room_event_start_cb_ref;
        napi_threadsafe_function room_event_player_added_cb_ref;
        napi_threadsafe_function room_event_message_cb_ref;
        napi_threadsafe_function room_player_removed_cb_ref;
        napi_threadsafe_function room_event_end_cb_ref;
        napi_threadsafe_function room_event_destroy_cb_ref;
        napi_threadsafe_function room_event_countdown_to_start_cb_ref;
        napi_threadsafe_function room_event_countdown_cancelled_cb_ref;
        
        void release(Napi::Env env) {
            napi_unref_threadsafe_function(env, pre_start_cb_ref);
            napi_unref_threadsafe_function(env, start_cb_ref);
            napi_unref_threadsafe_function(env, stop_cb_ref);
            napi_unref_threadsafe_function(env, error_cb_ref);
            napi_unref_threadsafe_function(env, room_event_create_cb_ref);
            napi_unref_threadsafe_function(env, room_event_start_cb_ref);
            napi_unref_threadsafe_function(env, room_event_player_added_cb_ref);
            napi_unref_threadsafe_function(env, room_event_message_cb_ref);
            napi_unref_threadsafe_function(env, room_player_removed_cb_ref);
            napi_unref_threadsafe_function(env, room_event_end_cb_ref);
            napi_unref_threadsafe_function(env, room_event_destroy_cb_ref);
            napi_unref_threadsafe_function(env, room_event_countdown_to_start_cb_ref);
            napi_unref_threadsafe_function(env, room_event_countdown_cancelled_cb_ref);
        }
    };

    Napi::Value napi_spawn_game_server(const Napi::CallbackInfo& info);
    Napi::Value napi_room_broadcast_except(const Napi::CallbackInfo& info);
    Napi::Value napi_room_broadcast(const Napi::CallbackInfo& info);
    Napi::Value napi_room_send_to(const Napi::CallbackInfo& info);
    Napi::Value napi_game_server_release_callbacks(const Napi::CallbackInfo& info);
    Napi::Value napi_qserver_stats_count(const Napi::CallbackInfo& info);
    Napi::Value napi_qserver_logfile(const Napi::CallbackInfo& info);
}
#endif
