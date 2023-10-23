#include "xrh.h"
#include "AndroidOut.h"

namespace xrh
{
    bool init_loader(JavaVM *vm, jobject ctx)
    {
        PFN_xrInitializeLoaderKHR xrInitializeLoaderKHR = nullptr;
        {
            auto result = xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR", reinterpret_cast<PFN_xrVoidFunction *>(&xrInitializeLoaderKHR));
            if (result != XR_SUCCESS)
            {
                aout << "xrGetInstanceProcAddr(xrInitializeLoaderKHR) failed" << std::endl;
                // Log error message
                return false;
            }
            aout << "got here: " << __FUNCTION__ << ":" << __LINE__ << std::endl;
        }
        {
            XrLoaderInitInfoAndroidKHR ii{XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR};
            ii.applicationVM = vm;
            ii.applicationContext = ctx;
            auto result = xrInitializeLoaderKHR(reinterpret_cast<XrLoaderInitInfoBaseHeaderKHR *>(&ii));
        }
        // until we complete the implementation of this function,
        // just return false...
        aout << "finish implementing this function" << std::endl;
        return false;
        // return true;
    }

    XrExtensionProperties make_ext_prop(const char* name, uint32_t ver) {
        XrExtensionProperties ep {XR_TYPE_EXTENSION_PROPERTIES};
        strcpy(ep.extensionName, name);
        ep.extensionVersion = ver;
        return ep;
    }


    std::vector<XrExtensionProperties> enum_extensions() {
        uint32_t ext_count = 0;
        auto res = xrEnumerateInstanceExtensionProperties( nullptr, 0, &ext_count, nullptr);
        if (res != XR_SUCCESS && ext_count == 0) {
            return std::vector<XrExtensionProperties>();
        }
        std::vector<XrExtensionProperties> ep(ext_count, {XR_TYPE_EXTENSION_PROPERTIES});
        res = xrEnumerateInstanceExtensionProperties( nullptr, ext_count, &ext_count, ep.data());
        if (res != XR_SUCCESS) {
            return std::vector<XrExtensionProperties>();
        }
        return ep;
    }

    bool ext_supported(std::span<const XrExtensionProperties> ext_span, const XrExtensionProperties& ext) {
        for (const auto& el : ext_span) {
            if (!strcmp(ext.extensionName, el.extensionName) && ext.extensionVersion <= el.extensionVersion) {
                return true;
            }
        }
        return false;
    }

    XrInstance create_instance(const std::vector<XrExtensionProperties>& required, std::vector<XrExtensionProperties>& desired) {
        auto available = enum_extensions();
        bool foundRequired = true;
        for (auto& req : required) {
            if (!ext_supported(available, req)) {
                foundRequired = false;
                aout << "Required extension not supported: " << req.extensionName << "(v" << req.extensionVersion << ")" << std::endl;
            }
        }
        if (!foundRequired) {
            return XR_NULL_HANDLE;
        }
        for (size_t i = 0; i < desired.size(); ++i) {
            XrExtensionProperties ep = desired[i];
            if (!ext_supported(available, ep)) {
                aout << "Desired extension not supported: " << ep.extensionName << "(v" << ep.extensionVersion << ")" << std::endl;
                desired.erase(desired.begin() + i);
                --i;
                continue;
            }
        }
        std::vector<const char*> extNames;
        for (const auto& req: required) {
            extNames.push_back(req.extensionName);
        }
        for (const auto& des: desired) {
            extNames.push_back(des.extensionName);
        }

        XrInstanceCreateInfo ci{XR_TYPE_INSTANCE_CREATE_INFO};
        ci.enabledExtensionCount = extNames.size();
        ci.enabledExtensionNames = extNames.data();
        XrInstance inst = XR_NULL_HANDLE;
        auto res = xrCreateInstance(&ci, &inst);
        if (res != XR_SUCCESS) {
            aout << "xrCreateInstance failed (" << res << ")" << std::endl;
            return XR_NULL_HANDLE;
        }
        return inst;
    }
}