//
// Created by Arun A on 28/01/24.
//
#include <jni.h>
#include <string>
#include "../../../../../../qh3client/qh3client/qh3client.hpp"
#include "../../../../../../qh3client/qh3client/qh3client_helper.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "qplugin-native-qh3client"

extern "C" {
JNIEXPORT jboolean JNICALL
        Java_com_gearsdk_qunityplugin_qh3client_send_1async_1request(JNIEnv *env, jobject thiz,
jstring j_host, jstring j_port,
jstring j_path, jstring j_payload, jobject request_cb_interface) {
    const char* native_str_host = (env)->GetStringUTFChars(j_host, 0);
    const char* native_str_port = (env)->GetStringUTFChars(j_port, 0);
    const char* native_str_path = (env)->GetStringUTFChars(j_path, 0);
    const char* native_str_payload = (env)->GetStringUTFChars(j_payload, 0);

    qstring host(native_str_host);
    qstring port((env)->GetStringUTFChars(j_port, 0));
    qstring path((env)->GetStringUTFChars(j_path, 0));
    qstring payload((env)->GetStringUTFChars(j_payload, 0));

    (env)->ReleaseStringUTFChars(j_host, native_str_host);
    (env)->ReleaseStringUTFChars(j_port, native_str_port);
    (env)->ReleaseStringUTFChars(j_path, native_str_path);
    (env)->ReleaseStringUTFChars(j_payload, native_str_payload);

    jclass callbackClass = env->GetObjectClass(request_cb_interface);
    jmethodID callbackMethod = env->GetMethodID(callbackClass, "callback_method", "(Ljava/lang/String;)V");


    qh3client_helper::send_async_request(host, port, conn_io_req_res::create(path, payload),
    [env, request_cb_interface, callbackClass, callbackMethod](conn_io_req_res *response) {
        bool validate = response->validate();
        //                assert(validate);
        if (!validate) {
        //DEBUG_PRINT_ERROR(__LOGTAG__, "crc fail !!!");
        }
        conn_io_req_res::header *token_header = response->get_header(
                "token");
        if (token_header == nullptr) {
        DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "No Token");
        }

        const conn_io_req_res::payload &payload = response->data;
        DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__,
        "async returned %s !!!",
        payload.buffer.c_str());


        // Find the interface class and its method
//        jclass callbackClass = env->GetObjectClass(request_cb_interface);
//        jmethodID callbackMethod = env->GetMethodID(callbackClass, "callback_method", "(Ljava/lang/String;)V");
        if (callbackMethod == nullptr) {
            return; // Method not found, handle error appropriately
        }

        // Create a string to pass to the callback
        jstring message = env->NewStringUTF("Hello from JNI");

        // Call the callback method
        env->CallVoidMethod(request_cb_interface, callbackMethod, message);

        // Clean up local references
        env->DeleteLocalRef(message);
        env->DeleteLocalRef(callbackClass);
    }, false);
    return true;
}
};