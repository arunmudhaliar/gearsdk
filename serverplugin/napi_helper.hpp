#ifndef NAPI_HELPER
#define NAPI_HELPER

#include <napi.h>

namespace napi_funcs {

napi_threadsafe_function create_threadsafe_func(Napi::Env env, Napi::Function jsCallback, const char* resourceName, napi_threadsafe_function_call_js return_cb);
}
#endif
