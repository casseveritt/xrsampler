// OpenXR Helper
// Author: Cass Everitt

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
#include <iostream>
#include <memory>
#include <set>
#include <span>
#include <vector>

#include "xrhlinear.h"
namespace xrh {

class InstanceOb;
class SessionOb;
class SpaceOb;
class RefSpaceOb;
class SwapchainOb;

using Instance = std::shared_ptr<InstanceOb>;
using Session = std::shared_ptr<SessionOb>;
using Space = std::shared_ptr<SpaceOb>;
using RefSpace = std::shared_ptr<RefSpaceOb>;
using Swapchain = std::shared_ptr<SwapchainOb>;

void set_ostream(std::ostream* out);
Instance make_instance();

#if defined(ANDROID)
// must be called before constructor
bool init_loader(JavaVM* vm, jobject ctx);
#endif

constexpr XrVector3f ZeroVec3{0, 0, 0};
constexpr XrQuaternionf IdentityQuat{0, 0, 0, 1};
constexpr XrPosef IdentityPose{IdentityQuat, ZeroVec3};

// Layer types
class Layer {
 public:
  enum class Type { Projection = 0, Quad = 1, Cylinder = 2, Equirect = 3, Cube = 4 };
  Layer(Type type_) : type(type_) {}
  virtual ~Layer();

  void set_swapchain(Swapchain sc, int index = 0) {
    if (index < 0 || index >= swapchains.size()) {
      // aout << __FUNCTION__ << " Invalid swapchain index: " << index << std::endl;
      return;
    }
    swapchains[index] = sc;
  }

  void set_space(Space space_) {
    space = space_;
  }

  void set_pose(const Posef& p) {
    pose = p;
  }

  Type type;
  Space space;
  std::array<Swapchain, 2> swapchains;
  Posef pose{IdentityPose};
};

class QuadLayer : public Layer {
 public:
  QuadLayer() : Layer(Type::Quad) {}

  void set_size(float widthMeters, float heightMeters) {
    width = widthMeters;
    height = heightMeters;
  }

  XrCompositionLayerQuad get_xr_quad_layer() const;

  float width{};
  float height{};
};

class InstanceOb : public std::enable_shared_from_this<InstanceOb> {
 public:
  InstanceOb();
  ~InstanceOb();

  void add_required_extension(const char* name, uint32_t ver = 1);
  void add_desired_extension(const char* name, uint32_t ver = 1);

  std::vector<XrExtensionProperties> get_available_extensions() const {
    return ext.available;
  }

  bool create();
  void destroy();

  // only available after successful create:

  XrInstance get_xr_instance() const {
    return inst;
  }

  XrInstanceProperties get_xr_instance_properties() const {
    return instprops;
  }

  XrSystemId get_xr_system_id() const {
    return sysid;
  }

  XrSystemProperties get_xr_system_properties() const {
    return sysprops;
  }

  std::vector<XrExtensionProperties> get_enabled_xr_extension_properties() const {
    return ext.enabled;
  }

#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
  void set_gfx_binding(EGLDisplay dpy, EGLConfig cfg, EGLContext ctx);
#endif

  Session create_session();

  const XrViewConfigurationView& get_xr_view_config_view(int eye) const {
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

class SessionOb : public std::enable_shared_from_this<SessionOb> {
 public:
  SessionOb(Instance inst_, XrSession ssn_);
  ~SessionOb();

  const Instance get_instance() const {
    return inst;
  }

  Space create_refspace(const XrReferenceSpaceCreateInfo& createInfo);
  Swapchain create_swapchain(const XrSwapchainCreateInfo& createInfo);

  bool begin_frame();
  XrTime get_predicted_display_time() const {
    return fs.predictedDisplayTime;
  }
  void add_layer(const Layer& layer);
  void end_frame();

 private:
  Instance inst;
  XrSession ssn;
  XrFrameState fs;
  XrSessionState state;
  std::set<XrReferenceSpaceType> refspacetypes;

  struct StereoProjectionLayer {
    XrCompositionLayerProjection proj;
    std::array<XrCompositionLayerProjectionView, 2> views;
  };
  union LayerUnion {
    XrCompositionLayerBaseHeader base;
    StereoProjectionLayer stereo;
    XrCompositionLayerQuad quad;
  };

  std::vector<LayerUnion> layers;
  std::vector<XrCompositionLayerBaseHeader*> layer_ptrs;
};

class SpaceOb {
 public:
  enum class Type {
    Reference = 0,
    Action = 1,
    SpatialAnchor = 2,
  };

  SpaceOb(Session ssn_, XrSpace space_, Type type_);
  ~SpaceOb();

  const Session get_session() const {
    return ssn;
  }

  Type get_type() const {
    return type;
  }

  XrSpace get_xr_space() const {
    return space;
  }

 private:
  Session ssn;
  XrSpace space;
  Type type;
};

class RefSpaceOb : public SpaceOb {
 public:
  using CreateInfo = XrReferenceSpaceCreateInfo;
  using RefType = XrReferenceSpaceType;
  static constexpr XrStructureType CIST = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
  static constexpr XrReferenceSpaceType Local = XR_REFERENCE_SPACE_TYPE_LOCAL;
  RefSpaceOb(Session ssn_, XrSpace space_, const CreateInfo& ci_);
  virtual ~RefSpaceOb();

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

class SwapchainOb {
 public:
  using CreateInfo = XrSwapchainCreateInfo;
  static constexpr XrStructureType CIST = XR_TYPE_SWAPCHAIN_CREATE_INFO;
  static constexpr int64_t SRGB_A = GL_SRGB8_ALPHA8;
  static constexpr uint64_t UsageSampled = XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
  static constexpr uint64_t UsageColorAttachment = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
  SwapchainOb(Session ssn_, XrSwapchain sc_, const CreateInfo& ci_);
  ~SwapchainOb();

  static constexpr CreateInfo make_create_info(uint32_t width, uint32_t height, int64_t format = SRGB_A) {
    return {CIST, nullptr, 0, UsageSampled | UsageColorAttachment, format, 1, width, height, 1, 1, 1};
  }

  XrSwapchain get_xr_swapchain() const {
    return swapchain;
  }

  uint32_t get_width() const {
    return ci.width;
  }

  uint32_t get_height() const {
    return ci.height;
  }

  XrExtent2Di get_extent() const {
    return {static_cast<int>(ci.width), static_cast<int>(ci.height)};
  }

#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
  const std::span<GLuint> enumerate_images() {
    return images;
  }
#endif

  uint32_t acquire_image() {
    XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    uint32_t imageIndex = 0;
    xrAcquireSwapchainImage(swapchain, &acquireInfo, &imageIndex);
    return imageIndex;
  }

  void wait_image() {
    XrSwapchainImageWaitInfo waitInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
    waitInfo.timeout = XR_INFINITE_DURATION;
    xrWaitSwapchainImage(swapchain, &waitInfo);
  }

  uint32_t acquire_and_wait_image() {
    uint32_t imageIndex = acquire_image();
    wait_image();
    return imageIndex;
  }

  void release_image() {
    XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
    xrReleaseSwapchainImage(swapchain, &releaseInfo);
  }

 private:
  Session ssn;
  XrSwapchain swapchain;
  CreateInfo ci;
  uint32_t chainlength;
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
  std::vector<GLuint> images;
#endif
};

}  // namespace xrh
