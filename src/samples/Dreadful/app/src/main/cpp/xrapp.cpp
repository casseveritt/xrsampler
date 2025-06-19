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
  set_ostream(&aout);
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

bool App::is_initialized() const {
  return bool(inst);
}

void App::frame() {
  if (!is_initialized()) {
    aout << "App is not initialized, cannot frame." << endl;
    return;
  }

  if (!ssn->begin_frame()) {
    // We can't begin a frame until the session is in a valid state.
    static int frameCount = 0;
    frameCount++;
    if (frameCount % 60 == 0) {
      aout << "Waiting for session to become synchronized, frame count: " << frameCount << endl;
    }
    return;
  }

  if (sc) {
    uint32_t imageIndex = sc->acquire_and_wait_image();

    // Render a frame
    renderer->render(imageIndex);

    // add a layer to be submitted at the end of the frame
    xrh::QuadLayer quad;
    double t = ssn->get_predicted_display_time() * 1e-9;  // Convert from nanoseconds to seconds

    quad.set_pose(Posef(Quatf(Vector3f(0, 0, 1), t), Vector3f(0, 0, -1)));
    quad.set_size(1.0f, 1.0f);  // Set the size of the quad layer
    quad.set_swapchain(sc);
    quad.set_space(local);
    ssn->add_layer(quad);

    sc->release_image();
  }

  ssn->end_frame();
}
}  // namespace xr