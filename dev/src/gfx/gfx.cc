#include "gfx/gfx.h"

#include <memory>
#include <string_view>

#include "SDL.h"
#include "common/deleter_ptr.h"
#include "gfx/core.h"
#include "gfx/image.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glog/logging.h"
#include "sdl/cleanup.h"

namespace land15 {
namespace gfx {

using common::deleter_ptr;
using glm::ivec2;
using glm::ivec3;
using std::string;
using std::string_view;
using std::unique_ptr;

namespace {

const ivec2 kTextCharacterDims{8, 8};

}  // namespace

// Gfx variables

Gfx::Cleanup Gfx::cleanup_;
deleter_ptr<SDL_Window> Gfx::window_ = nullptr;
deleter_ptr<SDL_Renderer> Gfx::renderer_ = nullptr;
unique_ptr<Image> Gfx::basic_font_ = nullptr;

// Input variables

uint32_t Gfx::input_cycle_ = 0;

Gfx::MouseButtonPressedState Gfx::mouse_button_state_{false, false, false};
ivec3 Gfx::mouse_pointer_position_{0, 0, 0};
bool Gfx::close_pressed_ = false;

void Gfx::Screen(ivec2 res, bool fullscreen, const string& title,
                 ivec2 physical_res) {
  CHECK(!is_init()) << "Cannot initialize Gfx more than once.";

  sdl::Cleanup::RegisterModule();
  SDL_Init(SDL_INIT_VIDEO);

  if ((physical_res.x == -1) || (physical_res.y == -1)) physical_res = res;
  // We open the window initially hidden (and then reveal it once all of this
  // setup is out of the way)
  window_ = deleter_ptr<SDL_Window>(
      SDL_CreateWindowWithPosition(
          title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
          physical_res.x, physical_res.y,
          (fullscreen ? SDL_WINDOW_FULLSCREEN : 0) | SDL_WINDOW_HIDDEN),
      [](SDL_Window* w) { SDL_DestroyWindow(w); });
  CHECK_NE(window_.get(), static_cast<SDL_Window*>(nullptr))
      << "SDL error (SDL_CreateWindowWithPosition): " << SDL_GetError();
  renderer_ = deleter_ptr<SDL_Renderer>(
      SDL_CreateRenderer(window_.get(), NULL,
                         SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC),
      [](SDL_Renderer* r) { SDL_DestroyRenderer(r); });

  CHECK_NE(renderer_.get(), static_cast<SDL_Renderer*>(nullptr))
      << "SDL error (SDL_CreateRenderer): " << SDL_GetError();
  CHECK_EQ(SDL_SetRenderLogicalPresentation(
               renderer_.get(), res.x, res.y, SDL_LOGICAL_PRESENTATION_STRETCH,
               SDL_ScaleMode::SDL_SCALEMODE_NEAREST),
           0)
      << "SDL error (SDL_RenderSetLogicalSize): " << SDL_GetError();

  CHECK_EQ(SDL_SetRenderDrawBlendMode(renderer_.get(), SDL_BLENDMODE_BLEND), 0)
      << "SDL error (SDL_SetRenderDrawBlendMode): " << SDL_GetError();

  // Load the system font
  PrepareFont();

  // Reveal our window
  SDL_ShowWindow(window_.get());
}

void Gfx::PrepareFont() {
  basic_font_ = Image::FromFile(kSystemFontPath);
  SDL_Texture* font_tex = basic_font_.get()->texture_.get();
  CHECK_EQ(SDL_SetTextureBlendMode(font_tex, SDL_BLENDMODE_BLEND), 0)
      << "SDL error (SDL_SetTextureBlendMode): " << SDL_GetError();
  CHECK_EQ(SDL_SetTextureAlphaMod(font_tex, 255), 0)
      << "SDL error (SDL_SetTextureAlphaMod): " << SDL_GetError();
}

void Gfx::SetRenderTarget(SDL_Texture* target) {
  CHECK_EQ(SDL_SetRenderTarget(renderer_.get(), target), 0)
      << "SDL error (SDL_SetRenderTarget): " << SDL_GetError();
}

void Gfx::SetRenderColor(Color32 col) {
  CHECK_EQ(SDL_SetRenderDrawColor(renderer_.get(), col.channel.r, col.channel.g,
                                  col.channel.b, col.channel.a),
           0)
      << "SDL error (SDL_SetRenderDrawColor): " << SDL_GetError();
}

bool Gfx::IsFullscreen() {
  CheckInit(__func__);
  return SDL_GetWindowFlags(window_.get()) & SDL_WINDOW_FULLSCREEN;
}

void Gfx::SetFullscreen(bool fullscreen) {
  CheckInit(__func__);
  CHECK_EQ(
      SDL_SetWindowFullscreen(window_.get(), fullscreen ? SDL_TRUE : SDL_FALSE),
      0)
      << "SDL error (SDL_SetWindowFullscreen): " << SDL_GetError();
}

ivec2 Gfx::GetResolution() {
  CheckInit(__func__);
  ivec2 res;
  SDL_RendererLogicalPresentation unused_mode;
  SDL_ScaleMode unused_scale_mode;
  SDL_GetRenderLogicalPresentation(renderer_.get(), &res.x, &res.y,
                                   &unused_mode, &unused_scale_mode);
  return res;
}

void Gfx::Flip() { SDL_RenderPresent(renderer_.get()); }

// Cls

void Gfx::Cls(const Image& target, Color32 col) {
  CheckInit(__func__);
  target.CheckTarget(__func__);
  InternalCls(target.texture_.get(), col);
}
void Gfx::Cls(Color32 col) {
  CheckInit(__func__);
  InternalCls(nullptr, col);
}
void Gfx::InternalCls(SDL_Texture* texture, Color32 col) {
  SetRenderTarget(texture);
  SetRenderColor(col);
  CHECK_EQ(SDL_RenderClear(renderer_.get()), 0)
      << "SDL error (SDL_RenderClear): " << SDL_GetError();
}

// PSet

void Gfx::PSet(ivec2 p, Color32 color) {
  CheckInit(__func__);
  InternalPSet(nullptr, p, color);
}
void Gfx::PSet(const Image& target, ivec2 p, Color32 color) {
  CheckInit(__func__);
  target.CheckTarget(__func__);
  InternalPSet(target.texture_.get(), p, color);
}
void Gfx::InternalPSet(SDL_Texture* texture, glm::ivec2 p, Color32 color) {
  SetRenderTarget(texture);
  SetRenderColor(color);
  CHECK_EQ(SDL_RenderPoint(renderer_.get(), p.x, p.y), 0)
      << "SDL error (SDL_RenderPoint): " << SDL_GetError();
}

// Line

void Gfx::Line(ivec2 a, ivec2 b, Color32 color) {
  CheckInit(__func__);
  InternalLine(nullptr, a, b, color);
}
void Gfx::Line(const Image& target, ivec2 a, ivec2 b, Color32 color) {
  CheckInit(__func__);
  target.CheckTarget(__func__);
  InternalLine(target.texture_.get(), a, b, color);
}
void Gfx::InternalLine(SDL_Texture* texture, ivec2 a, ivec2 b, Color32 color) {
  SetRenderTarget(texture);
  SetRenderColor(color);
  CHECK_EQ(SDL_RenderLine(renderer_.get(), a.x, a.y, b.x, b.y), 0)
      << "SDL error (SDL_RenderLine): " << SDL_GetError();
}

// Rect

void Gfx::Rect(ivec2 a, ivec2 b, Color32 color) {
  CheckInit(__func__);
  InternalRect(nullptr, a, b, color);
}
void Gfx::Rect(const Image& target, ivec2 a, ivec2 b, Color32 color) {
  CheckInit(__func__);
  target.CheckTarget(__func__);
  InternalRect(target.texture_.get(), a, b, color);
}
void Gfx::InternalRect(SDL_Texture* texture, ivec2 a, ivec2 b, Color32 color) {
  SetRenderTarget(texture);
  SetRenderColor(color);
  SDL_FRect rect{a.x, a.y, b.x, b.y};
  CHECK_EQ(SDL_RenderRect(renderer_.get(), &rect), 0)
      << "SDL error (SDL_RenderRect): " << SDL_GetError();
}

// FillRect

void Gfx::FillRect(ivec2 a, ivec2 b, Color32 color) {
  CheckInit(__func__);
  InternalFillRect(nullptr, a, b, color);
}
void Gfx::FillRect(const Image& target, ivec2 a, ivec2 b, Color32 color) {
  CheckInit(__func__);
  target.CheckTarget(__func__);
  InternalFillRect(target.texture_.get(), a, b, color);
}
void Gfx::InternalFillRect(SDL_Texture* texture, ivec2 a, ivec2 b,
                           Color32 color) {
  SetRenderTarget(texture);
  SetRenderColor(color);
  SDL_FRect rect{a.x, a.y, b.x, b.y};
  CHECK_EQ(SDL_RenderFillRect(renderer_.get(), &rect), 0)
      << "SDL error (SDL_RenderFillRect): " << SDL_GetError();
}

// Put & PutEx

void Gfx::Put(const Image& src, ivec2 p, ivec2 src_a, ivec2 src_b) {
  CheckInit(__func__);
  InternalPut(nullptr, src.texture_.get(), {src.width(), src.height()}, p,
              PutOptions(), src_a, src_b);
}
void Gfx::Put(const Image& target, const Image& src, ivec2 p, ivec2 src_a,
              ivec2 src_b) {
  CheckInit(__func__);
  target.CheckTarget(__func__);
  InternalPut(target.texture_.get(), src.texture_.get(),
              {src.width(), src.height()}, p, PutOptions(), src_a, src_b);
}

void Gfx::PutEx(const Image& src, ivec2 p, PutOptions opts, ivec2 src_a,
                ivec2 src_b) {
  CheckInit(__func__);
  InternalPut(nullptr, src.texture_.get(), {src.width(), src.height()}, p, opts,
              src_a, src_b);
}
void Gfx::PutEx(const Image& target, const Image& src, ivec2 p, PutOptions opts,
                ivec2 src_a, ivec2 src_b) {
  CheckInit(__func__);
  target.CheckTarget(__func__);
  InternalPut(target.texture_.get(), src.texture_.get(),
              {src.width(), src.height()}, p, opts, src_a, src_b);
}

inline SDL_BlendMode GetSdlBlendMode(Gfx::PutOptions::BlendMode m) {
  switch (m) {
    case Gfx::PutOptions::kBlendNone:
      return SDL_BLENDMODE_NONE;
    case Gfx::PutOptions::kBlendAlpha:
      return SDL_BLENDMODE_BLEND;
    case Gfx::PutOptions::kBlendAdd:
      return SDL_BLENDMODE_ADD;
    case Gfx::PutOptions::kBlendMod:
      return SDL_BLENDMODE_MOD;
    default:
      CHECK(false) << "Not a real blend mode: " << m;
  }
}

void Gfx::InternalPut(SDL_Texture* dest, SDL_Texture* src, ivec2 src_dims,
                      ivec2 p, PutOptions opts, ivec2 src_a, ivec2 src_b) {
  SetRenderTarget(dest);

  CHECK_EQ(SDL_SetTextureBlendMode(src, GetSdlBlendMode(opts.blend)), 0)
      << "SDL error (SDL_SetTextureBlendMode): " << SDL_GetError();
  CHECK_EQ(SDL_SetTextureColorMod(src, opts.mod.channel.r, opts.mod.channel.g,
                                  opts.mod.channel.b),
           0)
      << "SDL error (SDL_SetTextureColorMod): " << SDL_GetError();
  CHECK_EQ(SDL_SetTextureAlphaMod(src, opts.mod.channel.a), 0)
      << "SDL error (SDL_SetTextureAlphaMod): " << SDL_GetError();

  SDL_FRect dst_rect;
  SDL_FRect src_rect;

  dst_rect.x = p.x;
  dst_rect.y = p.y;

  const SDL_FRect* src_rect_target = &src_rect;
  if ((src_a.x == -1) || (src_a.y == -1) || (src_b.x == -1) ||
      (src_b.y == -1)) {
    src_rect_target = nullptr;
    dst_rect.w = src_dims.x;
    dst_rect.h = src_dims.y;
  } else {
    if (src_a.x > src_b.x) std::swap(src_a.x, src_b.y);
    if (src_a.y > src_b.y) std::swap(src_a.y, src_b.y);
    src_rect.x = src_a.x;
    src_rect.y = src_a.y;
    src_rect.w = src_b.x - src_a.x + 1;
    src_rect.h = src_b.y - src_a.y + 1;
    dst_rect.w = src_rect.w;
    dst_rect.h = src_rect.h;
  }

  CHECK_EQ(SDL_RenderTexture(renderer_.get(), src, src_rect_target, &dst_rect),
           0)
      << "SDL error (SDL_RenderTexture): " << SDL_GetError();
}

// TextLine

void Gfx::TextLine(string_view text, ivec2 p, Color32 color, TextHAlign h_align,
                   TextVAlign v_align) {
  CheckInit(__func__);
  InternalTextLine(nullptr, text, p, color, h_align, v_align);
}
void Gfx::TextLine(const Image& target, string_view text, ivec2 p,
                   Color32 color, TextHAlign h_align, TextVAlign v_align) {
  CheckInit(__func__);
  target.CheckTarget(__func__);
  InternalTextLine(target.texture_.get(), text, p, color, h_align, v_align);
}

void Gfx::InternalTextLine(SDL_Texture* texture, string_view text, ivec2 p,
                           Color32 color, TextHAlign h_align,
                           TextVAlign v_align) {
  SetRenderTarget(texture);
  CHECK_EQ(
      SDL_SetTextureColorMod(basic_font_.get()->texture_.get(), color.channel.r,
                             color.channel.g, color.channel.b),
      0)
      << "SDL error (SDL_SetTextureColorMod): " << SDL_GetError();
  const ivec2 box_dims{text.size() * kTextCharacterDims.x,
                       kTextCharacterDims.y};
  switch (h_align) {
    case kTextAlignHLeft:
      break;
    case kTextAlignHCenter:
      p.x -= box_dims.x * 0.5;
      break;
    case kTextAlignHRight:
      p.x -= box_dims.x;
      break;
    default:
      CHECK(false) << "Invalid horizontal text alignment specified: "
                   << h_align;
  }

  switch (v_align) {
    case kTextAlignVTop:
      break;
    case kTextAlignVCenter:
      p.y -= box_dims.y * 0.5;
      break;
    case kTextAlignVBottom:
      p.y -= box_dims.y;
      break;
    default:
      CHECK(false) << "Invalid vertical text alignment specified: " << h_align;
  }

  SDL_FRect src_rect{0, 0, kTextCharacterDims.x, kTextCharacterDims.y};
  SDL_FRect dst_rect{p.x, p.y, kTextCharacterDims.x, kTextCharacterDims.y};
  SDL_Texture* font_tex = basic_font_.get()->texture_.get();
  for (const char c : text) {
    src_rect.x = (c & 0x1f) * kTextCharacterDims.x;
    src_rect.y = (c >> 5) * kTextCharacterDims.y;
    CHECK_EQ(SDL_RenderTexture(renderer_.get(), font_tex, &src_rect, &dst_rect),
             0)
        << "SDL error (SDL_RenderCopy): " << SDL_GetError();
    dst_rect.x += kTextCharacterDims.x;
  }
}

// TextParagraph

void Gfx::TextParagraph(string_view text, ivec2 a, ivec2 b, Color32 color,
                        TextHAlign h_align, TextVAlign v_align) {
  CheckInit(__func__);
  InternalTextParagraph(nullptr, text, a, b, color, h_align, v_align);
}
void Gfx::TextParagraph(const Image& target, string_view text, ivec2 a, ivec2 b,
                        Color32 color, TextHAlign h_align, TextVAlign v_align) {
  CheckInit(__func__);
  target.CheckTarget(__func__);
  InternalTextParagraph(target.texture_.get(), text, a, b, color, h_align,
                        v_align);
}

void Gfx::InternalTextParagraph(SDL_Texture* texture, string_view text, ivec2 a,
                                ivec2 b, Color32 color, TextHAlign h_align,
                                TextVAlign v_align) {
  SetRenderTarget(texture);
  CHECK_EQ(
      SDL_SetTextureColorMod(basic_font_.get()->texture_.get(), color.channel.r,
                             color.channel.g, color.channel.b),
      0)
      << "SDL error (SDL_SetTextureColorMod): " << SDL_GetError();
  if (a.x > b.x) std::swap(a.x, b.y);
  if (a.y > b.y) std::swap(a.y, b.y);
  const ivec2 box_dims = b - a + ivec2{1, 1};

  if ((box_dims.x < kTextCharacterDims.x) ||
      (box_dims.y < kTextCharacterDims.y))
    return;

  const int lines_height =
      (box_dims.y / kTextCharacterDims.y) * kTextCharacterDims.y;

  SDL_FRect dst_rect{0, a.y, kTextCharacterDims.x, kTextCharacterDims.y};

  switch (v_align) {
    case kTextAlignVTop:
      break;
    case kTextAlignVCenter:
      dst_rect.y += (box_dims.y - lines_height) * 0.5;
      break;
    case kTextAlignVBottom:
      dst_rect.y += box_dims.y - lines_height;
      break;
    default:
      CHECK(false) << "Invalid vertical text alignment specified: " << h_align;
  }

  SDL_FRect src_rect{0, 0, kTextCharacterDims.x, kTextCharacterDims.y};
  SDL_Texture* font_tex = basic_font_.get()->texture_.get();
  int cursor = 0;
  while (cursor < text.size()) {
    int space_skip = 1;
    int line_term = cursor;
    int line_s = cursor;
    for (line_s = cursor;
         (line_s < text.size()) &&
         (((line_s - cursor) * kTextCharacterDims.x) <= box_dims.x);
         ++line_s) {
      if (text[line_s] == ' ') line_term = line_s;
    }
    if (line_term == cursor) {
      line_term = line_s;
      if (text[line_s] != ' ') space_skip = 0;
    }
    const int line_width = (line_term - cursor) * kTextCharacterDims.x;
    switch (v_align) {
      case kTextAlignHLeft:
        dst_rect.x = a.x;
        break;
      case kTextAlignHCenter:
        dst_rect.x = a.x + (box_dims.x - line_width) * 0.5;
        break;
      case kTextAlignHRight:
        dst_rect.x = a.x + box_dims.x - line_width;
        break;
      default:
        CHECK(false) << "Invalid horizontal text alignment specified: "
                     << h_align;
    }

    for (int c_i = cursor; c_i < line_term; ++c_i) {
      const char c = text[c_i];
      src_rect.x = (c & 0x1f) * kTextCharacterDims.x;
      src_rect.y = (c >> 5) * kTextCharacterDims.y;
      CHECK_EQ(
          SDL_RenderTexture(renderer_.get(), font_tex, &src_rect, &dst_rect), 0)
          << "SDL error (SDL_RenderCopy): " << SDL_GetError();
      dst_rect.x += kTextCharacterDims.x;
    }

    cursor = line_term + space_skip;
    dst_rect.y += kTextCharacterDims.y;
  }
}

bool Gfx::GetKeyPressed(Key key) {
  CheckInit(__func__);
  return SDL_GetKeyboardState(nullptr)[key];
}

const std::tuple<glm::ivec3, Gfx::MouseButtonPressedState&> Gfx::GetMouse() {
  return {mouse_pointer_position_, mouse_button_state_};
}

bool Gfx::Close() {
  CheckInit(__func__);
  return close_pressed_;
}

void Gfx::SyncInputs() {
  CheckInit(__func__);
  close_pressed_ = false;
  mouse_button_state_ = {false, false, false};
  ++input_cycle_;
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_EVENT_QUIT:
        close_pressed_ = true;
        break;
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
        HandleMouseButtonEvent(event);
        break;
      case SDL_EVENT_MOUSE_WHEEL:
        mouse_pointer_position_.z += event.wheel.y;
        break;
      case SDL_EVENT_MOUSE_MOTION:
        mouse_pointer_position_.x = event.motion.x;
        mouse_pointer_position_.y = event.motion.y;
        break;
    }
  }
}

void Gfx::HandleMouseButtonEvent(SDL_Event event) {
  switch (event.button.button) {
    case SDL_BUTTON_LEFT:
      mouse_button_state_.left = true;
      return;
    case SDL_BUTTON_MIDDLE:
      mouse_button_state_.center = true;
      return;
    case SDL_BUTTON_RIGHT:
      mouse_button_state_.right = true;
      return;
  }
}
}  // namespace gfx
}  // namespace land15