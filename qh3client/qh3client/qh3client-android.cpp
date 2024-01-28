//
// Created by Arun A on 28/01/24.
//
#include "qh3client-android.h"
#include "qh3client_helper.hpp"
using namespace client;

qh3client_android::qh3client_android(const qstring &host, const qstring &port) :
                                     qh3client(host, port) {
}

qh3client_android::~qh3client_android() {

}

void qh3client_android::on_prepare_client_send() {
    attachStatus = (gsdk::device::g_JavaVM)->AttachCurrentThread(&env, NULL);
    if (attachStatus != JNI_OK) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "couldn't attach to jvm - %d !!!", attachStatus);
    }
}

void qh3client_android::on_post_send_cleanup() {
    if (attachStatus == JNI_OK) {
        // Detach the thread when done with JNI operations
        (gsdk::device::g_JavaVM)->DetachCurrentThread();
    }
}

void* qh3client_android::get_client_specific_data() {
    return env;
}