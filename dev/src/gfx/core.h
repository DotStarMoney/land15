#ifndef LAND15_GFX_CORE_H_
#define LAND15_GFX_CORE_H_

#include <stdint.h>

namespace land15 {
namespace gfx {

struct Color32 {
  Color32(int32_t x) : value(x) {}
  Color32(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    channel.a = a;
    channel.b = b;
    channel.g = g;
    channel.r = r;
  }
  union {
    struct {
      unsigned int a : 8;
      unsigned int b : 8;
      unsigned int g : 8;
      unsigned int r : 8;
    } channel;
    int32_t value;
  };
  
  Color32& operator=(const int32_t& x) { this->value = x; }
  operator int32_t() const { return this->value; }
  enum : uint32_t {
    kTransparentBlack = 0x00000000,
    kBlack = 0x000000ff,
    kWhite = 0xffffffff,
    kRed = 0xff0000ff,
    kGreen = 0x00ff00ff,
    kBlue = 0x0000ffff,
    kCyan = 0x00ffffff,
    kMagenta = 0xff00ffff,
    kYellow = 0xffff00ff
  };
};

static_assert(sizeof(Color32) == 4);

}  // namespace gfx
}  // namespace land15

#endif  // LAND15_GFX_CORE_H_