#include <jni.h>
#include <string>

#include <openxr/openxr.h>

extern "C" JNIEXPORT jstring JNICALL
Java_us_xyzw_awful_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    PFN_xrInitializeLoaderKHR xrInitializeLoaderKHR = nullptr;
    xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR", reinterpret_cast<PFN_xrVoidFunction*>(&xrInitializeLoaderKHR));
    return env->NewStringUTF(hello.c_str());
}
