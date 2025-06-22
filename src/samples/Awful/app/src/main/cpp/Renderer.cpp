#include "Renderer.h"

#include <GLES3/gl3.h>
#include <android/imagedecoder.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>

#include <memory>
#include <vector>

#include "AndroidOut.h"
#include "Shader.h"
#include "TextureAsset.h"
#include "linear.h"
#include "tiny_gltf.h"

using namespace std;

//! executes glGetString and outputs the result to logcat
#define PRINT_GL_STRING(s) \
  { aout << #s ": " << glGetString(s) << endl; }

/*!
 * @brief if glGetString returns a space separated list of elements, prints each one on a new line
 *
 * This works by creating an istringstream of the input c-style string. Then that is used to create
 * a vector -- each element of the vector is a new element in the input string. Finally a foreach
 * loop consumes this and outputs it to logcat using @a aout
 */
#define PRINT_GL_STRING_AS_LIST(s)                                                                       \
  {                                                                                                      \
    istringstream extensionStream((const char*)glGetString(s));                                          \
    vector<string> extensionList(istream_iterator<string>{extensionStream}, istream_iterator<string>()); \
    aout << #s ":\n";                                                                                    \
    for (auto& extension : extensionList) {                                                              \
      aout << extension << "\n";                                                                         \
    }                                                                                                    \
    aout << endl;                                                                                        \
  }

//! Color for cornflower blue. Can be sent directly to glClearColor
#define CORNFLOWER_BLUE 100 / 255.f, 149 / 255.f, 237 / 255.f, 1

// Vertex shader, you'd typically load this from assets
static const char* vertex = R"vertex(#version 300 es
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;

out vec2 fragUV;

uniform mat4 uToClipFromObject;

void main() {
    fragUV = inUV;
    gl_Position = uToClipFromObject * vec4(inPosition, 1.0);
}
)vertex";

// Fragment shader, you'd typically load this from assets
static const char* fragment = R"fragment(#version 300 es
precision mediump float;

in vec2 fragUV;

uniform sampler2D uTexture;

out vec4 outColor;

void main() {
    outColor = texture(uTexture, fragUV);
}
)fragment";

/*!
 * Half the height of the projection matrix. This gives you a renderable area of height 4 ranging
 * from -2 to 2
 */
static constexpr float kProjectionHalfHeight = 2.f;

/*!
 * The near plane distance for the projection matrix. Since this is an orthographic projection
 * matrix, it's convenient to have negative values for sorting (and avoiding z-fighting at 0).
 */
static constexpr float kProjectionNearPlane = -1.f;

/*!
 * The far plane distance for the projection matrix. Since this is an orthographic porjection
 * matrix, it's convenient to have the far plane equidistant from 0 as the near plane.
 */
static constexpr float kProjectionFarPlane = 1.f;

Renderer::~Renderer() {
  if (display_ != EGL_NO_DISPLAY) {
    eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (context_ != EGL_NO_CONTEXT) {
      eglDestroyContext(display_, context_);
      context_ = EGL_NO_CONTEXT;
    }
    if (surface_ != EGL_NO_SURFACE) {
      eglDestroySurface(display_, surface_);
      surface_ = EGL_NO_SURFACE;
    }
    eglTerminate(display_);
    display_ = EGL_NO_DISPLAY;
  }
}

void Renderer::bindFbo(uint32_t imageIndex) {
  // Make sure we have a valid context
  if (context_ == EGL_NO_CONTEXT || display_ == EGL_NO_DISPLAY || surface_ == EGL_NO_SURFACE) {
    aout << "Renderer::bindFbo() called without a valid EGL context, display, or surface" << endl;
    return;
  }

  if (imageIndex >= colorImages_.size()) {
    aout << "Invalid image index: " << imageIndex << ", numImages: " << colorImages_.size() << endl;
    return;
  }

  // Configure the fbo
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorImages_[imageIndex].textureId, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthImages_[imageIndex].textureId, 0);
  // Check FBO completeness
  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    aout << "Framebuffer not complete: 0x" << std::hex << status << std::dec << endl;
    return;
  }
}

void Renderer::render() {
  glViewport(0, 0, colorImages_[0].width, colorImages_[0].height);

  static int frameCount = 0;
  frameCount++;
  {
    float t = frameCount / 60.f;
    glClearColor(sin(1.7212 * t + 1.813) * 0.5f + 0.5f, sin(0.6212 * t + 2.13) * 0.5f + 0.5f,
                 sin(0.7612 * t + .213) * 0.5f + 0.5f, 0.5f);
  }

  // clear the color and depth buffers
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  shader_->activate();

  // Render all the models. There's no depth testing in this sample so they're accepted in the
  // order provided. But the sample EGL setup requests a 24 bit depth buffer so you could
  // configure it at the end of initRenderer
  if (!models_.empty()) {
    for (const auto& model : models_) {
      shader_->drawModel(model);
    }
  }
}

void Renderer::unbindFbo() {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::initRenderer() {
  // Choose your render attributes
  constexpr EGLint attribs[] = {EGL_RENDERABLE_TYPE,
                                EGL_OPENGL_ES3_BIT,
                                EGL_SURFACE_TYPE,
                                EGL_WINDOW_BIT,
                                EGL_BLUE_SIZE,
                                8,
                                EGL_GREEN_SIZE,
                                8,
                                EGL_RED_SIZE,
                                8,
                                EGL_DEPTH_SIZE,
                                24,
                                EGL_NONE};

  // The default display is probably what you want on Android
  auto display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  eglInitialize(display, nullptr, nullptr);

  EGLint numConfigs = 0;
  if (eglGetConfigs(display, nullptr, 0, &numConfigs) == EGL_FALSE) {
    aout << "eglGetConfigs() failed" << endl;
    return;
  }
  vector<EGLConfig> configs(numConfigs);
  if (eglGetConfigs(display, configs.data(), numConfigs, &numConfigs) == EGL_FALSE) {
    aout << "eglGetConfigs() failed" << endl;
    return;
  }
  const EGLint configAttribs[] = {EGL_RED_SIZE,
                                  8,
                                  EGL_GREEN_SIZE,
                                  8,
                                  EGL_BLUE_SIZE,
                                  8,
                                  EGL_ALPHA_SIZE,
                                  8,  // need alpha for alpha blending layers
                                  EGL_DEPTH_SIZE,
                                  0,
                                  EGL_STENCIL_SIZE,
                                  0,
                                  EGL_SAMPLES,
                                  0,
                                  EGL_NONE};
  EGLConfig config = 0;
  for (int i = 0; i < numConfigs; i++) {
    auto& cfg = configs[i];
    EGLint value = 0;

    eglGetConfigAttrib(display, cfg, EGL_RENDERABLE_TYPE, &value);
    if ((value & EGL_OPENGL_ES3_BIT_KHR) != EGL_OPENGL_ES3_BIT_KHR) {
      continue;
    }

    // The pbuffer config also needs to be compatible with normal window rendering
    // so it can share textures with the window context.
    eglGetConfigAttrib(display, cfg, EGL_SURFACE_TYPE, &value);
    if ((value & EGL_PBUFFER_BIT) != EGL_PBUFFER_BIT) {
      continue;
    }

    int j = 0;
    for (; configAttribs[j] != EGL_NONE; j += 2) {
      eglGetConfigAttrib(display, cfg, configAttribs[j], &value);
      if (value != configAttribs[j + 1]) {
        break;
      }
    }
    if (configAttribs[j] == EGL_NONE) {
      config = configs[i];
      break;
    }
  }
  if (config == 0) {
    aout << "failed to find suitable EGLConfig" << endl;
    return;
  }

  EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
  EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
  if (context == EGL_NO_CONTEXT) {
    aout << "eglCreateContext() failed" << endl;
    return;
  }
  const EGLint surfaceAttribs[] = {EGL_WIDTH, 16, EGL_HEIGHT, 16, EGL_NONE};
  EGLSurface vestigialSurface = eglCreatePbufferSurface(display, config, surfaceAttribs);
  if (vestigialSurface == EGL_NO_SURFACE) {
    aout << "eglCreatePbufferSurface() failed" << endl;
    eglDestroyContext(display, context);
    return;
  }
  if (eglMakeCurrent(display, vestigialSurface, vestigialSurface, context) == EGL_FALSE) {
    aout << "eglMakeCurrent() failed" << endl;
    eglDestroySurface(display, vestigialSurface);
    eglDestroyContext(display, context);
    return;
  }

  display_ = display;
  surface_ = vestigialSurface;
  context_ = context;

  PRINT_GL_STRING(GL_VENDOR);
  PRINT_GL_STRING(GL_RENDERER);
  PRINT_GL_STRING(GL_VERSION);
  PRINT_GL_STRING_AS_LIST(GL_EXTENSIONS);

  // Create a framebuffer object to render to
  glGenFramebuffers(1, &fbo);

  shader_ = unique_ptr<Shader>(Shader::loadShader(vertex, fragment, "inPosition", "inUV", "uToClipFromObject"));

  shader_->activate();
  r3::Matrix4f toClipFromObject = r3::Matrix4f::Identity();
  shader_->setToClipFromObject(toClipFromObject);

  // setup any other gl related global states
  glClearColor(CORNFLOWER_BLUE);

  // enable alpha globally for now, you probably don't want to do this in a game
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // get some demo models into memory
  createModels();
}

/**
 * @brief Create any demo models we want for this demo.
 */
void Renderer::createModels() {
  /*
   * This is a square:
   * 3 --- 2
   * |   / |
   * |  /  |
   * | /   |
   * 0 --- 1
   */
  vector<Vertex> vertices = {
      Vertex(r3::Vec3f{1, 1, 0}, r3::Vec2f{0, 0}),    // 0
      Vertex(r3::Vec3f{-1, 1, 0}, r3::Vec2f{1, 0}),   // 1
      Vertex(r3::Vec3f{-1, -1, 0}, r3::Vec2f{1, 1}),  // 2
      Vertex(r3::Vec3f{1, -1, 0}, r3::Vec2f{0, 1})    // 3
  };
  vector<Index> indices = {0, 1, 2, 0, 2, 3};

  // loads an image and assigns it to the square.
  //
  // Note: there is no texture management in this sample, so if you reuse an image be careful not
  // to load it repeatedly. Since you get a shared_ptr you can safely reuse it in many models.
  auto assetManager = app_->activity->assetManager;
  auto spAndroidRobotTexture = TextureAsset::loadAsset(assetManager, "android_robot.png");

  // Create a model and put it in the back of the render list.
  models_.emplace_back(vertices, indices, spAndroidRobotTexture);
}

void Renderer::setSwapchainImages(uint32_t width, uint32_t height, const std::span<GLuint>& images) {
  // Populate the swapchainImages vector with the provided images
  colorImages_.reserve(images.size());
  depthImages_.reserve(images.size());
  for (auto& image : images) {
    colorImages_.push_back({image, width, height});
    // Create a depth texture for each swapchain image
    GLuint depthTex = 0;
    glGenTextures(1, &depthTex);
    glBindTexture(GL_TEXTURE_2D, depthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    depthImages_.push_back({depthTex, width, height});
  }
}
