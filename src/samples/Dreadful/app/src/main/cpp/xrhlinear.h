/* helpers for OpenXR type to/from r3 linear */

#include "xrh.h"
#include "linear.h"

namespace xrh {
    struct Vector3f : public r3::Vec3f {
        Vector3f(const XrVector3f& v) : r3::Vec3f(&v.x) {}
        operator XrVector3f() const {
            return {x, y, z};
        }
    };
}

