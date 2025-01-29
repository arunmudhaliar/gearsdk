#include "napi_helper.hpp"


// Helper to create a thread-safe function
napi_threadsafe_function napi_funcs::create_threadsafe_func(Napi::Env env, Napi::Function jsCallback, const char* resourceName, napi_threadsafe_function_call_js return_cb) {
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
