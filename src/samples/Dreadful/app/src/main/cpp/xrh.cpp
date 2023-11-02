#include "xrh.h"
#include "AndroidOut.h"

#include <span>

using namespace std;

XrResult XRH_CheckErrors(XrResult result, const char *function)
{
    if (XR_FAILED(result))
    {
        char errorBuffer[XR_MAX_RESULT_STRING_SIZE];
        xrResultToString(XR_NULL_HANDLE, result, errorBuffer);
        aout << "OpenXR error: " << function << ": (" << result << ") " << errorBuffer << endl;
    }
    return result;
}
#define XRH(func) XRH_CheckErrors(func, #func);

#define DECL_PFN(pfn) PFN_##pfn pfn = nullptr
#define INIT_PFN(inst, pfn) XRH(xrGetInstanceProcAddr(inst, #pfn, reinterpret_cast<PFN_xrVoidFunction *>(&pfn)))

#define DECL_INIT_PFN(inst, pfn) \
    DECL_PFN(pfn);               \
    INIT_PFN(inst, pfn)

namespace
{
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
    DECL_PFN(xrGetOpenGLESGraphicsRequirementsKHR);
#endif

    XrExtensionProperties make_ext_prop(const char *name, uint32_t ver)
    {
        XrExtensionProperties ep{XR_TYPE_EXTENSION_PROPERTIES};
        strcpy(ep.extensionName, name);
        ep.extensionVersion = ver;
        return ep;
    }

    XrInstanceProperties get_instance_properties(XrInstance inst)
    {
        XrInstanceProperties ii = {XR_TYPE_INSTANCE_PROPERTIES};
        XRH(xrGetInstanceProperties(inst, &ii));
        aout << "Runtime: " << ii.runtimeName << "Version: "
             << XR_VERSION_MAJOR(ii.runtimeVersion) << "."
             << XR_VERSION_MINOR(ii.runtimeVersion) << "."
             << XR_VERSION_PATCH(ii.runtimeVersion) << endl;
        return ii;
    }

    XrSystemId get_system_id(XrInstance inst)
    {
        XrSystemGetInfo sysGetInfo = {XR_TYPE_SYSTEM_GET_INFO};
        sysGetInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

        XrSystemId sysid;
        XrResult res;
        XRH(res = xrGetSystem(inst, &sysGetInfo, &sysid));
        if (res != XR_SUCCESS)
        {
            aout << "Failed to get system." << endl;
            return XR_NULL_SYSTEM_ID;
        }
        return sysid;
    }

    XrSystemProperties get_system_properties(XrInstance inst, XrSystemId sysid)
    {
        XrSystemProperties sysprops = {XR_TYPE_SYSTEM_PROPERTIES};
        XRH(xrGetSystemProperties(inst, sysid, &sysprops));

        aout << "System Properties: Name=" << sysprops.systemName
             << " VendorId=" << sysprops.vendorId << endl;
        aout << "System Graphics Properties:"
             << " MaxWidth=" << sysprops.graphicsProperties.maxSwapchainImageWidth
             << " MaxHeight=" << sysprops.graphicsProperties.maxSwapchainImageHeight
             << " MaxLayers=" << sysprops.graphicsProperties.maxLayerCount << endl;
        return sysprops;
    }

    vector<XrExtensionProperties> enum_extensions()
    {
        uint32_t ext_count = 0;
        auto res = xrEnumerateInstanceExtensionProperties(nullptr, 0, &ext_count, nullptr);
        if (res != XR_SUCCESS && ext_count == 0)
        {
            return vector<XrExtensionProperties>();
        }
        vector<XrExtensionProperties> ep(ext_count, {XR_TYPE_EXTENSION_PROPERTIES});
        res = xrEnumerateInstanceExtensionProperties(nullptr, ext_count, &ext_count, ep.data());
        if (res != XR_SUCCESS)
        {
            return vector<XrExtensionProperties>();
        }
        return ep;
    }

    bool ext_supported(span<const XrExtensionProperties> ext_span, const XrExtensionProperties &ext)
    {
        for (const auto &el : ext_span)
        {
            if (!strcmp(ext.extensionName, el.extensionName) && ext.extensionVersion <= el.extensionVersion)
            {
                return true;
            }
        }
        return false;
    }

}

namespace xrh
{
    bool init_loader(JavaVM *vm, jobject ctx)
    {
        DECL_INIT_PFN(XR_NULL_HANDLE, xrInitializeLoaderKHR);
        if (xrInitializeLoaderKHR == nullptr)
        {
            return false;
        }
        {
            XrLoaderInitInfoAndroidKHR ii{XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR};
            ii.applicationVM = vm;
            ii.applicationContext = ctx;
            auto result = xrInitializeLoaderKHR(reinterpret_cast<XrLoaderInitInfoBaseHeaderKHR *>(&ii));
        }
        return true;
    }

    instance::instance()
    {
    }

    instance::~instance()
    {
    }

    void instance::add_required_extension(const char *extName, uint32_t ver)
    {
        ext.required.push_back(make_ext_prop(extName, ver));
    }

    void instance::add_desired_extension(const char *extName, uint32_t ver)
    {
        ext.desired.push_back(make_ext_prop(extName, ver));
    }

    bool instance::create()
    {
        ext.available = enum_extensions();
        ext.enabled.clear();
        bool foundRequired = true;
        for (auto &req : ext.required)
        {
            if (ext_supported(ext.available, req))
            {
                ext.enabled.push_back(req);
            }
            else
            {
                foundRequired = false;
                aout << "Required extension not supported: " << req.extensionName << "(v" << req.extensionVersion << ")" << endl;
            }
        }
        if (!foundRequired)
        {
            return false;
        }
        for (auto &des : ext.desired)
        {
            if (ext_supported(ext.available, des))
            {
                ext.enabled.push_back(des);
            }
            else
            {
                aout << "Desired extension not supported: " << des.extensionName << "(v" << des.extensionVersion << ")" << endl;
            }
        }
        vector<const char *> extNames;
        for (const auto &en : ext.enabled)
        {
            extNames.push_back(en.extensionName);
        }

        XrInstanceCreateInfo ci{XR_TYPE_INSTANCE_CREATE_INFO};
        ci.enabledExtensionCount = extNames.size();
        ci.enabledExtensionNames = extNames.data();
        inst = XR_NULL_HANDLE;
        auto res = XRH(xrCreateInstance(&ci, &inst));
        if (inst == XR_NULL_HANDLE)
        {
            return false;
        }

        instprops = ::get_instance_properties(inst);
        sysid = ::get_system_id(inst);
        sysprops = ::get_system_properties(inst, sysid);

        return true;
    }

    void instance::destroy()
    {
        if (inst == XR_NULL_HANDLE) {
            return;
        }
        XRH(xrDestroyInstance(inst));
        inst = XR_NULL_HANDLE;
    }
}