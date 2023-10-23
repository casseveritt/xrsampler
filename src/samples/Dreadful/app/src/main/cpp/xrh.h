#pragma once

#if defined(ANDROID)
#include <jni.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#define XR_USE_GRAPHICS_API_OPENGL_ES 1
#define XR_USE_PLATFORM_ANDROID 1
#endif

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <vector>
#include <span>

namespace xrh
{
    #if defined(ANDROID)
    bool init_loader(JavaVM *vm, jobject ctx);
    #endif

    XrExtensionProperties make_ext_prop(const char* name, uint32_t ver = 1);

    std::vector<XrExtensionProperties> enum_extensions();

    bool ext_supported(std::span<const XrExtensionProperties> ext_span, const XrExtensionProperties& ext);

    XrInstance create_instance(const std::vector<XrExtensionProperties>& required, std::vector<XrExtensionProperties>& desired);

}