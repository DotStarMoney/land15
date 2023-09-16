#include "gfx/image.h"

#include <stdint.h>

#include <memory>
#include <string>

#include "gfx/gfx.h"
#include "glog/logging.h"
#define STB_IMAGE_IMPLEMENTATION
#include "common/deleter_ptr.h"
#include "stb_image.h"

namespace land15 {
namespace gfx {

using common::deleter_ptr;
using glm::ivec2;
using std::string;
using std::unique_ptr;

Image::Image(deleter_ptr<SDL_Texture> texture, int w, int h, bool is_target)
    : texture_(std::move(texture)), w_(w), h_(h), is_target_(is_target) {}

unique_ptr<Image> Image::OfSize(ivec2 dimensions) {
  Gfx::CheckInit(__func__);

  deleter_ptr<SDL_Texture> texture(
      SDL_CreateTexture(Gfx::renderer_.get(), SDL_PIXELFORMAT_RGBA8888,
                        SDL_TEXTUREACCESS_TARGET, dimensions.x, dimensions.y),
      [](SDL_Texture* t) { SDL_DestroyTexture(t); });
  CHECK_NE(texture.get(), static_cast<SDL_Texture*>(NULL))
      << "SDL error (SDL_CreateTexture): " << SDL_GetError();
  return unique_ptr<Image>(
      new Image(std::move(texture), dimensions.x, dimensions.y, true));
}

deleter_ptr<SDL_Texture> Image::TextureFromSurface(SDL_Surface* surface) {
  deleter_ptr<SDL_Texture> texture(
      SDL_CreateTextureFromSurface(Gfx::renderer_.get(), surface),
      [](SDL_Texture* t) { SDL_DestroyTexture(t); });
  CHECK_NE(texture.get(), static_cast<SDL_Texture*>(NULL))
      << "SDL error (SDL_CreateTextureFromSurface): " << SDL_GetError();
  return std::move(texture);
}

unique_ptr<Image> Image::FromFile(const string& filename) {
  Gfx::CheckInit(__func__);

  int w;
  int h;
  int orig_format_unused;
  deleter_ptr<StbImageData> image_data(
      stbi_load(filename.c_str(), &w, &h, &orig_format_unused, STBI_rgb_alpha),
      [](StbImageData* d) { stbi_image_free(d); });
  CHECK_NE(static_cast<void*>(image_data.get()), static_cast<void*>(NULL))
      << "stb_image error (stbi_load): " << stbi_failure_reason();

  deleter_ptr<SDL_Surface> surface(
      SDL_CreateSurfaceFrom(image_data.get(), w, h, 4 * w,
                            SDL_PIXELFORMAT_ABGR8888),
      [](SDL_Surface* s) { SDL_DestroySurface(s); });
  CHECK_NE(surface.get(), static_cast<SDL_Surface*>(NULL))
      << "SDL error (SDL_CreateRGBSurfaceWithFormatFrom): " << SDL_GetError();

  return unique_ptr<Image>(
      new Image(TextureFromSurface(surface.get()), w, h, false));
}

}  // namespace gfx
}  // namespace land15
