#pragma once

#if defined(ANDROID)
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <jni.h>
#define XR_USE_GRAPHICS_API_OPENGL_ES 1
#define XR_USE_PLATFORM_ANDROID 1
#endif

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <array>
#include <set>
#include <vector>
namespace xrh {
#if defined(ANDROID)
// must be called before constructor
bool init_loader(JavaVM* vm, jobject ctx);
#endif

class session;
class instance {
 public:
  instance();
  ~instance();

  void add_required_extension(const char* name, uint32_t ver = 1);
  void add_desired_extension(const char* name, uint32_t ver = 1);

  std::vector<XrExtensionProperties> get_available_extensions() const {
    return ext.available;
  }

  bool create();
  void destroy();

  // only available after successful create:

  XrInstance get_instance() const {
    return inst;
  }

  XrInstanceProperties get_instance_properties() const {
    return instprops;
  }

  XrSystemId get_system_id() const {
    return sysid;
  }

  XrSystemProperties get_system_properties() const {
    return sysprops;
  }

  std::vector<XrExtensionProperties> get_enabled_extensions() const {
    return ext.enabled;
  }

#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
  void set_gfx_binding(EGLDisplay dpy, EGLConfig cfg, EGLContext ctx);
#endif

  session* create_session();
  void session_destroyed(session* sess);

 private:
  struct extensions {
    std::vector<XrExtensionProperties> available;
    std::vector<XrExtensionProperties> required;
    std::vector<XrExtensionProperties> desired;
    std::vector<XrExtensionProperties> enabled;
  };

  extensions ext;
  XrInstance inst = XR_NULL_HANDLE;
  XrInstanceProperties instprops;
  XrSystemId sysid = XR_NULL_SYSTEM_ID;
  XrSystemProperties sysprops;
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
  XrGraphicsRequirementsOpenGLESKHR gfxreqs;
  XrGraphicsBindingOpenGLESAndroidKHR gfxbinding;
#endif
  session* ssn = nullptr;
  bool fov_mutable = false;
  std::array<XrViewConfigurationView, 2> view_config_views;
};

class session {
 public:
  session(instance* instptr, XrSession sess);
  ~session();

 private:
  instance* inst;
  XrSession ssn;
  std::set<XrReferenceSpaceType> refspacetypes;
};

}  // namespace xrh
