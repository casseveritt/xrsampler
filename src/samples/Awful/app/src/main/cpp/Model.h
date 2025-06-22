#ifndef ANDROIDGLINVESTIGATIONS_MODEL_H
#define ANDROIDGLINVESTIGATIONS_MODEL_H

#include <vector>

#include "TextureAsset.h"
#include "linear.h"

struct Vertex {
  constexpr Vertex(const r3::Vec3f& inPosition, const r3::Vec2f& inUV) : position(inPosition), uv(inUV) {}

  r3::Vec3f position;
  r3::Vec2f uv;
};

typedef uint16_t Index;

class Model {
 public:
  inline Model(std::vector<Vertex> vertices, std::vector<Index> indices, std::shared_ptr<TextureAsset> spTexture)
      : vertices_(std::move(vertices)), indices_(std::move(indices)), spTexture_(std::move(spTexture)) {}

  inline const Vertex* getVertexData() const {
    return vertices_.data();
  }

  inline const size_t getIndexCount() const {
    return indices_.size();
  }

  inline const Index* getIndexData() const {
    return indices_.data();
  }

  inline const TextureAsset& getTexture() const {
    return *spTexture_;
  }

 private:
  std::vector<Vertex> vertices_;
  std::vector<Index> indices_;
  std::shared_ptr<TextureAsset> spTexture_;
};

#endif  // ANDROIDGLINVESTIGATIONS_MODEL_H