// This file is part of the OpenXR Dreadful sample application.
// Author: Cass Everitt

#include <game-activity/GameActivity.cpp>
#include <game-text-input/gametextinput.cpp>
#include <span>

#include "AndroidOut.h"
#include "Renderer.h"
#include "xrh.h"
#include "xrhlinear.h"

using namespace std;
using namespace xrh;

extern "C" {

#include <game-activity/native_app_glue/android_native_app_glue.c>

/*!
 * Handles commands sent to this Android application
 * @param pApp the app the commands are coming from
 * @param cmd the command to handle
 */
void handle_cmd(android_app* pApp, int32_t cmd) {
  switch (cmd) {
    case APP_CMD_INIT_WINDOW:
      // A new window is created, associate a renderer with it. You may replace this with a
      // "game" class if that suits your needs. Remember to change all instances of userData
      // if you change the class here as a reinterpret_cast is dangerous this in the
      // android_main function and the APP_CMD_TERM_WINDOW handler case.
      pApp->userData = new Renderer(pApp);
      break;
    case APP_CMD_TERM_WINDOW:
      // The window is being destroyed. Use this to clean up your userData to avoid leaking
      // resources.
      //
      // We have to check if userData is assigned just in case this comes in really quickly
      if (pApp->userData) {
        //
        auto* pRenderer = reinterpret_cast<Renderer*>(pApp->userData);
        pApp->userData = nullptr;
        delete pRenderer;
      }
      break;
    default:
      break;
  }
}

/*!
 * Enable the motion events you want to handle; not handled events are
 * passed back to OS for further processing. For this example case,
 * only pointer and joystick devices are enabled.
 *
 * @param motionEvent the newly arrived GameActivityMotionEvent.
 * @return true if the event is from a pointer or joystick device,
 *         false for all other input devices.
 */
bool motion_event_filter_func(const GameActivityMotionEvent* motionEvent) {
  auto sourceClass = motionEvent->source & AINPUT_SOURCE_CLASS_MASK;
  return (sourceClass == AINPUT_SOURCE_CLASS_POINTER || sourceClass == AINPUT_SOURCE_CLASS_JOYSTICK);
}

namespace {
struct Xr {
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
  bool init(EGLDisplay dpy, EGLConfig cfg, EGLContext ctx)
#else
  bool init()
#endif
  {
    // instance
    inst = make_instance();
    inst->add_required_extension(XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME);
    if (!inst->create()) {
      aout << "OpenXR instance creation failed, exiting." << endl;
      return false;
    }
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
    inst->set_gfx_binding(dpy, cfg, ctx);
#endif
    // session
    ssn = inst->create_session();

    // local space
    auto rsci = RefSpace::element_type::make_create_info();
    local = ssn->create_refspace(rsci);

    // swapchain
    auto vcv = inst->get_xr_view_config_view(0);
    auto scci = Swapchain::element_type::make_create_info(vcv.recommendedImageRectWidth, vcv.recommendedImageRectHeight);
    sc = ssn->create_swapchain(scci);
    return true;
  }

  bool is_initialized() const {
    return bool(inst);
  }

  bool begin_frame() {
    return ssn->begin_frame();
  }

  void add_layer(const Layer& layer) {
    ssn->add_layer(layer);
  }

  void end_frame() {
    ssn->end_frame();
  }

  Instance inst;
  Session ssn;
  Space local;
  Swapchain sc;
};

}  // namespace

/*!
 * This the main entry point for a native activity
 */
void android_main(struct android_app* pApp) {
  // Can be removed, useful to ensure your code is running
  aout << "Welcome to android_main" << endl;

  // Register an event handler for Android events
  pApp->onAppCmd = handle_cmd;

  if (!init_loader(pApp->activity->vm, pApp->activity->javaGameActivity)) {
    aout << "OpenXR loader initialization failed, exiting" << endl;
    return;
  }

  Xr xr;

  // This sets up a typical game/event loop. It will run until the app is destroyed.
  int events;
  android_poll_source* pSource;
  do {
    // Process all pending events before running game logic.
    if (ALooper_pollAll(0, nullptr, &events, (void**)&pSource) >= 0) {
      if (pSource) {
        pSource->process(pApp, pSource);
      }
    }

    // Check if any user data is associated. This is assigned in handle_cmd
    if (pApp->userData) {
      // We know that our user data is a Renderer, so reinterpret cast it. If you change your
      // user data remember to change it here
      auto* pRenderer = reinterpret_cast<Renderer*>(pApp->userData);

      // Process game input
      pRenderer->handleInput();

      if (!xr.is_initialized()) {
        xr.init(pRenderer->getDisplay(), pRenderer->getConfig(), pRenderer->getContext());
        // I need to get the swapchain, enumerate the images, and pass the corresponding
        // texture objects to the renderer.

        // Assuming Swapchain has a method enumerate_images() returning std::vector<GLuint>
        const std::span<GLuint> textures = xr.sc->enumerate_images();
        pRenderer->setSwapchainImages(xr.sc->get_width(), xr.sc->get_height(), textures);
      }

      if (!xr.begin_frame()) {
        // We can't begin a frame until the session is in a valid state.
        static int frameCount = 0;
        frameCount++;
        if (frameCount % 60 == 0) {
          aout << "Waiting for session to become synchronized, frame count: " << frameCount << endl;
        }
        continue;
      }

      // Acquire the next image index for the swapchain
      uint32_t imageIndex = xr.sc->acquire_and_wait_image();

      // Render a frame
      pRenderer->render(imageIndex);

      xr.sc->release_image();

      // add a layer to be submitted at the end of the frame
      xrh::QuadLayer quad;
      quad.set_pose(Posef(Quatf::Identity(), Vector3f(0, 0, -1)));
      quad.set_size(1.0f, 1.0f);  // Set the size of the quad layer
      quad.set_swapchain(xr.sc);
      quad.set_space(xr.local);
      xr.add_layer(quad);

      xr.end_frame();
    }
  } while (!pApp->destroyRequested);
}
}