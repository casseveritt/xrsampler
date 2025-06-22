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

void GltfRenderer::Render(Shader* shader) {
  __android_log_print(ANDROID_LOG_INFO, "GltfRenderer", "Render called");
  if (!model_) return;
  shader->activate();
  // For each scene node, draw recursively
  for (const auto& scene : model_->scenes) {
    for (int nodeIdx : scene.nodes) {
      DrawNode(nodeIdx, r3::Matrix4f::Identity(), shader);
    }
  }
  shader->deactivate();
}

void GltfRenderer::Destroy() {
  __android_log_print(ANDROID_LOG_INFO, "GltfRenderer", "Destroy called");
  for (auto& b : buffersGL_) {
    if (b.buffer) glDeleteBuffers(1, &b.buffer);
  }
  for (auto& tex : texturesGL_) {
    if (tex.texture) glDeleteTextures(1, &tex.texture);
  }
  glDeleteVertexArrays(vaos_.size(), vaos_.data());
  buffersGL_.clear();
  texturesGL_.clear();
  vaos_.clear();
}

bool GltfRenderer::CreateBuffers(const tinygltf::Model& model) {
  __android_log_print(ANDROID_LOG_INFO, "GltfRenderer", "CreateBuffers called");
  // Allocate OpenGL buffers for each bufferView
  std::vector<GLuint> buffers(model.buffers.size());
  buffersGL_.resize(model.buffers.size());
  glGenBuffers(model.buffers.size(), buffers.data());

  for (size_t i = 0; i < model.buffers.size(); ++i) {
    const auto& buf = model.buffers[i];
    auto buffer = buffers[i];
    buffersGL_[i].buffer = buffer;
    buffersGL_[i].size = buf.data.size();
    // We don't know the target yet; will bind as needed for bufferViews
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, buf.data.size(), buf.data.data(), GL_STATIC_DRAW);
    __android_log_print(ANDROID_LOG_INFO, "GltfRenderer", "Created buffer %zu: size=%zu", i, buf.data.size());
  }
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  return true;
}

namespace {
GLint numComponents(const int type) {
  switch (type) {
    case TINYGLTF_TYPE_SCALAR:
      return 1;
    case TINYGLTF_TYPE_VEC2:
      return 2;
    case TINYGLTF_TYPE_VEC3:
      return 3;
    case TINYGLTF_TYPE_VEC4:
      return 4;
    case TINYGLTF_TYPE_MAT2:
      return 4;  // 2x2 matrix
    case TINYGLTF_TYPE_MAT3:
      return 9;  // 3x3 matrix
    case TINYGLTF_TYPE_MAT4:
      return 16;  // 4x4 matrix
    default:
      return 0;  // Unsupported type
  }
}

GLenum glComponentType(const int componentType) {
  switch (componentType) {
    case TINYGLTF_COMPONENT_TYPE_BYTE:
      return GL_BYTE;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
      return GL_UNSIGNED_BYTE;
    case TINYGLTF_COMPONENT_TYPE_SHORT:
      return GL_SHORT;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
      return GL_UNSIGNED_SHORT;
    case TINYGLTF_COMPONENT_TYPE_INT:
      return GL_INT;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
      return GL_UNSIGNED_INT;
    case TINYGLTF_COMPONENT_TYPE_FLOAT:
      return GL_FLOAT;
    default:
      return 0;  // Unsupported type
  }
}

}  // namespace

bool GltfRenderer::CreateVertexArrays(const tinygltf::Model& model) {
  __android_log_print(ANDROID_LOG_INFO, "GltfRenderer", "CreateVertexArrays called");
  // For each mesh/primitive, create a VAO and set up attribute pointers
  // This is a simplified example; real code should handle all attribute types and buffer bindings
  int numVaos = 0;
  for (const auto& mesh : model.meshes) {
    for (const auto& prim : mesh.primitives) {
      numVaos++;
    }
  }
  vaos_.reserve(numVaos);
  for (const auto& mesh : model.meshes) {
    for (const auto& prim : mesh.primitives) {
      __android_log_print(ANDROID_LOG_INFO, "GltfRenderer", "Creating VAO for mesh");
      GLuint vao = 0;
      glGenVertexArrays(1, &vao);
      glBindVertexArray(vao);

      // Start with just indices (if any) and position.
      if (prim.indices >= 0 && prim.indices < model.accessors.size()) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffersGL_[prim.indices].buffer);
      }

      const int pos = prim.attributes.at("POSITION");
      if (pos >= 0 && pos < model.accessors.size()) {
        const auto& bvIdx = model.accessors[pos].bufferView;
        const auto& bv = model.bufferViews[bvIdx];
        glBindBuffer(GL_ARRAY_BUFFER, buffersGL_[bvIdx].buffer);
        glEnableVertexAttribArray(0);  // Position attribute
        const auto& accessor = model.accessors[pos];
        glVertexAttribPointer(0, numComponents(accessor.type), glComponentType(accessor.componentType), GL_FALSE,
                              accessor.ByteStride(bv), reinterpret_cast<const GLvoid*>(accessor.byteOffset));
      }

      vaos_.push_back(vao);
    }
  }
  glBindVertexArray(0);
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

void GltfRenderer::DrawNode(int nodeIndex, const r3::Matrix4f& parentMatrix, Shader* shader) {
  __android_log_print(ANDROID_LOG_INFO, "GltfRenderer", "DrawNode called for node %d", nodeIndex);
  const auto& node = model_->nodes[nodeIndex];
  r3::Matrix4f toClipFromObject = parentMatrix * GetNodeTransform(node);

  int currVaoIdx = -1;
  if (node.mesh >= 0 && node.mesh < model_->meshes.size()) {
    const auto& mesh = model_->meshes[node.mesh];
    for (size_t primIdx = 0; primIdx < mesh.primitives.size(); ++primIdx) {
      currVaoIdx++;
      const auto& prim = mesh.primitives[primIdx];
      if (currVaoIdx < vaos_.size()) {
        glBindVertexArray(vaos_[currVaoIdx]);
      }
      // TODO: Bind material/textures if needed

      shader->setToClipFromObject(toClipFromObject);
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
    DrawNode(child, toClipFromObject, shader);
  }
}
