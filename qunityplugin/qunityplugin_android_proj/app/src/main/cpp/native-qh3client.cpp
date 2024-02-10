//
// Created by Arun A on 28/01/24.
//
#include <jni.h>
#include <string>
#include "../../../../../../qh3client/qh3client/qh3client-android.h"
#include "../../../../../../qh3client/qh3client/qh3client_helper.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "qplugin-native-qh3client"

using namespace client;

extern "C" {
JNIEXPORT jboolean JNICALL
Java_com_gearsdk_qunityplugin_qh3client_1android_send_1async_1request(JNIEnv *env, jobject thiz,
jstring j_host, jstring j_port,
jstring j_path, jstring j_payload, jobject arg, jobject request_cb_interface) {
    if (j_payload == nullptr || request_cb_interface == nullptr) {
        return false;
    }
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

    jobject g_request_cb_interface = env->NewGlobalRef(request_cb_interface);
    jobject g_arg = env->NewGlobalRef(arg);

    qh3client_helper::send_async_request<client::qh3client_android>(host, port,
                                                                    conn_io_req_res::create(path, payload),
                                                                    g_arg,
    [g_request_cb_interface](conn_io_req_res *response, void* client_specific_data, void* arg) {
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

        JNIEnv* env1 = (JNIEnv*)client_specific_data;

        // Find the interface class and its method
        jclass callbackClass = env1->GetObjectClass(g_request_cb_interface);
        if (callbackClass == nullptr) {
            return; // Class not found, handle error appropriately
        }
        jmethodID callbackMethod = env1->GetMethodID(callbackClass,
                                                     "callback_method",
                                                     "(Ljava/lang/String;Ljava/lang/Object;I)V");

        if (callbackMethod == nullptr) {
            return; // Method not found, handle error appropriately
        }
        // Create a string to pass to the callback
        jstring j_payload = env1->NewStringUTF(payload.buffer.c_str());
        // Call the callback method
        env1->CallVoidMethod(g_request_cb_interface, callbackMethod, j_payload, arg, 0);
        // Clean up local references
        env1->DeleteLocalRef(j_payload);
        env1->DeleteLocalRef(callbackClass);
        env1->DeleteGlobalRef(g_request_cb_interface);
        if (arg != nullptr) {
            env1->DeleteGlobalRef((jobject) arg);
        }
    });
    return true;
}
};