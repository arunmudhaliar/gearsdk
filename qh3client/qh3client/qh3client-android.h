//
// Created by Arun A on 28/01/24.
//

#ifndef QUNITYPLUGIN_QH3CLIENT_ANDROID_H
#define QUNITYPLUGIN_QH3CLIENT_ANDROID_H
#include <jni.h>
#include <string>
#include "qh3client.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "qplugin-native-qh3client"
namespace client {
    class qh3client_android : public qh3client {
    public:
        qh3client_android(const qstring &host, const qstring &port);
        virtual ~qh3client_android();

        void on_prepare_client_send() override;
        void on_post_send_cleanup() override;
        void* get_client_specific_data() override;
        
    private:
        JNIEnv* env = nullptr;
        jint attachStatus = 0;
    };
};
#endif //QUNITYPLUGIN_QH3CLIENT_ANDROID_H
