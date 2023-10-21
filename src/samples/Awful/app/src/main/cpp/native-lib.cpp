#include <jni.h>
#include <string>

#if defined(ANDROID)
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#define XR_USE_GRAPHICS_API_OPENGL_ES 1
#define XR_USE_PLATFORM_ANDROID 1
#endif

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

extern "C" JNIEXPORT jstring JNICALL
Java_us_xyzw_awful_MainActivity_stringFromJNI(JNIEnv* env, jobject obj) {
    JavaVM* vm;
    env->GetJavaVM(&vm);
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

bool initOpenXR(JavaVM* vm, jobject ctx) {
    PFN_xrInitializeLoaderKHR xrInitializeLoaderKHR = nullptr;
    {
        auto result = xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR", reinterpret_cast<PFN_xrVoidFunction*>(&xrInitializeLoaderKHR));
        if (result != XR_SUCCESS) {
            // Log error message
            return false;
        }
    }
    {
        XrLoaderInitInfoAndroidKHR ii{XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR};
        ii.applicationVM = vm;
        ii.applicationContext = ctx;
        auto result = xrInitializeLoaderKHR(reinterpret_cast<XrLoaderInitInfoBaseHeaderKHR*>(&ii));
    }
    return true;
}
