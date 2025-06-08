/* helpers for OpenXR type to/from r3 linear */

#include "linear.h"
#include "xrh.h"

#pragma once

namespace xrh {
struct Vector3f : public r3::Vec3f {
  Vector3f() = default;
  Vector3f(float x_, float y_, float z_) : r3::Vec3f(x_, y_, z_) {}
  Vector3f(const float* tp) : r3::Vec3f(tp) {}
  Vector3f(const XrVector3f& v) : r3::Vec3f(&v.x) {}
  Vector3f(const r3::Vec3f& v) : r3::Vec3f(v) {}
  operator XrVector3f() const {
    return {x, y, z};
  }
};

struct Quatf : public r3::Quaternionf {
  Quatf() = default;
  Quatf(float x_, float y_, float z_, float w_) : r3::Quaternionf(x_, y_, z_, w_) {}
  Quatf(const float* v) : r3::Quaternionf(v) {}
  Quatf(const Vector3f& axis, float angle) : r3::Quaternionf(r3::Vec3f(axis), angle) {}
  Quatf(const XrQuaternionf& q) : r3::Quaternionf(&q.x) {}
  Quatf(const r3::Quaternionf& q) : r3::Quaternionf(q) {}
  operator XrQuaternionf() const {
    return {x, y, z, w};
  }
};

struct Posef : public r3::Posef {
  Posef() = default;
  Posef(const Quatf& r, const Vector3f& t) : r3::Posef(r3::Quaternionf(r), r3::Vec3f(t)) {}
  Posef(const XrPosef& p) : r3::Posef(Quatf(p.orientation), Vector3f(p.position)) {}
  operator XrPosef() const {
    return {Quatf(r), Vector3f(t)};
  }
};
}  // namespace xrh
