#define _USE_MATH_DEFINES
#include <math.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "common/random.h"
#include "gfx/gfx.h"
#include "gfx/image.h"
#include "glm/geometric.hpp"
#include "glm/vec2.hpp"
#include "glog/logging.h"

namespace land15 {
class Land15 {
 public:
  Land15(const Land15&) = delete;
  Land15& operator=(const Land15&) = delete;

  struct Land15Config {
    int w;
    int h;
    float island_radius_p;
    float island_river_aspect_p;
    int island_warp_harmonics_n;
    float island_warp_harmonic_decay;
    float island_warp_harmonic_amplitude;
  };

  Land15(const Land15Config& config)
      : config(config),
        board(config.w * config.h, Square{.state = Lulc::kWater}) {
    InitializeBoard();
  }

  enum class Lulc : int {
    kUnknown = 0,
    kSand = 1,
    kWater = 2,
    kTrees = 3,
    kLowBuilt = 4,
    kHighBuilt = 5,
    kGrass = 6,
    kRock = 7,
    kAgriculture = 8,
    kWasteland = 9,
    kBare = 10
  };

 protected:
  struct Square {
    Lulc state;
    bool burning;
    float elevation;
    float temperature;
    float humidity;
    float inundation;
    float nutrients;
    float pollution;
    float fuel;
  };

  std::vector<Square> board;
  const Land15Config config;

 private:
  struct Harmonic {
    const float offset;
    const float amplitude;
  };

  static constexpr float kIslandHarmonicAmplitudeMin = 0.2;

  void InitializeBoard() {
    std::vector<bool> land_mask(config.w * config.h, false);

    // Create a land mask with a river running through it.
    float river_a = common::rndd() * M_PI;
    glm::vec2 river_v = {std::cosf(river_a), std::sinf(river_a)};
    float river_d = (common::rndd() * 2.0 - 1.0) * config.island_radius_p * 0.5;
    glm::vec2 river_o = glm::vec2{river_v.y, -river_v.x} * river_d;
    glm::vec2 aspect = glm::vec2{config.w, config.h} /
                       static_cast<float>(std::min(config.w, config.h));

    std::vector<Harmonic> x_harmonics;
    std::vector<Harmonic> y_harmonics;
    for (int i = 0; i < config.island_warp_harmonics_n; ++i) {
      float amplitude = config.island_warp_harmonic_amplitude *
                        std::powf(config.island_warp_harmonic_decay, i);
      x_harmonics.push_back(
          {.offset = static_cast<float>(common::rndd() * M_PI * 2.0),
           .amplitude = amplitude});
      y_harmonics.push_back(
          {.offset = static_cast<float>(common::rndd() * M_PI * 2.0),
           .amplitude = amplitude});
    }

    for (int y = 0; y < config.h; ++y) {
      for (int x = 0; x < config.w; ++x) {
        glm::vec2 n = glm::vec2{static_cast<float>(x) / config.w,
                                static_cast<float>(y) / config.h} *
                          2.0f -
                      1.0f;
        glm::vec2 n_harmonic_aspect =
            n * aspect * static_cast<float>(M_PI * 2.0f);
        glm::vec2 displace(0, 0);
        for (int i = 0; i < config.island_warp_harmonics_n; ++i) {
          const auto& x_harmonic = x_harmonics[i];
          const auto& y_harmonic = y_harmonics[i];
          float x_shift = std::sinf(n_harmonic_aspect.x * (i * 0.5 + 1) +
                                    x_harmonic.offset) *
                          x_harmonic.amplitude;
          float y_shift = std::sinf(n_harmonic_aspect.y * (i * 0.5 + 1) +
                                    x_harmonic.offset) *
                          y_harmonic.amplitude;
          displace += glm::vec2{x_shift, y_shift};
        }
        n += displace;

        bool land = glm::length(n) <= config.island_radius_p;
        glm::vec2 rd = glm::dot(n - river_o, river_v) * river_v + river_o;
        land &= glm::length((n - rd) * aspect) > config.island_river_aspect_p;

        if ((y > 0) && (y < (config.h - 1)) && (x > 0) &&
            (x < (config.w - 1))) {
          land_mask[y * config.w + x] = land;
        }
      }
    }

    // Morphological open.
    std::vector<bool> land_mask_temp(config.w * config.h, false);
    for (int y = 1; y < config.h - 1; ++y) {
      for (int x = 1; x < config.w - 1; ++x) {
        if (!land_mask[y * config.w + x]) continue;
        for (const auto& d : {glm::vec2{0, -1}, glm::vec2{1, 0},
                              glm::vec2{0, 1}, glm::vec2{-1, 0}}) {
          int ox = x + d.x;
          int oy = y + d.y;
          if (!land_mask[oy * config.w + ox])
            goto land15_Land15_InitializeBoard_skip_morph_erode;
        }
        land_mask_temp[y * config.w + x] = true;
      land15_Land15_InitializeBoard_skip_morph_erode:
        continue;
      }
    }

    for (int y = 1; y < config.h - 1; ++y) {
      for (int x = 1; x < config.w - 1; ++x) {
        if (land_mask_temp[y * config.w + x]) {
          land_mask[y * config.w + x] = true;
          continue;
        }

        for (const auto& d : {glm::vec2{0, -1}, glm::vec2{1, 0},
                              glm::vec2{0, 1}, glm::vec2{-1, 0}}) {
          int ox = x + d.x;
          int oy = y + d.y;
          if (land_mask_temp[oy * config.w + ox]) {
            land_mask[y * config.w + x] = true;
            goto land15_Land15_InitializeBoard_skip_morph_dilate;
          }
        }
      land_mask[y * config.w + x] = false;
      land15_Land15_InitializeBoard_skip_morph_dilate:
        continue;
      }
    }

    // Set up the initial board. Everything is grass except for stuff bordering
    // water which is beach.
    for (int y = 1; y < config.h - 1; ++y) {
      for (int x = 1; x < config.w - 1; ++x) {
        if (!land_mask[y * config.w + x]) continue;
        auto square_state = Lulc::kGrass;
        for (const auto& d : {glm::vec2{0, -1}, glm::vec2{1, 0},
                              glm::vec2{0, 1}, glm::vec2{-1, 0}}) {
          int ox = x + d.x;
          int oy = y + d.y;
          if (!land_mask[oy * config.w + ox]) {
            square_state = Lulc::kSand;
            goto land15_Land15_InitializeBoard_skip_init_board_sand;
          }
        }
      land15_Land15_InitializeBoard_skip_init_board_sand:

        board[y * config.w + x].state = square_state;
      }
    }

    // Add rocky outcrops
    // Throw down some trees
    // Add 2 settlements close to water on grass.


  }
};

class VisualLand15 : public Land15 {
 public:
  VisualLand15(const VisualLand15&) = delete;
  VisualLand15& operator=(const VisualLand15&) = delete;
  VisualLand15(const Land15Config& config)
      : Land15(config), tiles_(gfx::Image::FromFile(kTileImageFilename)) {}

  void Draw(int frame) const {
    for (int y = 0; y < config.h; ++y) {
      for (int x = 0; x < config.w; ++x) {
        auto state = board[y * config.w + x].state;
        int x_offset = kLulcToDrawXOffset[static_cast<int>(state)] * kTileSize;
        if (state == Lulc::kWater)
          x_offset += (frame / kWaterAnimFrames) & 1 ? kTileSize : 0;
        gfx::Gfx::Put(*tiles_.get(), {x * kTileSize, y * kTileSize},
                      {x_offset, 0}, {x_offset + kTileSize - 1, kTileSize - 1});
      }
    }
  }

 private:
  static const std::string kTileImageFilename;
  static constexpr int kTileSize = 16;
  static constexpr int kWaterAnimFrames = 60;

  static constexpr std::array<int, 11> kLulcToDrawXOffset = {14, 0, 1, 3, 4, 5,
                                                             6,  7, 8, 9, 10};

  std::unique_ptr<gfx::Image> tiles_;
};

const std::string VisualLand15::kTileImageFilename = "res/tiles.png";

}  // namespace land15

constexpr int kFps = 60;

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);

  land15::gfx::Gfx::Screen({640, 480});

  land15::VisualLand15 sim({.w = 40,
                            .h = 30,
                            .island_radius_p = 0.8,
                            .island_river_aspect_p = 0.1,
                            .island_warp_harmonics_n = 2,
                            .island_warp_harmonic_decay = 0.75,
                            .island_warp_harmonic_amplitude = 0.1});

  int frame_counter = 0;
  while (!land15::gfx::Gfx::Close()) {
    land15::gfx::Gfx::SyncInputs();

    sim.Draw(frame_counter++);

    land15::gfx::Gfx::Flip();
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }

  return 0;
}
