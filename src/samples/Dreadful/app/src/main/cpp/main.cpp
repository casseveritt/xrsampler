// This file is part of the OpenXR Dreadful sample application.
// Author: Cass Everitt

#include <game-activity/GameActivity.cpp>
#include <game-text-input/gametextinput.cpp>
#include <span>

#include "AndroidOut.h"
#include "Renderer.h"
#include "xrapp.h"
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
      aout << "APP_CMD_INIT_WINDOW" << endl;
      pApp->userData = new xr::App(pApp);
      break;
    case APP_CMD_TERM_WINDOW:
      // The window is being destroyed. Use this to clean up your userData to avoid leaking
      // resources.
      //
      // We have to check if userData is assigned just in case this comes in really quickly
      aout << "APP_CMD_TERM_WINDOW" << endl;
      if (pApp->userData) {
        //
        auto* pxr = reinterpret_cast<xr::App*>(pApp->userData);

        pApp->userData = nullptr;
        delete pxr;
      }
      break;
    default:
      aout << "Unhandled command: " << cmd << endl;
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

/*!
 * This the main entry point for a native activity
 */
void android_main(struct android_app* pApp) {
  // Register an event handler for Android events
  pApp->onAppCmd = handle_cmd;

  if (!init_loader(pApp->activity->vm, pApp->activity->javaGameActivity)) {
    aout << "OpenXR loader initialization failed, exiting" << endl;
    return;
  }

  // This sets up a typical game/event loop. It will run until the app is destroyed.
  int events;
  android_poll_source* pSource;
  do {
    // Process all pending events before running game logic.
    if (ALooper_pollOnce(0, nullptr, &events, (void**)&pSource) >= 0) {
      if (pSource) {
        pSource->process(pApp, pSource);
      }
    }

    // Check if any user data is associated. This is assigned in handle_cmd
    if (!pApp->userData) {
      continue;
    }
    // We know that our user data is a Renderer, so reinterpret cast it. If you change your
    // user data remember to change it here
    auto& xrapp = *reinterpret_cast<xr::App*>(pApp->userData);
    xrapp.frame();
  } while (!pApp->destroyRequested);
}
}