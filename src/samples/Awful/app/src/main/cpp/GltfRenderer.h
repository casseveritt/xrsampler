#pragma once

#include <GLES3/gl3.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "Shader.h"
#include "linear.h"
#include "tiny_gltf.h"

class GltfRenderer {
 public:
  GltfRenderer();
  ~GltfRenderer();

  // Initialize OpenGL resources from a loaded tinygltf::Model
  bool Init(const tinygltf::Model& model);

  // Render the model (call every frame as needed)
  void Render(Shader* shader);

  // Destroy OpenGL resources
  void Destroy();

 private:
  struct BufferGL {
    GLuint buffer = 0;
    GLsizeiptr size = 0;
  };

  struct TextureGL {
    GLuint texture = 0;
    int width = 0;
    int height = 0;
  };

  // Store OpenGL handles for each bufferView and texture
  std::vector<BufferGL> buffersGL_;
  std::vector<TextureGL> texturesGL_;

  // Store VAOs for each mesh/primitive
  std::vector<GLuint> vaos_;

  // Store the model for reference (optional)
  const tinygltf::Model* model_ = nullptr;

  // Helper functions
  bool CreateBuffers(const tinygltf::Model& model);
  bool CreateTextures(const tinygltf::Model& model);
  bool CreateVertexArrays(const tinygltf::Model& model);

  void DrawNode(int nodeIndex, const r3::Matrix4f& parentMatrix, Shader* shader);

  // Add your shader program(s) and uniform locations here
  GLuint shaderProgram_ = 0;
};