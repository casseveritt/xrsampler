/* helpers for OpenXR type to/from r3 linear */

#include "linear.h"
#include "xrh.h"

namespace xrh {
struct Vector3f : public r3::Vec3f {
  Vector3f(const XrVector3f& v) : r3::Vec3f(&v.x) {}
  Vector3f(const r3::Vec3f& v) : r3::Vec3f(v) {}
  operator XrVector3f() const {
    return {x, y, z};
  }
};

struct Quatf : public r3::Quaternionf {
  Quatf(const XrQuaternionf& q) : r3::Quaternionf(&q.x) {}
  Quatf(const r3::Quaternionf& q) : r3::Quaternionf(q) {}
  operator XrQuaternionf() const {
    return {x, y, z, w};
  }
};

struct Posef : public r3::Posef {
  Posef(const XrPosef& p) : r3::Posef(Quatf(p.orientation), Vector3f(p.position)) {}
  operator XrPosef() const {
    return {Quatf(r), Vector3f(t)};
  }
};
}  // namespace xrh
