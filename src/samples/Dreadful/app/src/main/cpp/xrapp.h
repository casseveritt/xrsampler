// the main app interface

#include <memory>

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

  RendererPtr get_renderer();
  xrh::Session get_session() const;
  xrh::Space get_local() const;
  bool is_initialized() const;
  bool begin_frame();
  xrh::Swapchain get_swapchain() const;
  void add_layer(const xrh::Layer& layer);
  void end_frame();

  void frame();

 private:
  android_app* app = nullptr;
  RendererPtr renderer;
  xrh::Instance inst;
  xrh::Session ssn;
  xrh::Space local;
  xrh::Swapchain sc;
};

}  // namespace xr