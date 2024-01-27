#include <jni.h>
#include <string>
#include "../../../../../../qh3client/qh3client/qh3client.hpp"
#include "../../../../../../qh3client/qh3client/qh3client_helper.hpp"
extern "C" JNIEXPORT jstring
#undef __LOGTAG__
#define __LOGTAG__ "qplugin-app"

JNICALL
Java_com_gearsdk_qunityplugin_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    init_gsdk();
    std::string hello = "Hello from C++";

    qstring host = "192.168.0.230";
    qstring port = "4004";
    qh3client_helper::send_async_request(host, port, conn_io_req_res::create("/whoami", "{}"),
        [](conn_io_req_res* response) {
         bool validate = response->validate();
        //                assert(validate);
         if (!validate) {
             //DEBUG_PRINT_ERROR(__LOGTAG__, "crc fail !!!");
         }
         conn_io_req_res::header* token_header = response->get_header("token");
         if (token_header == nullptr) {
             DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "No Token");
         }

         const conn_io_req_res::payload& payload = response->data;
         DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "async returned %s !!!", payload.buffer.c_str());
         }, false);
    return env->NewStringUTF(hello.c_str());
}