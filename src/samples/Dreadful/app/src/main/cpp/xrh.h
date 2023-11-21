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

constexpr XrVector3f ZeroVec3{0, 0, 0};
constexpr XrQuaternionf IdentityQuat{0, 0, 0, 1};
constexpr XrPosef IdentityPose{IdentityQuat, ZeroVec3};

class Session;
class Instance {
 public:
  Instance();
  ~Instance();

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

  Session* create_session();

  const XrViewConfigurationView& get_view_config_view(int eye) const {
    return view_config_views[std::clamp(eye, 0, 1)];
  }

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
  bool fov_mutable = false;
  std::array<XrViewConfigurationView, 2> view_config_views;
};

class Space;
class Swapchain;

class Session {
 public:
  Session(Instance* inst_, XrSession ssn_);
  ~Session();

  const Instance* get_instance() const {
    return inst;
  }

  Space* create_refspace(const XrReferenceSpaceCreateInfo& createInfo);
  Swapchain* create_swapchain(const XrSwapchainCreateInfo& createInfo);

 private:
  Instance* inst;
  XrSession ssn;
  std::set<XrReferenceSpaceType> refspacetypes;
};

class Space {
 public:
  enum class Type {
    Reference = 0,
    Action = 1,
    SpatialAnchor = 2,
  };

  Space(Session* ssn_, XrSpace space_, Type type_);
  ~Space();

  const Session* get_session() const {
    return ssn;
  }

  Type get_type() const {
    return type;
  }

  XrSpace get_space() const {
    return space;
  }

 private:
  Session* ssn;
  XrSpace space;
  Type type;
};

class RefSpace : public Space {
 public:
  using CreateInfo = XrReferenceSpaceCreateInfo;
  using RefType = XrReferenceSpaceType;
  static constexpr XrStructureType CIST = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
  static constexpr XrReferenceSpaceType Local = XR_REFERENCE_SPACE_TYPE_LOCAL;
  RefSpace(Session* ssn_, XrSpace space_, const CreateInfo& ci_);
  virtual ~RefSpace();

  RefType get_reftype() const {
    return reftype;
  }

  static CreateInfo make_create_info(RefType reftype = Local) {
    return {CIST, nullptr, reftype, IdentityPose};
  }

 private:
  RefType reftype;
  CreateInfo ci;
};

class Swapchain {
 public:
  using CreateInfo = XrSwapchainCreateInfo;
  static constexpr XrStructureType CIST = XR_TYPE_SWAPCHAIN_CREATE_INFO;
  static constexpr int64_t SRGB_A = GL_SRGB8_ALPHA8;
  static constexpr uint64_t UsageSampled = XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
  static constexpr uint64_t UsageColorAttachment = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
  Swapchain(Session* ssn_, XrSwapchain swapchain_, const CreateInfo& ci_);
  ~Swapchain();

  static CreateInfo make_create_info(uint32_t width, uint32_t height, int64_t format = SRGB_A) {
    return {CIST, nullptr, 0, UsageSampled | UsageColorAttachment, format, 1, width, height, 1, 1, 1};
  }
 private:
  Session* ssn;
  XrSwapchain swapchain;
  CreateInfo ci;
  uint32_t chainlength;
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
#endif
};

}  // namespace xrh
