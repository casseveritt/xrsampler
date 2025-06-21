#include "GltfRenderer.h"

GltfRenderer::GltfRenderer() {}

GltfRenderer::~GltfRenderer() {
  Destroy();
}

bool GltfRenderer::Init(const tinygltf::Model& model) {
  model_ = &model;
  if (!CreateBuffers(model)) return false;
  if (!CreateTextures(model)) return false;
  if (!CreateVertexArrays(model)) return false;
  // TODO: Create/load shaders and set up uniforms
  return true;
}

void GltfRenderer::Render() {
  if (!model_) return;
  // For each scene node, draw recursively
  for (const auto& scene : model_->scenes) {
    for (int nodeIdx : scene.nodes) {
      DrawNode(nodeIdx, r3::Matrix4f::Identity());
    }
  }
}

void GltfRenderer::Destroy() {
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
  // Allocate OpenGL buffers for each bufferView
  bufferViewsGL_.resize(model.bufferViews.size());
  for (size_t i = 0; i < model.bufferViews.size(); ++i) {
    const auto& bv = model.bufferViews[i];
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
  // Allocate OpenGL textures for each image
  texturesGL_.resize(model.images.size());
  for (size_t i = 0; i < model.images.size(); ++i) {
    const auto& img = model.images[i];
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
  // For each mesh/primitive, create a VAO and set up attribute pointers
  // This is a simplified example; real code should handle all attribute types and buffer bindings
  for (const auto& mesh : model.meshes) {
    for (const auto& prim : mesh.primitives) {
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

void GltfRenderer::DrawNode(int nodeIndex, const r3::Matrix4f& parentMatrix) {
  const auto& node = model_->nodes[nodeIndex];
  r3::Matrix4f localMatrix = parentMatrix;
  // Compute local transform from node.translation, rotation, scale, matrix
  // ...
  if (node.mesh >= 0) {
    // Bind VAO, set uniforms, draw elements
    // ...
  }
  for (int child : node.children) {
    DrawNode(child, localMatrix);
  }
}