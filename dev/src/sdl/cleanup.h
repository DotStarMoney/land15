#ifndef LAND15_SDL_CLEANUP_H_
#define LAND15_SDL_CLEANUP_H_

#include "glog/logging.h"
#include "SDL.h"

namespace land15 {
// Forward declarations of classes that can use Cleanup
namespace gfx {
class Gfx;
}  // namespace gfx

namespace sdl {

  class Cleanup {
  friend class gfx::Gfx;
  static int remaining_modules_;
  static void RegisterModule() { ++remaining_modules_; }
  static void UnregisterModule() {
    if (--remaining_modules_ == 0) {
      SDL_Quit();
    }
  }
  Cleanup() {
    CHECK(false) << "An instance of Cleanup should not be constructed.";
  }
};

}  // namespace sdl
}  // namespace land15

#endif  // LAND15_SDL_CLEANUP_H_