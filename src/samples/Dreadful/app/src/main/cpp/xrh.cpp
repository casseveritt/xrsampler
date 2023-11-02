#include "xrh.h"
#include "AndroidOut.h"

using namespace std;

void XRH_CheckErrors(XrResult result, const char *function, bool failOnError)
{
    if (XR_FAILED(result))
    {
        char errorBuffer[XR_MAX_RESULT_STRING_SIZE];
        xrResultToString(XR_NULL_HANDLE, result, errorBuffer);
        aout << "OpenXR error: " << function << ": (" << result << ") " << errorBuffer << endl;
    }
}

#define DECL_LOC_PFN(pfn) PFN_##pfn pfn = nullptr
#define INIT_PFN(inst, pfn) XRH(xrGetInstanceProcAddr(inst, #pfn, reinterpret_cast<PFN_xrVoidFunction *>(&pfn)))

#define DECL_INIT_PFN(inst, pfn) \
    DECL_LOC_PFN(pfn);         \
    INIT_PFN(inst, pfn)

namespace
{
    void get_instance_properties(XrInstance inst)
    {
        XrInstanceProperties ii = {XR_TYPE_INSTANCE_PROPERTIES};
        XRH(xrGetInstanceProperties(inst, &ii));
        aout << "Runtime: " << ii.runtimeName << "Version: "
             << XR_VERSION_MAJOR(ii.runtimeVersion) << "."
             << XR_VERSION_MINOR(ii.runtimeVersion) << "."
             << XR_VERSION_PATCH(ii.runtimeVersion) << endl;
    }

    void get_system_properties(XrInstance inst)
    {

        XrSystemGetInfo sysGetInfo = {XR_TYPE_SYSTEM_GET_INFO};
        sysGetInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

        XrSystemId sysid;
        XrResult res;
        XRH(res = xrGetSystem(inst, &sysGetInfo, &sysid));
        if (res != XR_SUCCESS)
        {
            aout << "Failed to get system." << endl;
            exit(1);
        }

        XrSystemProperties sysprops = {XR_TYPE_SYSTEM_PROPERTIES};
        XRH(xrGetSystemProperties(inst, sysid, &sysprops));

        aout << "System Properties: Name=" << sysprops.systemName
             << " VendorId=" << sysprops.vendorId << endl;
        aout << "System Graphics Properties:"
             << " MaxWidth=" << sysprops.graphicsProperties.maxSwapchainImageWidth
             << " MaxHeight=" << sysprops.graphicsProperties.maxSwapchainImageHeight
             << " MaxLayers=" << sysprops.graphicsProperties.maxLayerCount << endl;
    }

}

namespace xrh
{
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
    DECL_LOC_PFN(xrGetOpenGLESGraphicsRequirementsKHR);
#endif

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

    XrExtensionProperties make_ext_prop(const char *name, uint32_t ver)
    {
        XrExtensionProperties ep{XR_TYPE_EXTENSION_PROPERTIES};
        strcpy(ep.extensionName, name);
        ep.extensionVersion = ver;
        return ep;
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

    XrInstance create_instance(const vector<XrExtensionProperties> &required, vector<XrExtensionProperties> &desired)
    {
        auto available = enum_extensions();
        bool foundRequired = true;
        for (auto &req : required)
        {
            if (!ext_supported(available, req))
            {
                foundRequired = false;
                aout << "Required extension not supported: " << req.extensionName << "(v" << req.extensionVersion << ")" << endl;
            }
        }
        if (!foundRequired)
        {
            return XR_NULL_HANDLE;
        }
        for (size_t i = 0; i < desired.size(); ++i)
        {
            XrExtensionProperties ep = desired[i];
            if (!ext_supported(available, ep))
            {
                aout << "Desired extension not supported: " << ep.extensionName << "(v" << ep.extensionVersion << ")" << endl;
                desired.erase(desired.begin() + i);
                --i;
                continue;
            }
        }
        vector<const char *> extNames;
        for (const auto &req : required)
        {
            extNames.push_back(req.extensionName);
        }
        for (const auto &des : desired)
        {
            extNames.push_back(des.extensionName);
        }

        XrInstanceCreateInfo ci{XR_TYPE_INSTANCE_CREATE_INFO};
        ci.enabledExtensionCount = extNames.size();
        ci.enabledExtensionNames = extNames.data();
        XrInstance inst = XR_NULL_HANDLE;
        auto res = xrCreateInstance(&ci, &inst);
        if (res != XR_SUCCESS)
        {
            aout << "xrCreateInstance failed (" << res << ")" << endl;
            return XR_NULL_HANDLE;
        }

        get_instance_properties(inst);
        get_system_properties(inst);

        return inst;
    }
}