#include "../../../../../../common/sdktypes.hpp"

#include <jni.h>
#include <string>

#undef __LOGTAG__
#define __LOGTAG__ "qplugin-app"

extern "C" {
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
	init_gsdk(vm);
	return JNI_VERSION_1_6;
}

JNIEXPORT jstring JNICALL Java_com_gearsdk_qunityplugin_MainActivity_stringFromJNI(JNIEnv* env, jobject /* this */) {
	std::string hello = "Hello from C++";
	return env->NewStringUTF(hello.c_str());
}
};