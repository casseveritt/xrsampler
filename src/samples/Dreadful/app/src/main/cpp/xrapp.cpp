#include "xrapp.h"

#include "AndroidOut.h"

using namespace xrh;
using namespace std;

namespace xr {
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
App::App(android_app* pApp)
    : app(pApp)
#else
App::App()
#endif
{
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
  renderer = make_shared<Renderer>(app);
  auto dpy = renderer->getDisplay();
  auto cfg = renderer->getConfig();
  auto ctx = renderer->getContext();
  // instance
  inst = make_instance();
  inst->add_required_extension(XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME);
  if (!inst->create()) {
    aout << "OpenXR instance creation failed, exiting." << endl;
    return;
  }
  inst->set_gfx_binding(dpy, cfg, ctx);
#else
  // instance
  inst = make_instance();
  inst->add_required_extension(XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME);
  if (!inst->create()) {
    aout << "OpenXR instance creation failed, exiting." << endl;
  }
#endif
  // session
  ssn = inst->create_session();

  // local space
  auto rsci = RefSpace::element_type::make_create_info();
  local = ssn->create_refspace(rsci);

  auto vcv = inst->get_xr_view_config_view(0);
  auto scci = Swapchain::element_type::make_create_info(vcv.recommendedImageRectWidth, vcv.recommendedImageRectHeight);
  sc = ssn->create_swapchain(scci);

  renderer->setSwapchainImages(sc->get_width(), sc->get_height(), sc->enumerate_images());
}

App::~App() {
  aout << "Destroying App instance." << inst.get() << endl;
}

App::RendererPtr App::get_renderer() {
  return renderer;
}

Session App::get_session() const {
  return ssn;
}

Space App::get_local() const {
  return local;
}

bool App::is_initialized() const {
  return bool(inst);
}

bool App::begin_frame() {
  return ssn->begin_frame();
}
Swapchain App::get_swapchain() const {
  return sc;
}

void App::add_layer(const Layer& layer) {
  ssn->add_layer(layer);
}

void App::end_frame() {
  ssn->end_frame();
}

}  // namespace xr