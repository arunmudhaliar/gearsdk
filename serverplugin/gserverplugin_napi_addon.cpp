#include "napi_helper.hpp"
#include "serverplugin.h"
#include "plugin_gameserver.hpp"
#include "gserverplugin_napi_addon.hpp"

namespace napi_gserver_funcs {
Napi::Value napi_spawn_qserver(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    /*
     int spawn_qserver(const char* server_address, const char* redis_address, const char* zk_uri, const char* root_dir, const char* app_id, qplugin_qserver_event_listener::type_on_qserver_pre_start pre_start_cb, qplugin_qserver_event_listener::type_on_qserver_start start_cb, qplugin_qserver_event_listener::type_on_qserver_stop stop_cb,
                              qplugin_qserver_event_listener::type_on_qserver_error error_cb,
                              qplugin_qserver_event_listener::type_room_event_create room_event_create_cb,
                              qplugin_qserver_event_listener::type_room_event_start room_event_start_cb,
                              qplugin_qserver_event_listener::type_room_event_player_added room_event_player_added_cb,
                              qplugin_qserver_event_listener::type_room_event_message room_event_message_cb,
                              qplugin_qserver_event_listener::type_room_event_player_removed room_player_removed_cb,
                              qplugin_qserver_event_listener::type_room_event_end room_event_end_cb,
                              qplugin_qserver_event_listener::type_room_event_countdown_to_start room_event_countdown_to_start_cb,
                              qplugin_qserver_event_listener::type_room_event_countdown_cancelled room_event_countdown_cancelled_cb);
     */
      // Validate arguments
    if (info.Length() < 18 ||
        !info[0].IsString() || !info[1].IsString() || !info[2].IsString() || !info[3].IsString() || !info[4].IsString() || !info[5].IsString() ||
        !info[6].IsFunction() || !info[7].IsFunction() || !info[8].IsFunction() || !info[9].IsFunction() ||
        !info[10].IsFunction() || !info[11].IsFunction() || !info[12].IsFunction() || !info[13].IsFunction() ||
        !info[14].IsFunction() || !info[15].IsFunction() || !info[16].IsFunction() || !info[17].IsFunction()
        ) {
        Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    // Extract arguments
    std::string server_address = info[0].As<Napi::String>();
    std::string redisAddress = info[1].As<Napi::String>();
    std::string zkUri = info[2].As<Napi::String>();
    std::string rootDir = info[3].As<Napi::String>();
    std::string inf_file = info[4].As<Napi::String>();
    std::string appId = info[5].As<Napi::String>();
    
    struct CallbackPayload {
        short type = 0;
        qnetworkserver* server = nullptr;
        qstring ip = "";
        uint16_t port = 0;
        int error_code = 0;
        int room = -1;
        class room* room_ptr = nullptr;
        qstring pid = nullptr;
        unsigned cid_hash = 0;
        qstring msg = nullptr;
        int count = 0;
        int max_count = 0;
    };

    auto return_cb = [](napi_env env, napi_value jsCallback, void* context, void* data) {
        UNUSED(context);
        // Convert the payload to CallbackPayload and extract pointers
        CallbackPayload* payload = static_cast<CallbackPayload*>(data);
        // Ensure proper scope
        Napi::HandleScope scope(env);
        Napi::Function jsFunc = Napi::Function(env, jsCallback);

        // Pass the pointers to the JS function
        switch (payload->type) {
        case 1:
        case 3:
            jsFunc.Call({
                Napi::External<qnetworkserver>::New(env, payload->server)
                });
            break;
        case 2:
            jsFunc.Call({
                Napi::External<qnetworkserver>::New(env, payload->server),
                Napi::String::New(env, payload->ip.c_str()),
                Napi::Number::New(env, payload->port)
                });
            break;
        case 4:
            jsFunc.Call({
                Napi::External<qnetworkserver>::New(env, payload->server),
                Napi::Number::New(env, payload->error_code)
                });
            break;
        case 5:
        case 6:
        case 10:
        case 12:
            jsFunc.Call({
                Napi::External<qnetworkserver>::New(env, payload->server),
                Napi::Number::New(env, payload->room),
                Napi::External<void>::New(env, payload->room_ptr)
                });
            break;
        case 7:
        case 9:
            jsFunc.Call({
                Napi::External<qnetworkserver>::New(env, payload->server),
                Napi::Number::New(env, payload->room),
                Napi::External<void>::New(env, payload->room_ptr),
                Napi::String::New(env, payload->pid.c_str()),
                Napi::Number::New(env, payload->cid_hash)
                });
            break;
        case 8:
            jsFunc.Call({
                Napi::External<qnetworkserver>::New(env, payload->server),
                Napi::Number::New(env, payload->room),
                Napi::External<void>::New(env, payload->room_ptr),
                Napi::String::New(env, payload->pid.c_str()),
                Napi::Number::New(env, payload->cid_hash),
                Napi::String::New(env, payload->msg.c_str())
                });
            break;
        case 11:
            jsFunc.Call({
                Napi::External<qnetworkserver>::New(env, payload->server),
                Napi::Number::New(env, payload->room),
                Napi::External<void>::New(env, payload->room_ptr),
                Napi::Number::New(env, payload->count),
                Napi::Number::New(env, payload->max_count)
                });
            break;
        }
        // Free the payload memory
        GX_DELETE(payload);
        };

    // Extract and persist JavaScript callbacks
    qserver_spawn_qserver_cb_data* callbackData = DEBUG_NEW qserver_spawn_qserver_cb_data();
    callbackData->preStartCbRef = napi_funcs::create_threadsafe_func(env, info[6].As<Napi::Function>(), "PreStart Callback", return_cb);
    callbackData->startCbRef = napi_funcs::create_threadsafe_func(env, info[7].As<Napi::Function>(), "Start Callback", return_cb);
    callbackData->stopCbRef = napi_funcs::create_threadsafe_func(env, info[8].As<Napi::Function>(), "Stop Callback", return_cb);
    callbackData->errorCbRef = napi_funcs::create_threadsafe_func(env, info[9].As<Napi::Function>(), "Error Callback", return_cb);
    callbackData->room_event_create_CbRef = napi_funcs::create_threadsafe_func(env, info[10].As<Napi::Function>(), "room_event_create Callback", return_cb);
    callbackData->room_event_start_CbRef = napi_funcs::create_threadsafe_func(env, info[11].As<Napi::Function>(), "room_event_start Callback", return_cb);
    callbackData->room_event_player_added_CbRef = napi_funcs::create_threadsafe_func(env, info[12].As<Napi::Function>(), "room_event_player_added Callback", return_cb);
    callbackData->room_event_message_CbRef = napi_funcs::create_threadsafe_func(env, info[13].As<Napi::Function>(), "room_event_message Callback", return_cb);
    callbackData->room_player_removed_CbRef = napi_funcs::create_threadsafe_func(env, info[14].As<Napi::Function>(), "room_player_removed Callback", return_cb);
    callbackData->room_event_end_CbRef = napi_funcs::create_threadsafe_func(env, info[15].As<Napi::Function>(), "room_event_end Callback", return_cb);
    callbackData->room_event_countdown_to_start_CbRef = napi_funcs::create_threadsafe_func(env, info[16].As<Napi::Function>(), "room_event_countdown_to_start Callback", return_cb);
    callbackData->room_event_countdown_cancelled_CbRef = napi_funcs::create_threadsafe_func(env, info[17].As<Napi::Function>(), "room_event_countdown_cancelled Callback", return_cb);
    
    /*
     const char* server_address, const char* redis_address, const char* zk_uri, const char* root_dir, const char* app_id, qplugin_qserver_event_listener::type_on_qserver_pre_start pre_start_cb, qplugin_qserver_event_listener::type_on_qserver_start start_cb, qplugin_qserver_event_listener::type_on_qserver_stop stop_cb,
                              qplugin_qserver_event_listener::type_on_qserver_error error_cb,
                              qplugin_qserver_event_listener::type_room_event_create room_event_create_cb,
                              qplugin_qserver_event_listener::type_room_event_start room_event_start_cb,
                              qplugin_qserver_event_listener::type_room_event_player_added room_event_player_added_cb,
                              qplugin_qserver_event_listener::type_room_event_message room_event_message_cb,
                              qplugin_qserver_event_listener::type_room_event_player_removed room_player_removed_cb,
                              qplugin_qserver_event_listener::type_room_event_end room_event_end_cb,
                              qplugin_qserver_event_listener::type_room_event_countdown_to_start room_event_countdown_to_start_cb,
                              qplugin_qserver_event_listener::type_room_event_countdown_cancelled room_event_countdown_cancelled_cb
     */
    /*
     struct CallbackPayload {
         short type = 0;
         qnetworkserver* server = nullptr;
         const char* ip = nullptr;
         uint16_t port = 0;
         int error_code = 0;
         int room = -1;
         class room* room_ptr = nullptr;
         const char* pid = nullptr;
         unsigned cid_hash = 0;
         const char* msg = nullptr;
         int count = 0;
         int max_count = 0;
     };
     */
    // Call the C++ function
    gsdk::server::spawn_qserver(
        server_address.c_str(), redisAddress.c_str(), zkUri.c_str(), rootDir.c_str(), inf_file.c_str(), appId.c_str(),
        [](qnetworkserver* server){ //pre_start_cb
            qserver_spawn_qserver_cb_data* cdata = static_cast<qserver_spawn_qserver_cb_data*>(server->get_user_arg());
            auto* payload = new CallbackPayload{ 1, server };
            napi_call_threadsafe_function(cdata->preStartCbRef, payload, napi_tsfn_blocking);
        },
        [](qnetworkserver* server, const char* ip, uint16_t port){  //start_cb
            qserver_spawn_qserver_cb_data* cdata = static_cast<qserver_spawn_qserver_cb_data*>(server->get_user_arg());
            auto* payload = new CallbackPayload{ 2, server, ip, port };
            napi_call_threadsafe_function(cdata->startCbRef, payload, napi_tsfn_blocking);
        },
        [](qnetworkserver* server){ //stop_cb
            qserver_spawn_qserver_cb_data* cdata = static_cast<qserver_spawn_qserver_cb_data*>(server->get_user_arg());
            auto* payload = new CallbackPayload{ 3, server};
            napi_call_threadsafe_function(cdata->stopCbRef, payload, napi_tsfn_blocking);
        },
        [](qnetworkserver* server, int error_code){ //error_cb
            qserver_spawn_qserver_cb_data* cdata = static_cast<qserver_spawn_qserver_cb_data*>(server->get_user_arg());
            auto* payload = new CallbackPayload{ 4, server, nullptr, 0, error_code};
            napi_call_threadsafe_function(cdata->errorCbRef, payload, napi_tsfn_blocking);
        },
        [](qnetworkserver* server, int room, class room* room_ptr){ //room_event_create_cb
            qserver_spawn_qserver_cb_data* cdata = static_cast<qserver_spawn_qserver_cb_data*>(server->get_user_arg());
            auto* payload = new CallbackPayload{ 5, server, nullptr, 0, 0, room, room_ptr};
            napi_call_threadsafe_function(cdata->room_event_create_CbRef, payload, napi_tsfn_blocking);
        },
        [](qnetworkserver* server, int room, class room* room_ptr){ //room_event_start_cb
            qserver_spawn_qserver_cb_data* cdata = static_cast<qserver_spawn_qserver_cb_data*>(server->get_user_arg());
            auto* payload = new CallbackPayload{ 6, server, nullptr, 0, 0, room, room_ptr};
            napi_call_threadsafe_function(cdata->room_event_start_CbRef, payload, napi_tsfn_blocking);
        },
        [](qnetworkserver* server, int room, class room* room_ptr, const char* pid, unsigned cid_hash){ //type_room_event_player_added
            qserver_spawn_qserver_cb_data* cdata = static_cast<qserver_spawn_qserver_cb_data*>(server->get_user_arg());
            auto* payload = new CallbackPayload{ 7, server, nullptr, 0, 0, room, room_ptr, pid, cid_hash};
            napi_call_threadsafe_function(cdata->room_event_player_added_CbRef, payload, napi_tsfn_blocking);
        },
        [](qnetworkserver* server, int room, class room* room_ptr, const char* pid, unsigned cid_hash, const char* msg){ //room_event_message_cb
            qserver_spawn_qserver_cb_data* cdata = static_cast<qserver_spawn_qserver_cb_data*>(server->get_user_arg());
            auto* payload = new CallbackPayload{ 8, server, nullptr, 0, 0, room, room_ptr, pid, cid_hash, msg};
            napi_call_threadsafe_function(cdata->room_event_message_CbRef, payload, napi_tsfn_blocking);
        },
        [](qnetworkserver* server, int room, class room* room_ptr, const char* pid, unsigned cid_hash){ //room_player_removed_cb
            qserver_spawn_qserver_cb_data* cdata = static_cast<qserver_spawn_qserver_cb_data*>(server->get_user_arg());
            auto* payload = new CallbackPayload{ 9, server, nullptr, 0, 0, room, room_ptr, pid, cid_hash};
            napi_call_threadsafe_function(cdata->room_player_removed_CbRef, payload, napi_tsfn_blocking);
        },
        [](qnetworkserver* server, int room, class room* room_ptr){ //room_event_end_cb
            qserver_spawn_qserver_cb_data* cdata = static_cast<qserver_spawn_qserver_cb_data*>(server->get_user_arg());
            auto* payload = new CallbackPayload{ 10, server, nullptr, 0, 0, room, room_ptr};
            napi_call_threadsafe_function(cdata->room_event_end_CbRef, payload, napi_tsfn_blocking);
        },
        [](qnetworkserver* server, int room, class room* room_ptr, int count, int max_count){ //room_event_countdown_to_start_cb
            qserver_spawn_qserver_cb_data* cdata = static_cast<qserver_spawn_qserver_cb_data*>(server->get_user_arg());
            auto* payload = new CallbackPayload{ 11, server, nullptr, 0, 0, room, room_ptr, nullptr, 0, nullptr, count, max_count};
            napi_call_threadsafe_function(cdata->room_event_countdown_to_start_CbRef, payload, napi_tsfn_blocking);
        },
        [](qnetworkserver* server, int room, class room* room_ptr){ //room_event_countdown_cancelled_cb
            qserver_spawn_qserver_cb_data* cdata = static_cast<qserver_spawn_qserver_cb_data*>(server->get_user_arg());
            auto* payload = new CallbackPayload{ 12, server, nullptr, 0, 0, room, room_ptr};
            napi_call_threadsafe_function(cdata->room_event_countdown_cancelled_CbRef, payload, napi_tsfn_blocking);
        },
        callbackData
        );

    return env.Null();
}

Napi::Value napi_qserver_release_callbacks(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsExternal()) {
        Napi::TypeError::New(env, "Expected (qnetworkserver*)").ThrowAsJavaScriptException();
        return env.Null();
    }
    qnetworkserver* server = info[0].As<Napi::External<qnetworkserver>>().Data();
    qserver_spawn_qserver_cb_data* cdata = static_cast<qserver_spawn_qserver_cb_data*>(server->get_user_arg());
    cdata->release(env);
    GX_DELETE(cdata);   // NOTE: server->get_user_arg() is now invalid. WE MUST NOT CALL ANY qserver function post this on this server.
    return env.Null();
}

Napi::Value napi_room_broadcast_except(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // Validate arguments count
    if (info.Length() < 4 || !info[0].IsExternal() || !info[1].IsExternal() ||
        !info[2].IsNumber() || !info[3].IsString()) {
        Napi::TypeError::New(env, "Expected (qnetworkserver*, room*, unsigned, string)").ThrowAsJavaScriptException();
        return env.Null();
    }

    // Extract arguments
    qnetworkserver* server = info[0].As<Napi::External<qnetworkserver>>().Data();
    room* room_ptr = info[1].As<Napi::External<room>>().Data();
    unsigned cid_hash = info[2].As<Napi::Number>().Uint32Value();
    std::string msg = info[3].As<Napi::String>().Utf8Value();

    // Call the actual function
    bool result = gsdk::server::room_broadcast_except(server, room_ptr, cid_hash, msg.c_str(), msg.length());

    // Return result as a boolean
    return Napi::Boolean::New(env, result);
}

Napi::Value napi_room_broadcast(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // Validate arguments
    if (info.Length() < 3 || !info[0].IsExternal() || !info[1].IsExternal() || !info[2].IsString()) {
        Napi::TypeError::New(env, "Expected (qnetworkserver*, room*, string)").ThrowAsJavaScriptException();
        return env.Null();
    }

    // Extract arguments
    qnetworkserver* server = info[0].As<Napi::External<qnetworkserver>>().Data();
    room* room_ptr = info[1].As<Napi::External<room>>().Data();
    std::string msg = info[2].As<Napi::String>().Utf8Value();

    // Call the actual C++ function
    gsdk::server::room_broadcast(server, room_ptr, msg.c_str(), msg.length());

    // No return value (void function)
    return env.Undefined();
}


Napi::Value napi_room_send_to(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // Validate arguments
    if (info.Length() < 4 || !info[0].IsExternal() || !info[1].IsExternal() || !info[2].IsNumber() || !info[3].IsString()) {
        Napi::TypeError::New(env, "Expected (qnetworkserver*, room*, unsigned, string)").ThrowAsJavaScriptException();
        return env.Null();
    }

    // Extract arguments
    qnetworkserver* server = info[0].As<Napi::External<qnetworkserver>>().Data();
    room* room_ptr = info[1].As<Napi::External<room>>().Data();
    unsigned cid_hash = info[2].As<Napi::Number>().Uint32Value();
    std::string msg = info[3].As<Napi::String>().Utf8Value();

    // Call the actual C++ function
    bool success = gsdk::server::room_send_to(server, room_ptr, cid_hash, msg.c_str(), msg.length());

    // Return success status as a boolean
    return Napi::Boolean::New(env, success);
}
}
