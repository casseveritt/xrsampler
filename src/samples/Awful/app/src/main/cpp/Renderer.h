#ifndef ANDROIDGLINVESTIGATIONS_RENDERER_H
#define ANDROIDGLINVESTIGATIONS_RENDERER_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#include <memory>
#include <span>

#include "Model.h"
#include "Shader.h"

struct android_app;

class Renderer {
 public:
  /*!
   * @param pApp the android_app this Renderer belongs to, needed to configure GL
   */
  inline Renderer(android_app* pApp)
      : app_(pApp), display_(EGL_NO_DISPLAY), config_(0), surface_(EGL_NO_SURFACE), context_(EGL_NO_CONTEXT) {
    initRenderer();
  }

  virtual ~Renderer();

  /*!
   * Sets the swap chain images for the renderer.
   * @param width The width of the swap chain images.
   * @param height The height of the swap chain images.
   * @param images A span of GLuint handles representing the swap chain images.
   */
  void setSwapchainImages(uint32_t width, uint32_t height, const std::span<GLuint>& images);

  /*!
   * Renders all the models in the renderer to the specified image.
   */
  void render();

  EGLDisplay getDisplay() {
    return display_;
  }

  EGLConfig getConfig() {
    return config_;
  }

  EGLContext getContext() {
    return context_;
  }

  void bindFbo(uint32_t imageIndex);
  void unbindFbo();

 private:
  /*!
   * Performs necessary OpenGL initialization. Customize this if you want to change your EGL
   * context or application-wide settings.
   */
  void initRenderer();

  /*!
   * @brief we have to check every frame to see if the framebuffer has changed in size. If it has,
   * update the viewport accordingly
   */
  void updateRenderArea();

  /*!
   * Creates the models for this sample. You'd likely load a scene configuration from a file or
   * use some other setup logic in your full game.
   */
  void createModels();

  android_app* app_;
  EGLDisplay display_;
  EGLConfig config_;
  EGLSurface surface_;
  EGLContext context_;

  std::unique_ptr<Shader> shader_;
  std::vector<Model> models_;

  struct SwapchainImage {
    GLuint textureId;
    uint32_t width;
    uint32_t height;
  };

  std::vector<SwapchainImage> colorImages_;
  std::vector<SwapchainImage> depthImages_;
  GLuint fbo;
};

#endif  // ANDROIDGLINVESTIGATIONS_RENDERER_H