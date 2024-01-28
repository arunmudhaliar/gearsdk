#include <jni.h>
#include <string>
#include "../../../../../../common/sdktypes.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "qplugin-app"

extern "C" {
JNIEXPORT void JNICALL
Java_com_gearsdk_qunityplugin_MainActivity_init_1gsdk(JNIEnv *env, jobject /* this */) {
    init_gsdk();
}

JNIEXPORT jstring JNICALL
Java_com_gearsdk_qunityplugin_MainActivity_stringFromJNI(JNIEnv *env, jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}
};