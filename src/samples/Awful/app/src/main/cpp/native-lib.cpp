#include <jni.h>
#include <string>

#include <openxr/openxr.h>

extern "C" JNIEXPORT jstring JNICALL
Java_us_xyzw_awful_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}
