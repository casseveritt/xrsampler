#include "GltfRenderer.h"

#include <android/log.h>

GltfRenderer::GltfRenderer() {}

GltfRenderer::~GltfRenderer() {
  __android_log_print(ANDROID_LOG_INFO, "GltfRenderer", "~GltfRenderer called");
  Destroy();
}

bool GltfRenderer::Init(const tinygltf::Model& model) {
  __android_log_print(ANDROID_LOG_INFO, "GltfRenderer", "Init called");
  model_ = &model;
  if (!CreateBuffers(model)) return false;
  if (!CreateTextures(model)) return false;
  if (!CreateVertexArrays(model)) return false;
  // TODO: Create/load shaders and set up uniforms
  return true;
}

void GltfRenderer::Render() {
  __android_log_print(ANDROID_LOG_INFO, "GltfRenderer", "Render called");
  if (!model_) return;
  // For each scene node, draw recursively
  for (const auto& scene : model_->scenes) {
    for (int nodeIdx : scene.nodes) {
      DrawNode(nodeIdx, r3::Matrix4f::Identity());
    }
  }
}

void GltfRenderer::Destroy() {
  __android_log_print(ANDROID_LOG_INFO, "GltfRenderer", "Destroy called");
  for (auto& bv : bufferViewsGL_) {
    if (bv.buffer) glDeleteBuffers(1, &bv.buffer);
  }
  for (auto& tex : texturesGL_) {
    if (tex.texture) glDeleteTextures(1, &tex.texture);
  }
  for (auto vao : vaos_) {
    if (vao) glDeleteVertexArrays(1, &vao);
  }
  bufferViewsGL_.clear();
  texturesGL_.clear();
  vaos_.clear();
  // TODO: Delete shader program(s)
}

bool GltfRenderer::CreateBuffers(const tinygltf::Model& model) {
  __android_log_print(ANDROID_LOG_INFO, "GltfRenderer", "CreateBuffers called");
  // Allocate OpenGL buffers for each bufferView
  bufferViewsGL_.resize(model.bufferViews.size());
  for (size_t i = 0; i < model.bufferViews.size(); ++i) {
    const auto& bv = model.bufferViews[i];
    __android_log_print(ANDROID_LOG_INFO, "GltfRenderer",
                        "Creating bufferView %zu: buffer=%d, target=0x%x, byteOffset=%zu, byteLength=%zu", i, bv.buffer,
                        bv.target, bv.byteOffset, bv.byteLength);
    GLuint buffer = 0;
    glGenBuffers(1, &buffer);
    glBindBuffer(bv.target, buffer);
    const auto& buf = model.buffers[bv.buffer];
    glBufferData(bv.target, bv.byteLength, &buf.data.at(bv.byteOffset), GL_STATIC_DRAW);
    bufferViewsGL_[i] = {buffer, (GLenum)bv.target, (GLsizeiptr)bv.byteLength};
  }
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  return true;
}

bool GltfRenderer::CreateTextures(const tinygltf::Model& model) {
  __android_log_print(ANDROID_LOG_INFO, "GltfRenderer", "CreateTextures called");
  // Allocate OpenGL textures for each image
  texturesGL_.resize(model.images.size());
  for (size_t i = 0; i < model.images.size(); ++i) {
    const auto& img = model.images[i];
    __android_log_print(ANDROID_LOG_INFO, "GltfRenderer", "Creating texture %zu: width=%d, height=%d, component=%d", i, img.width,
                        img.height, img.component);
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width, img.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.image.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    texturesGL_[i] = {tex, img.width, img.height};
  }
  glBindTexture(GL_TEXTURE_2D, 0);
  return true;
}

bool GltfRenderer::CreateVertexArrays(const tinygltf::Model& model) {
  __android_log_print(ANDROID_LOG_INFO, "GltfRenderer", "CreateVertexArrays called");
  // For each mesh/primitive, create a VAO and set up attribute pointers
  // This is a simplified example; real code should handle all attribute types and buffer bindings
  for (const auto& mesh : model.meshes) {
    for (const auto& prim : mesh.primitives) {
      __android_log_print(ANDROID_LOG_INFO, "GltfRenderer", "Creating VAO for mesh");
      GLuint vao = 0;
      glGenVertexArrays(1, &vao);
      glBindVertexArray(vao);
      // Bind attributes, index buffer, etc.
      // ...
      vaos_.push_back(vao);
    }
  }
  glBindVertexArray(0);
  return true;
}

namespace {
r3::Matrix4f GetNodeTransform(const tinygltf::Node& node) {
  r3::Matrix4f mat = r3::Matrix4f::Identity();

  if (node.matrix.size() == 16) {
    // Use the node's matrix directly
    mat = r3::Matrix4f(node.matrix[0], node.matrix[1], node.matrix[2], node.matrix[3], node.matrix[4], node.matrix[5],
                       node.matrix[6], node.matrix[7], node.matrix[8], node.matrix[9], node.matrix[10], node.matrix[11],
                       node.matrix[12], node.matrix[13], node.matrix[14], node.matrix[15]);
  } else {
    // Compose from T * R * S
    r3::Vec3f translation(0.0f, 0.0f, 0.0f);
    if (node.translation.size() == 3) {
      translation = r3::Vec3f(static_cast<float>(node.translation[0]), static_cast<float>(node.translation[1]),
                              static_cast<float>(node.translation[2]));
    }

    r3::Quaternionf rotation = r3::Quaternionf::Identity();
    if (node.rotation.size() == 4) {
      rotation = r3::Quaternionf(static_cast<float>(node.rotation[3]),  // w
                                 static_cast<float>(node.rotation[0]),  // x
                                 static_cast<float>(node.rotation[1]),  // y
                                 static_cast<float>(node.rotation[2])   // z
      );
    }

    r3::Vec3f scale(1.0f, 1.0f, 1.0f);
    if (node.scale.size() == 3) {
      scale = r3::Vec3f(static_cast<float>(node.scale[0]), static_cast<float>(node.scale[1]), static_cast<float>(node.scale[2]));
    }

    r3::Matrix4f tMat = r3::Matrix4f::Translate(translation);
    r3::Matrix4f rMat = rotation.GetMatrix4();
    r3::Matrix4f sMat = r3::Matrix4f::Scale(scale);

    mat = tMat * rMat * sMat;
  }

  return mat;
}
}  // namespace

void GltfRenderer::DrawNode(int nodeIndex, const r3::Matrix4f& parentMatrix) {
  __android_log_print(ANDROID_LOG_INFO, "GltfRenderer", "DrawNode called for node %d", nodeIndex);
  const auto& node = model_->nodes[nodeIndex];
  r3::Matrix4f localMatrix = parentMatrix * GetNodeTransform(node);

  if (node.mesh >= 0 && node.mesh < model_->meshes.size()) {
    const auto& mesh = model_->meshes[node.mesh];
    for (size_t primIdx = 0; primIdx < mesh.primitives.size(); ++primIdx) {
      const auto& prim = mesh.primitives[primIdx];
      // Bind VAO for this primitive (assumes 1:1 mapping)
      size_t vaoIdx = node.mesh;  // This may need adjustment if VAOs are per-primitive
      if (vaoIdx < vaos_.size()) {
        glBindVertexArray(vaos_[vaoIdx]);
      }
      // TODO: Bind material/textures if needed

      // Set model matrix uniform if using shaders
      // GLint modelLoc = glGetUniformLocation(shaderProgram_, "u_ModelMatrix");
      // glUniformMatrix4fv(modelLoc, 1, GL_FALSE, localMatrix.Data());

      // Draw call
      if (prim.indices >= 0 && prim.indices < model_->accessors.size()) {
        const auto& accessor = model_->accessors[prim.indices];
        GLenum mode = prim.mode == -1 ? GL_TRIANGLES : prim.mode;
        GLenum type = accessor.componentType;
        GLsizei count = accessor.count;
        GLvoid* offset = reinterpret_cast<GLvoid*>(accessor.byteOffset);
        glDrawElements(mode, count, type, offset);
      } else {
        // No indices, draw arrays
        GLenum mode = prim.mode == -1 ? GL_TRIANGLES : prim.mode;
        const auto& posAccessor = prim.attributes.at("POSITION");
        const auto& accessor = model_->accessors[posAccessor];
        GLsizei count = accessor.count;
        glDrawArrays(mode, 0, count);
      }
      glBindVertexArray(0);
    }
  }

  for (int child : node.children) {
    DrawNode(child, localMatrix);
  }
}
