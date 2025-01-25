#include <napi.h>
#include <pthread.h>


// A function to create a MyObject instance and pass it to JavaScript
Napi::Value GetPointer(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // Create a new instance of MyObject
    // MyObject* obj = new MyObject();

    // Wrap the object pointer in an Napi::External and return it
    return Napi::External<void>::New(env, nullptr);
}

// A function to interact with the MyObject from JavaScript
Napi::Value SetPointer(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // Check if the argument is an External object
    if (info.Length() < 1 || !info[0].IsExternal()) {
        Napi::TypeError::New(env, "Expected an External object").ThrowAsJavaScriptException();
        return env.Null();
    }

    // Get the object pointer from the External object
    Napi::External<void> external = info[0].As<Napi::External<void>>();
    void* obj = external.Data(); // Get the pointer to MyObject

    // // Check if the pointer is valid
    // if (obj == nullptr) {
    //     Napi::Error::New(env, "Invalid object pointer").ThrowAsJavaScriptException();
    //     return env.Null();
    // }
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

    const char* thread_name = info[0].ToString().Utf8Value().c_str();
    // Napi::String threadName = info[0].As<Napi::String>();
    // Napi::String::Utf8Value threadNameUtf8(env, threadName);
    // Napi::String::Utf8Value threadName(env, info[0]);

    // Set the thread name using pthread API
    pthread_setname_np(thread_name);
}

// A simple function to add two numbers and call back with the result
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
    Napi::Env env = info.Env();
    printf("Test\n");
}

// Initialize the module and export the function
Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set(Napi::String::New(env, "add"), Napi::Function::New(env, Add));
    exports.Set(Napi::String::New(env, "test"), Napi::Function::New(env, Test));
    exports.Set(Napi::String::New(env, "get_thread_name"), Napi::Function::New(env, GetThreadName));
    exports.Set(Napi::String::New(env, "set_thread_name"), Napi::Function::New(env, SetThreadName));
    exports.Set(Napi::String::New(env, "get_pointer"), Napi::Function::New(env, GetPointer));
    exports.Set(Napi::String::New(env, "set_pointer"), Napi::Function::New(env, SetPointer));
    return exports;
}

// Define the module
NODE_API_MODULE(addon, Init)
