#ifndef LAND15_GFX_IMAGE_H_
#define LAND15_GFX_IMAGE_H_

#include <memory>
#include <string>

#include "common/deleter_ptr.h"
#include "gfx/core.h"
#include "glm/vec2.hpp"
#include "glog/logging.h"
#include "SDL.h"

namespace land15 {
namespace gfx {

class Gfx;

// Fixed size 32bit image class, basically a wrapper around SDL_Texture and an
// image loading library.
class Image {
  friend class Gfx;

 public:
  Image(const Image&) = delete;
  Image& operator=(const Image&) = delete;

  virtual ~Image() {}

  // Load an image from a file.
  static std::unique_ptr<Image> FromFile(const std::string& filename);

  // Create an image of the provided dimensions. The contents of the texture
  // are undefined and should be cleared/filled-entirely before use.
  static std::unique_ptr<Image> OfSize(glm::ivec2 dimensions);

  int width() const { return w_; }
  int height() const { return h_; }
  bool is_render_target() const { return is_target_; }

 private:
  typedef unsigned char StbImageData;
  Image(common::deleter_ptr<SDL_Texture> texture, int w, int h, bool is_target);

  static common::deleter_ptr<SDL_Texture> TextureFromSurface(
      SDL_Surface* surface);

  void CheckTarget(std::string_view meth_name) const {
    CHECK(is_target_) << "Image cannot be the target of drawing operation "
                      << meth_name << ".";
  }

  const common::deleter_ptr<SDL_Texture> texture_;
  const int w_;
  const int h_;
  const bool is_target_;
};

}  // namespace gfx
}  // namespace land15

#endif  // LAND15_GFX_IMAGE_H_