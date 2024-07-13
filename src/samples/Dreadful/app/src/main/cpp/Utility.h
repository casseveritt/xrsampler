#ifndef ANDROIDGLINVESTIGATIONS_UTILITY_H
#define ANDROIDGLINVESTIGATIONS_UTILITY_H

#include <cassert>

class Utility {
 public:
  static bool checkAndLogGlError(bool alwaysLog = false);

  static inline void assertGlError() {
    assert(checkAndLogGlError());
  }

};

#endif  // ANDROIDGLINVESTIGATIONS_UTILITY_H