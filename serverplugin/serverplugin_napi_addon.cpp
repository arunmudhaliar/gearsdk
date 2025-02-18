#include <napi.h>
#include <pthread.h>
#include "napi_helper.hpp"
#include "plugin_qh3server.h"
#include "serverplugin_napi_addon.hpp"
#include "gserverplugin_napi_addon.hpp"

//// A function to create a MyObject instance and pass it to JavaScript
//Napi::Value GetPointer(const Napi::CallbackInfo& info) {
//    Napi::Env env = info.Env();
//
//    // Create a new instance of MyObject
//    // MyObject* obj = new MyObject();
//
//    // Wrap the object pointer in an Napi::External and return it
//    return Napi::External<void>::New(env, nullptr);
//}
namespace napi_funcs {
// A function to interact with the MyObject from JavaScript
Napi::Value SetPointer(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsExternal()) {
        Napi::TypeError::New(env, "Expected an External object").ThrowAsJavaScriptException();
        return env.Null();
    }
    Napi::External<void> external = info[0].As<Napi::External<void>>();
    void* obj = external.Data();
    UNUSED(obj);
    return env.Null();
}

Napi::Value GetThreadName(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();  // Get the environment for NAPI
    char threadName[128];
    pthread_getname_np(pthread_self(), threadName, sizeof(threadName));
    return Napi::String::New(env, threadName);
}

void SetThreadName(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Thread name must be a string").ThrowAsJavaScriptException();
        return;
    }
    std::string thread_name = info[0].As<Napi::String>().Utf8Value();
    PTHREAD_NAME(thread_name.c_str());
}

void Add(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // Ensure two arguments are provided
    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsNumber() || !info[2].IsFunction()) {
        Napi::TypeError::New(env, "Two numbers and a callback function expected").ThrowAsJavaScriptException();
        return;
    }

    // Get the arguments
    double arg0 = info[0].As<Napi::Number>().DoubleValue();
    double arg1 = info[1].As<Napi::Number>().DoubleValue();
    Napi::Function callback = info[2].As<Napi::Function>();

    // Perform the addition
    double result = arg0 + arg1;

    // Call the callback with the result
    callback.Call({ Napi::Number::New(env, result) });
}

void Test(const Napi::CallbackInfo& info) {
    UNUSED(info);
    printf("Test\n");
}

void napi_setup_signal_handler(const Napi::CallbackInfo& info) {
    UNUSED(info);
    gsdk::server::setup_signal_handler();
}

void napi_pre_init_serverplugin_sdk(const Napi::CallbackInfo& info) {
    UNUSED(info);
    gsdk::server::pre_init_serverplugin_sdk();
}

// Helper to create a thread-safe function
napi_threadsafe_function CreateTSFN(Napi::Env env, Napi::Function jsCallback, const char* resourceName, napi_threadsafe_function_call_js return_cb) {
    napi_threadsafe_function tsfn;
    napi_create_threadsafe_function(
        env,
        jsCallback,
        nullptr,
        Napi::String::New(env, resourceName),
        0,
        1,
        nullptr,
        nullptr,
        nullptr,
        return_cb,
        &tsfn
    );
    return tsfn;
}

Napi::Value napi_spawn_qh3router(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // Validate arguments
    if (info.Length() < 12 || !info[0].IsString() || !info[1].IsString() || !info[2].IsString() ||
        !info[3].IsString() || !info[4].IsString() || !info[5].IsString() || !info[6].IsNumber() ||
        !info[7].IsNumber() || !info[8].IsString() || !info[9].IsFunction() ||
        !info[10].IsFunction() || !info[11].IsFunction() || !info[12].IsFunction()) {
        Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    // Extract arguments
    std::string routerAddress = info[0].As<Napi::String>();
    std::string mongodbUri = info[1].As<Napi::String>();
    std::string redisAddress = info[2].As<Napi::String>();
    std::string zkUri = info[3].As<Napi::String>();
    std::string rootDir = info[4].As<Napi::String>();
    std::string inf_file = info[5].As<Napi::String>();
    uint16_t commandPort = info[6].As<Napi::Number>().Uint32Value();
    uint16_t routerPortReturn = info[7].As<Napi::Number>().Uint32Value();
    std::string appId = info[8].As<Napi::String>();

    struct CallbackPayload {
        short type = 0;
        qh3router* router = nullptr;
        napi_qh3router_cb_data* cb_data = nullptr;
        int error_code = 0;
    };

    auto return_cb = [](napi_env env, napi_value jsCallback, void* context, void* data) {
        UNUSED(context);
        // Convert the payload to CallbackPayload and extract pointers
        CallbackPayload* payload = static_cast<CallbackPayload*>(data);
        qh3router* router = payload->router;
        napi_qh3router_cb_data* cdata = payload->cb_data;
        
        // Ensure proper scope
        Napi::HandleScope scope(env);
        Napi::Function jsFunc = Napi::Function(env, jsCallback);

        switch (payload->type) {
            case 1:
            {
                jsFunc.Call({
                    Napi::External<qh3router>::New(env, router),
                    Napi::External<napi_qh3router_cb_data>::New(env, cdata)
                    });
            }
            break;
            case 2:
            case 3:
            {
                jsFunc.Call({
                    Napi::External<qh3router>::New(env, router)
                    });
            }
            break;
            case 4:
            {
                jsFunc.Call({
                    Napi::External<qh3router>::New(env, router),
                    Napi::Number::New(env, payload->error_code),
                    });
            }
            break;
        }


        // Free the payload memory
        GX_DELETE(payload);
        };

    // Extract and persist JavaScript callbacks
    napi_qh3router_cb_data* callbackData = DEBUG_NEW napi_qh3router_cb_data();
    callbackData->pre_start_cb_ref = create_threadsafe_func(env, info[9].As<Napi::Function>(), "PreStart Callback", return_cb);
    callbackData->start_cb_ref = create_threadsafe_func(env, info[10].As<Napi::Function>(), "Start Callback", return_cb);
    callbackData->stop_cb_ref = create_threadsafe_func(env, info[11].As<Napi::Function>(), "Stop Callback", return_cb);
    callbackData->error_cb_ref = create_threadsafe_func(env, info[12].As<Napi::Function>(), "Error Callback", return_cb);

    // Call the C++ function
    gsdk::server::spawn_qh3router(routerAddress.c_str(), mongodbUri.c_str(), redisAddress.c_str(), zkUri.c_str(), rootDir.c_str(), inf_file.c_str(),
        commandPort, routerPortReturn, appId.c_str(),
        [](qh3router* router, void* user_arg) mutable {
        napi_qh3router_cb_data* cdata = static_cast<napi_qh3router_cb_data*>(user_arg);
            auto* payload = new CallbackPayload{ 1, router, cdata };
            napi_call_threadsafe_function(cdata->pre_start_cb_ref, payload, napi_tsfn_blocking);
        }, [](qh3router* router, void* user_arg) {
            napi_qh3router_cb_data* cdata = static_cast<napi_qh3router_cb_data*>(user_arg);
            auto* payload = new CallbackPayload{ 2, router };
            napi_call_threadsafe_function(cdata->start_cb_ref, payload, napi_tsfn_blocking);
        }, [](qh3router* router, void* user_arg) {
            napi_qh3router_cb_data* cdata = static_cast<napi_qh3router_cb_data*>(user_arg);
            auto* payload = new CallbackPayload{ 3, router };
            napi_call_threadsafe_function(cdata->stop_cb_ref, payload, napi_tsfn_blocking);
        }, [](qh3router* router, void* user_arg, int error_code) {
            napi_qh3router_cb_data* cdata = static_cast<napi_qh3router_cb_data*>(user_arg);
            auto* payload = new CallbackPayload{ 4, router, nullptr, error_code };
            napi_call_threadsafe_function(cdata->error_cb_ref, payload, napi_tsfn_blocking);
            }, callbackData);

    return env.Null();
}

Napi::Value napi_qh3router_release_callbacks(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsExternal()) {
        Napi::TypeError::New(env, "Expected (qh3router*)").ThrowAsJavaScriptException();
        return env.Null();
    }
    napi_qh3router_cb_data* cdata = info[0].As<Napi::External<napi_qh3router_cb_data>>().Data();
    cdata->release(env);
    GX_DELETE(cdata);   // NOTE: server->get_server_config().user_arg is now invalid. WE MUST NOT CALL ANY qh3router function post this on this server.
    debug_print(LOG_LEVEL_0, __LOGTAG__, "releasing qh3router_spawn_qh3router_cb_data");
    return env.Null();
}

Napi::Value napi_spawn_qh3server(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    // Validate arguments
    if (info.Length() < 12 || !(info[0].IsExternal() || info[0].IsNull()) || !info[1].IsString() ||
        !info[2].IsString() || !info[3].IsString() || !info[4].IsNumber() ||
        !info[5].IsNumber() || !info[6].IsString() || !info[7].IsFunction() ||
        !info[8].IsFunction() || !info[9].IsFunction() || !info[10].IsFunction() || !info[11].IsFunction()) {
        Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    qh3router* router = nullptr;
    if (info[0].IsExternal()) {
        Napi::External<void> external = info[0].As<Napi::External<void>>();
        router = static_cast<qh3router*>(external.Data());
    }

    // Extract arguments
    std::string routerAddress = info[1].As<Napi::String>();
    std::string rootDir = info[2].As<Napi::String>();
    std::string inf_file = info[3].As<Napi::String>();
    uint16_t commandPort = info[4].As<Napi::Number>().Uint32Value();
    uint16_t routerPortReturn = info[5].As<Napi::Number>().Uint32Value();
    std::string appId = info[6].As<Napi::String>();

    struct CallbackPayload {
        short type = 0;
        qh3server* server = nullptr;
        const char* ip = nullptr;
        uint16_t port = 0;
        int error_code = 0;
        uint8_t* cid = nullptr;
        uint16_t cid_len = 0;
        const char* path = nullptr;
        const char* buffer = nullptr;
        unsigned long len = 0;
        const char* headers_buffer = nullptr;
        unsigned long headers_buffer_size = 0;
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
                Napi::External<qh3server>::New(env, payload->server)
                });
            break;
        case 2:
            jsFunc.Call({
                Napi::External<qh3server>::New(env, payload->server),
                Napi::String::New(env, payload->ip),
                Napi::Number::New(env, payload->port)
                });
            break;
        case 4:
            jsFunc.Call({
                Napi::External<qh3server>::New(env, payload->server),
                Napi::Number::New(env, payload->error_code)
                });
            break;
        case 5:
            jsFunc.Call({
                Napi::External<qh3server>::New(env, payload->server),
                Napi::External<void>::New(env, payload->cid),
                Napi::Number::New(env, payload->cid_len),
                Napi::String::New(env, payload->path),
                Napi::String::New(env, payload->buffer, payload->len),
                Napi::String::New(env, payload->headers_buffer, payload->headers_buffer_size),
                });
            break;
        }
        // Free the payload memory
        GX_DELETE(payload);
        };

    // Extract and persist JavaScript callbacks
    napi_qh3server_cb_data* callbackData = DEBUG_NEW napi_qh3server_cb_data();
    callbackData->pre_start_cb_ref = create_threadsafe_func(env, info[7].As<Napi::Function>(), "PreStart Callback", return_cb);
    callbackData->start_cb_ref = create_threadsafe_func(env, info[8].As<Napi::Function>(), "Start Callback", return_cb);
    callbackData->stop_cb_ref = create_threadsafe_func(env, info[9].As<Napi::Function>(), "Stop Callback", return_cb);
    callbackData->error_cb_ref = create_threadsafe_func(env, info[10].As<Napi::Function>(), "Error Callback", return_cb);
    callbackData->parse_cb_ref = create_threadsafe_func(env, info[11].As<Napi::Function>(), "Parse Callback", return_cb);

    // Call the C++ function
    gsdk::server::spawn_qh3server(router, routerAddress.c_str(), rootDir.c_str(), inf_file.c_str(), commandPort, routerPortReturn, appId.c_str(),
        [](qh3server* server, void* user_arg) mutable {
        napi_qh3server_cb_data* cdata = static_cast<napi_qh3server_cb_data*>(user_arg);
            auto* payload = new CallbackPayload{ 1, server };
            napi_call_threadsafe_function(cdata->pre_start_cb_ref, payload, napi_tsfn_blocking);
        }, [](qh3server* server, void* user_arg, const char* ip, uint16_t port) {
            napi_qh3server_cb_data* cdata = static_cast<napi_qh3server_cb_data*>(user_arg);
            auto* payload = new CallbackPayload{ 2, server, ip, port };
            napi_call_threadsafe_function(cdata->start_cb_ref, payload, napi_tsfn_blocking);
        }, [](qh3server* server, void* user_arg) {
            napi_qh3server_cb_data* cdata = static_cast<napi_qh3server_cb_data*>(user_arg);
            auto* payload = new CallbackPayload{ 3, server };
            napi_call_threadsafe_function(cdata->stop_cb_ref, payload, napi_tsfn_blocking);
        }, [](qh3server* server, void* user_arg, int error_code) {
            napi_qh3server_cb_data* cdata = static_cast<napi_qh3server_cb_data*>(user_arg);
            auto* payload = new CallbackPayload{ 4, server, nullptr, 0, error_code };
            napi_call_threadsafe_function(cdata->error_cb_ref, payload, napi_tsfn_blocking);
        }, [](qh3server* server, void* user_arg, uint8_t* cid, uint16_t cid_len, const char* path, const char* buffer, unsigned long len, const char* headers_buffer, unsigned long headers_buffer_size) {
            napi_qh3server_cb_data* cdata = static_cast<napi_qh3server_cb_data*>(user_arg);
            auto* payload = new CallbackPayload{ 5, server, nullptr, 0, 0, cid, cid_len, path, buffer, len, headers_buffer, headers_buffer_size};
            napi_call_threadsafe_function(cdata->parse_cb_ref, payload, napi_tsfn_blocking);
        }, callbackData);

    return env.Null();
}

Napi::Value napi_qh3server_try_send_response(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // Validate the number of arguments
    if (info.Length() < 5) {
        Napi::TypeError::New(env, "Expected 4 arguments: server, cid, cid_len, payload (string), payload length (number)").ThrowAsJavaScriptException();
        return env.Null();
    }

    // Validate the first argument (server pointer)
    if (!info[0].IsExternal()) {
        Napi::TypeError::New(env, "Expected an external object for server").ThrowAsJavaScriptException();
        return env.Null();
    }
    auto* server = info[0].As<Napi::External<qh3server>>().Data();

    // Validate the second argument (CID buffer)
    if (!info[1].IsExternal()) {
        Napi::TypeError::New(env, "Expected a external object for cid").ThrowAsJavaScriptException();
        return env.Null();
    }
    Napi::External<uint8_t> cidBuffer = info[1].As<Napi::External<uint8_t>>();
    uint8_t* cid = cidBuffer.Data();

    if (!info[2].IsNumber()) {
        Napi::TypeError::New(env, "Expected a number for cid length").ThrowAsJavaScriptException();
        return env.Null();
    }

    uint16_t cid_len = info[2].As<Napi::Number>().Uint32Value();

    // Validate the third argument (payload string)
    if (!info[3].IsString()) {
        Napi::TypeError::New(env, "Expected a string for payload").ThrowAsJavaScriptException();
        return env.Null();
    }
    std::string payload = info[3].As<Napi::String>();

    // Validate the fourth argument (payload length)
    if (!info[4].IsNumber()) {
        Napi::TypeError::New(env, "Expected a number for payload length").ThrowAsJavaScriptException();
        return env.Null();
    }
    size_t payload_len = info[4].As<Napi::Number>().Uint32Value();

    // Call the actual C++ function
    gsdk::server::qh3server_try_send_response(server, cid, cid_len, payload.c_str(), payload_len);

    return env.Null();
}

Napi::Value napi_qh3server_release_callbacks(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsExternal()) {
        Napi::TypeError::New(env, "Expected (qh3server*)").ThrowAsJavaScriptException();
        return env.Null();
    }
    qh3server* server = info[0].As<Napi::External<qh3server>>().Data();
    napi_qh3server_cb_data* cdata = static_cast<napi_qh3server_cb_data*>(server->get_user_arg());
    cdata->release(env);
    GX_DELETE(cdata);   // NOTE: server->get_user_arg() is now invalid. WE MUST NOT CALL ANY qh3server function post this on this server.
    debug_print(LOG_LEVEL_0, __LOGTAG__, "releasing qh3server_spawn_qh3server_cb_data");
    return env.Null();
}

Napi::Value napi_get_live_connection_count(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // Check if the argument is an External object
    if (info.Length() < 1 || !info[0].IsExternal()) {
        Napi::TypeError::New(env, "Expected an External object").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::External<qh3server> external = info[0].As<Napi::External<qh3server>>();
    qh3server* server = external.Data(); // Get the pointer to MyObject
    return Napi::Number::New(env, server->get_live_connection_count());
}

Napi::Value napi_get_device_public_ip(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return Napi::String::New(env, gsdk::server::get_device_public_ip());
}


// NAPI wrapper for `get_crc32`
Napi::Value napi_get_crc32(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // Validate arguments
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected a string argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    // Get the string argument
    std::string guid = info[0].As<Napi::String>();

    // Call the actual C++ function
    unsigned long result = gsdk::server::get_crc32(guid.c_str(), static_cast<int>(guid.length()));

    // Return the result
    return Napi::Number::New(env, result);
}

// NAPI wrapper for `mod_crc32`
Napi::Value napi_mod_crc32(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // Validate arguments
    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsBuffer()) {
        Napi::TypeError::New(env, "Expected a number and a Buffer argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    // Get the arguments
    uLong adler = info[0].As<Napi::Number>().Uint32Value();
    Napi::Buffer<Bytef> buffer = info[1].As<Napi::Buffer<Bytef>>();

    // Call the actual C++ function
    unsigned long result = gsdk::server::mod_crc32(adler, buffer.Data(), buffer.Length());

    // Return the result
    return Napi::Number::New(env, result);
}

Napi::Value napi_qh3server_logfile(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // Validate arguments
    if (info.Length() < 7 ||
        !info[0].IsExternal() ||  // qh3server*
        !info[1].IsNumber() ||    // log level
        !info[2].IsNumber() ||    // log type
        !info[3].IsString() ||    // tag
        !info[4].IsString() ||    // pid
        !info[5].IsString() ||    // roomid
        !info[6].IsString()) {    // message
        Napi::TypeError::New(env, "Expected (qh3server*, number, number, string, string, string, string)").ThrowAsJavaScriptException();
        return env.Null();
    }

    // Extract arguments
    qh3server* server = info[0].As<Napi::External<qh3server>>().Data();
    qlogfile::log_lvls lvl = static_cast<qlogfile::log_lvls>(info[1].As<Napi::Number>().Uint32Value());
    qcustomlogger::elog_type type = static_cast<qcustomlogger::elog_type>(info[2].As<Napi::Number>().Uint32Value());
    std::string tag = info[3].As<Napi::String>();
    std::string pid = info[4].As<Napi::String>();
    std::string roomid = info[5].As<Napi::String>();
    std::string message = info[6].As<Napi::String>();

    // Call the actual function
    uint64_t result = gsdk::server::qh3server_logfile(server, lvl, type, tag.c_str(), pid.c_str(), roomid.c_str(), message.c_str());

    // Return result as BigInt (since it's a uint64_t)
    return Napi::BigInt::New(env, result);
}

Napi::Value napi_qh3server_stats_count(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // Validate arguments (first 3 are required, rest are optional)
    if (info.Length() < 5 ||
        !info[0].IsExternal() ||  // qh3server*
        !info[1].IsString() ||    // counter name
        !info[2].IsNumber() ||
        !info[3].IsString() ||
        !info[4].IsString()
        ) {    // count_val
        Napi::TypeError::New(env, "Expected (qh3server*, string, number, string, string, [optional strings...])").ThrowAsJavaScriptException();
        return env.Null();
    }

    // Extract mandatory parameters
    qh3server* server = info[0].As<Napi::External<qh3server>>().Data();
    std::string counter = info[1].As<Napi::String>();
    long count_val = info[2].As<Napi::Number>().Int64Value();

    // Extract optional parameters (default to "")
    std::string session = (info.Length() > 3) ? info[3].As<Napi::String>().Utf8Value() : "";
    std::string pid = (info.Length() > 4) ? info[4].As<Napi::String>().Utf8Value() : "";
    std::string version = (info.Length() > 5) ? info[5].As<Napi::String>().Utf8Value() : "";
    std::string epic = (info.Length() > 6) ? info[6].As<Napi::String>().Utf8Value() : "";
    std::string myth = (info.Length() > 7) ? info[7].As<Napi::String>().Utf8Value() : "";
    std::string legend = (info.Length() > 8) ? info[8].As<Napi::String>().Utf8Value() : "";
    std::string story = (info.Length() > 9) ? info[9].As<Napi::String>().Utf8Value() : "";
    std::string message = (info.Length() > 10) ? info[10].As<Napi::String>().Utf8Value() : "";

    // Call the actual function
    size_t result = gsdk::server::qh3server_stats_count(server, counter.c_str(), count_val, session.c_str(), pid.c_str(),
                                          version.c_str(), epic.c_str(), myth.c_str(), legend.c_str(),
                                          story.c_str(), message.c_str());

    // Return result as a Number
    return Napi::Number::New(env, result);
}


Napi::Value napi_qh3server_shutdown(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // Check if the argument is an External object
    if (info.Length() < 1 || !info[0].IsExternal()) {
        Napi::TypeError::New(env, "Expected an External object").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::External<gsdk::server::plugin_qh3server> external = info[0].As<Napi::External<gsdk::server::plugin_qh3server>>();
    gsdk::server::plugin_qh3server* server = external.Data();
    server->shutdown_server();
    return env.Null();
}

// Initialize the module and export the function
Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set(Napi::String::New(env, "get_thread_name"), Napi::Function::New(env, GetThreadName));
    exports.Set(Napi::String::New(env, "set_thread_name"), Napi::Function::New(env, SetThreadName));
    //    exports.Set(Napi::String::New(env, "set_pointer"), Napi::Function::New(env, SetPointer));
    exports.Set(Napi::String::New(env, "setup_signal_handler"), Napi::Function::New(env, napi_setup_signal_handler));
    exports.Set(Napi::String::New(env, "pre_init_serverplugin_sdk"), Napi::Function::New(env, napi_pre_init_serverplugin_sdk));
    exports.Set(Napi::String::New(env, "spawn_qh3router"), Napi::Function::New(env, napi_spawn_qh3router));
    exports.Set(Napi::String::New(env, "spawn_qh3server"), Napi::Function::New(env, napi_spawn_qh3server));
    exports.Set(Napi::String::New(env, "qh3server_try_send_response"), Napi::Function::New(env, napi_qh3server_try_send_response));
    exports.Set(Napi::String::New(env, "get_live_connection_count"), Napi::Function::New(env, napi_get_live_connection_count));
    exports.Set(Napi::String::New(env, "get_device_public_ip"), Napi::Function::New(env, napi_get_device_public_ip));
    exports.Set(Napi::String::New(env, "get_crc32"), Napi::Function::New(env, napi_get_crc32));
    exports.Set(Napi::String::New(env, "mod_crc32"), Napi::Function::New(env, napi_mod_crc32));
    exports.Set(Napi::String::New(env, "qh3server_logfile"), Napi::Function::New(env, napi_qh3server_logfile));
    exports.Set(Napi::String::New(env, "qh3server_stats_count"), Napi::Function::New(env, napi_qh3server_stats_count));
    exports.Set(Napi::String::New(env, "qh3server_shutdown"), Napi::Function::New(env, napi_qh3server_shutdown));
    exports.Set(Napi::String::New(env, "qh3server_release_callbacks"), Napi::Function::New(env, napi_qh3server_release_callbacks));
    exports.Set(Napi::String::New(env, "qh3router_release_callbacks"), Napi::Function::New(env, napi_qh3router_release_callbacks));
    
    exports.Set(Napi::String::New(env, "spawn_game_server"), Napi::Function::New(env, napi_gserver_funcs::napi_spawn_game_server));
    exports.Set(Napi::String::New(env, "room_broadcast_except"), Napi::Function::New(env, napi_gserver_funcs::napi_room_broadcast_except));
    exports.Set(Napi::String::New(env, "room_broadcast"), Napi::Function::New(env, napi_gserver_funcs::napi_room_broadcast));
    exports.Set(Napi::String::New(env, "room_send_to"), Napi::Function::New(env, napi_gserver_funcs::napi_room_send_to));
    exports.Set(Napi::String::New(env, "game_server_release_callbacks"), Napi::Function::New(env, napi_gserver_funcs::napi_game_server_release_callbacks));
    exports.Set(Napi::String::New(env, "qserver_logfile"), Napi::Function::New(env, napi_gserver_funcs::napi_qserver_logfile));
    exports.Set(Napi::String::New(env, "qserver_stats_count"), Napi::Function::New(env, napi_gserver_funcs::napi_qserver_stats_count));
    
    return exports;
}
// Define the module
NODE_API_MODULE(addon, Init)
}
