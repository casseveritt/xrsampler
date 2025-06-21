// the main app interface

#include <memory>

#include "GltfRenderer.h"
#include "Renderer.h"
#include "xrh.h"

namespace xr {
struct App {
  using RendererPtr = std::shared_ptr<Renderer>;

#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
  App(android_app* pApp);
#else
  App();
#endif

  ~App();

  bool is_initialized() const;
  void frame();

 private:
  android_app* app = nullptr;
  RendererPtr renderer;
  xrh::Instance inst;
  xrh::Session ssn;
  xrh::Space local;
  xrh::Swapchain sc;
  tinygltf::Model model;
  GltfRenderer gltfRenderer;
};

}  // namespace xr