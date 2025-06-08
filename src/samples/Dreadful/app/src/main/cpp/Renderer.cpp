#include "Renderer.h"

#include <GLES3/gl3.h>
#include <android/imagedecoder.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>

#include <memory>
#include <vector>

#include "AndroidOut.h"
#include "Shader.h"
#include "TextureAsset.h"

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
in vec3 inPosition;
in vec2 inUV;

out vec2 fragUV;

uniform mat4 uProjection;

void main() {
    fragUV = inUV;
    gl_Position = uProjection * vec4(inPosition, 1.0);
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

void Renderer::render(uint32_t imageIndex) {
  // Make sure we have a valid context
  if (context_ == EGL_NO_CONTEXT || display_ == EGL_NO_DISPLAY || surface_ == EGL_NO_SURFACE) {
    aout << "Renderer::render() called without a valid EGL context, display, or surface" << endl;
    return;
  }

  if (imageIndex >= colorImages_.size()) {
    aout << "Invalid image index: " << imageIndex << endl;
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

  glViewport(0, 0, colorImages_[imageIndex].width, colorImages_[imageIndex].height);

  // clear the color and depth buffers
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Render all the models. There's no depth testing in this sample so they're accepted in the
  // order provided. But the sample EGL setup requests a 24 bit depth buffer so you could
  // configure it at the end of initRenderer
  if (!models_.empty()) {
    for (const auto& model : models_) {
      shader_->drawModel(model);
    }
  }

  // Unbind the fbo
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

  shader_ = unique_ptr<Shader>(Shader::loadShader(vertex, fragment, "inPosition", "inUV", "uProjection"));

  // Note: there's only one shader in this demo, so I'll activate it here. For a more complex game
  // you'll want to track the active shader and activate/deactivate it as necessary
  shader_->activate();

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
      Vertex(Vector3{1, 1, 0}, Vector2{0, 0}),    // 0
      Vertex(Vector3{-1, 1, 0}, Vector2{1, 0}),   // 1
      Vertex(Vector3{-1, -1, 0}, Vector2{1, 1}),  // 2
      Vertex(Vector3{1, -1, 0}, Vector2{0, 1})    // 3
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

void Renderer::handleInput() {
  // handle all queued inputs
  auto* inputBuffer = android_app_swap_input_buffers(app_);
  if (!inputBuffer) {
    // no inputs yet.
    return;
  }

  // handle motion events (motionEventsCounts can be 0).
  for (auto i = 0; i < inputBuffer->motionEventsCount; i++) {
    auto& motionEvent = inputBuffer->motionEvents[i];
    auto action = motionEvent.action;

    // Find the pointer index, mask and bitshift to turn it into a readable value.
    auto pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
    aout << "Pointer(s): ";

    // get the x and y position of this event if it is not ACTION_MOVE.
    auto& pointer = motionEvent.pointers[pointerIndex];
    auto x = GameActivityPointerAxes_getX(&pointer);
    auto y = GameActivityPointerAxes_getY(&pointer);

    // determine the action type and process the event accordingly.
    switch (action & AMOTION_EVENT_ACTION_MASK) {
      case AMOTION_EVENT_ACTION_DOWN:
      case AMOTION_EVENT_ACTION_POINTER_DOWN:
        aout << "(" << pointer.id << ", " << x << ", " << y << ") "
             << "Pointer Down";
        break;

      case AMOTION_EVENT_ACTION_CANCEL:
        // treat the CANCEL as an UP event: doing nothing in the app, except
        // removing the pointer from the cache if pointers are locally saved.
        // code pass through on purpose.
      case AMOTION_EVENT_ACTION_UP:
      case AMOTION_EVENT_ACTION_POINTER_UP:
        aout << "(" << pointer.id << ", " << x << ", " << y << ") "
             << "Pointer Up";
        break;

      case AMOTION_EVENT_ACTION_MOVE:
        // There is no pointer index for ACTION_MOVE, only a snapshot of
        // all active pointers; app needs to cache previous active pointers
        // to figure out which ones are actually moved.
        for (auto index = 0; index < motionEvent.pointerCount; index++) {
          pointer = motionEvent.pointers[index];
          x = GameActivityPointerAxes_getX(&pointer);
          y = GameActivityPointerAxes_getY(&pointer);
          aout << "(" << pointer.id << ", " << x << ", " << y << ")";

          if (index != (motionEvent.pointerCount - 1)) aout << ",";
          aout << " ";
        }
        aout << "Pointer Move";
        break;
      default:
        aout << "Unknown MotionEvent Action: " << action;
    }
    aout << endl;
  }
  // clear the motion input count in this buffer for main thread to re-use.
  android_app_clear_motion_events(inputBuffer);

  // handle input key events.
  for (auto i = 0; i < inputBuffer->keyEventsCount; i++) {
    auto& keyEvent = inputBuffer->keyEvents[i];
    aout << "Key: " << keyEvent.keyCode << " ";
    switch (keyEvent.action) {
      case AKEY_EVENT_ACTION_DOWN:
        aout << "Key Down";
        break;
      case AKEY_EVENT_ACTION_UP:
        aout << "Key Up";
        break;
      case AKEY_EVENT_ACTION_MULTIPLE:
        // Deprecated since Android API level 29.
        aout << "Multiple Key Actions";
        break;
      default:
        aout << "Unknown KeyEvent Action: " << keyEvent.action;
    }
    aout << endl;
  }
  // clear the key input count too.
  android_app_clear_key_events(inputBuffer);
}