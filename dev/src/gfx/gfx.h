#ifndef LAND15_GFX_GFX_H_
#define LAND15_GFX_GFX_H_

#include <string_view>
#include <tuple>

#include "common/deleter_ptr.h"
#include "gfx/core.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glog/logging.h"
#include "SDL.h"
#include "sdl/cleanup.h"

// Single context, micro graphics library to mimic the venerable fbgfx.bi of
// FreeBASIC.

namespace land15 {
namespace gfx {

constexpr char kSystemFontPath[] = "res/system_font_.png";

class Image;
class Gfx final {
  friend class Image;

 public:
  // Must be called to use graphics functionality, can only be called once.
  // Resolution is the physical resolution of the drawing area whereas the
  // logical resolution is the resolution at which the pixels are displayed.
  static void Screen(glm::ivec2 res, bool fullscreen = false,
                     const std::string& title = "Title",
                     glm::ivec2 physical_res = {-1, -1});

  // Clear the screen (optionally to a color)
  static void Cls(Color32 col = Color32::kBlack);
  static void Cls(const Image& target, Color32 col = Color32::kBlack);

  static glm::ivec2 GetResolution();

  static bool IsFullscreen();
  static void SetFullscreen(bool fullscreen);

  // Updates the screen after waiting for vsync, clobbering the back buffer
  // in the process (be sure to ClS if you don't plan on overwriting the whole
  // backbuffer)
  static void Flip();

  static void PSet(glm::ivec2 p, Color32 color = Color32::kWhite);
  static void PSet(const Image& target, glm::ivec2 p,
                   Color32 color = Color32::kWhite);

  static void Line(glm::ivec2 a, glm::ivec2 b, Color32 color = Color32::kWhite);
  static void Line(const Image& target, glm::ivec2 a, glm::ivec2 b,
                   Color32 color = Color32::kWhite);

  static void Rect(glm::ivec2 a, glm::ivec2 b, Color32 color = Color32::kWhite);
  static void Rect(const Image& target, glm::ivec2 a, glm::ivec2 b,
                   Color32 color = Color32::kWhite);

  static void FillRect(glm::ivec2 a, glm::ivec2 b,
                       Color32 color = Color32::kWhite);
  static void FillRect(const Image& target, glm::ivec2 a, glm::ivec2 b,
                       Color32 color = Color32::kWhite);

  enum TextHAlign {
    kTextAlignHLeft = 0,
    kTextAlignHCenter = 1,
    kTextAlignHRight = 2
  };
  enum TextVAlign {
    kTextAlignVTop = 0,
    kTextAlignVCenter = 1,
    kTextAlignVBottom = 2
  };
  static void TextLine(std::string_view text, glm::ivec2 p,
                       Color32 color = Color32::kWhite,
                       TextHAlign h_align = kTextAlignHLeft,
                       TextVAlign v_align = kTextAlignVTop);
  static void TextLine(const Image& target, std::string_view text, glm::ivec2 p,
                       Color32 color = Color32::kWhite,
                       TextHAlign h_align = kTextAlignHLeft,
                       TextVAlign v_align = kTextAlignVTop);

  static void TextParagraph(std::string_view text, glm::ivec2 a, glm::ivec2 b,
                            Color32 color = Color32::kWhite,
                            TextHAlign h_align = kTextAlignHLeft,
                            TextVAlign v_align = kTextAlignVTop);
  static void TextParagraph(const Image& target, std::string_view text,
                            glm::ivec2 a, glm::ivec2 b,
                            Color32 color = Color32::kWhite,
                            TextHAlign h_align = kTextAlignHLeft,
                            TextVAlign v_align = kTextAlignVTop);

  static void Put(const Image& src, glm::ivec2 p, glm::ivec2 src_a = {-1, -1},
                  glm::ivec2 src_b = {-1, -1});
  static void Put(const Image& target, const Image& src, glm::ivec2 p,
                  glm::ivec2 src_a = {-1, -1}, glm::ivec2 src_b = {-1, -1});

  struct PutOptions {
   public:
    enum BlendMode { kBlendNone, kBlendAlpha, kBlendAdd, kBlendMod };
    BlendMode blend = kBlendAlpha;
    Color32 mod = Color32::kWhite;
    PutOptions& SetBlend(BlendMode blend) {
      this->blend = blend;
      return *this;
    }
    PutOptions& SetMod(Color32 mod) {
      this->mod = mod;
      return *this;
    }
  };
  static void PutEx(const Image& src, glm::ivec2 p, PutOptions opts,
                    glm::ivec2 src_a = {-1, -1}, glm::ivec2 src_b = {-1, -1});
  static void PutEx(const Image& target, const Image& src, glm::ivec2 p,
                    PutOptions opts, glm::ivec2 src_a = {-1, -1},
                    glm::ivec2 src_b = {-1, -1});

  // Updates the internal state from a queue of the inputs triggered since the
  // last call to SyncInputs. This must be called before calls to GetMouse or
  // GetKeyPressed.
  static void SyncInputs();

  struct MouseButtonPressedState {
    bool left;
    bool right;
    bool center;
  };

  // Returns the most recent location of the mouse cursor and if any of the
  // mouse buttons were pressed since the last call to SyncInputs().
  static const std::tuple<glm::ivec3, MouseButtonPressedState&> GetMouse();

  enum Key {
    kUpArrow = SDL_SCANCODE_UP,
    kRightArrow = SDL_SCANCODE_RIGHT,
    kDownArrow = SDL_SCANCODE_DOWN,
    kLeftArrow = SDL_SCANCODE_LEFT,
    kSpaceBar = SDL_SCANCODE_SPACE,
    kBackspace = SDL_SCANCODE_BACKSPACE,
    kEscape = SDL_SCANCODE_ESCAPE
  };

  // Returns true if the given key is currently pressed as of the last call to
  // SyncInputs()
  static bool GetKeyPressed(Key key);

  // Returns true if the close button was pressed since the last call to
  // SyncInputs()
  static bool Close();

 private:
  Gfx() { CHECK(false) << "An instance of Gfx should not be constructed."; }
  static void CheckInit(std::string_view meth_name) {
    CHECK(is_init()) << "Cannot call " << meth_name << " before FbGfx::Screen.";
  }

  static void PrepareFont();

  // Using SetRender* methods assumes that CheckInit has already been called.
  static void SetRenderTarget(SDL_Texture* target);
  static void SetRenderColor(Color32 col);

  static void InternalCls(SDL_Texture* texture, Color32 col);
  static void InternalPSet(SDL_Texture* texture, glm::ivec2 p, Color32 color);
  static void InternalLine(SDL_Texture* texture, glm::ivec2 a, glm::ivec2 b,
                           Color32 color);
  static void InternalRect(SDL_Texture* texture, glm::ivec2 a, glm::ivec2 b,
                           Color32 color);
  static void InternalFillRect(SDL_Texture* texture, glm::ivec2 a, glm::ivec2 b,
                               Color32 color);
  static void InternalPut(SDL_Texture* dest, SDL_Texture* src,
                          glm::ivec2 src_dims, glm::ivec2 p, PutOptions opts,
                          glm::ivec2 src_a, glm::ivec2 src_b);
  static void InternalTextLine(SDL_Texture* texture, std::string_view text,
                               glm::ivec2 p, Color32 color, TextHAlign h_align,
                               TextVAlign v_align);
  static void InternalTextParagraph(SDL_Texture* texture, std::string_view text,
                                    glm::ivec2 a, glm::ivec2 b, Color32 color,
                                    TextHAlign h_align, TextVAlign v_align);

  static bool is_init() { return window_.get() != nullptr; }
  static common::deleter_ptr<SDL_Window> window_;
  static common::deleter_ptr<SDL_Renderer> renderer_;

  static std::unique_ptr<Image> basic_font_;

  static uint32_t input_cycle_;

  static void HandleMouseButtonEvent(SDL_Event event);

  static MouseButtonPressedState mouse_button_state_;
  static glm::ivec3 mouse_pointer_position_;
  static bool close_pressed_;

  struct Cleanup {
    ~Cleanup() { sdl::Cleanup::UnregisterModule(); }
  };
  static Cleanup cleanup_;
};

}  // namespace gfx
}  // namespace land15

#endif  // LAND15_GFX_GFX_H_