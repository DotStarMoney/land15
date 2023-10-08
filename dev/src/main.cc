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
    float island_rock_prob;
    float island_tree_prob;
    float island_grow_cycles;
    float island_height_offset;
    float island_fixed_height_p;
  };

  Land15(const Land15Config& config)
      : config(config),
        board_a(config.w * config.h, Square{.state = Lulc::kWater}),
        board_b(config.w * config.h, Square{.state = Lulc::kWater}),
        parity(true) {
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

  void AdvanceDay() {
    auto src_board_ptr = &board_a;
    auto dst_board_ptr = &board_b;
    if (!parity) std::swap(src_board_ptr, dst_board_ptr);
    auto& src_board = *src_board_ptr;
    auto& dst_board = *dst_board_ptr;

    int cell_offset = config.w + 1;
    for (int y = 1; y < (config.h - 1); ++y) {
      for (int x = 1; x < (config.w - 1); ++x) {
        switch (src_board[cell_offset].state) {
          case Lulc::kTrees: {
            break;
          }
          default:
            // Nothing happens.
        }

        // Strike lightning maybe.
        // Add up humidity, maybe start rain.

        ++cell_offset;
      }
      cell_offset += config.w + 2;
    }
  }

 protected:
  struct Square {
    Lulc state;
    bool burning;

    // In meters.
    float elevation;

    // In celcius.
    float temperature;

    // Absolute humidity in g/m^3.
    float humidity;

    // Absolute below-ground water density g/m^3.
    float inundation;

    // Soil nutrients (N, K, P) in g/m^3 (same as PPM).
    float nutrients;

    // Noxious particulate matter in g/m^3 (same as PPM).
    float pollution;

    // Above ground carbon in g/m^2.
    float biomass;
  };

  std::vector<Square> board_a;
  std::vector<Square> board_b;
  bool parity;
  const Land15Config config;

 private:
  struct Harmonic {
    const float offset;
    const float amplitude;
  };

  static constexpr float kIslandHarmonicAmplitudeMin = 0.2;

  void InitState() {
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

        board_a[y * config.w + x].state = square_state;
      }
    }

    // Add some rocks and trees.
    for (int y = 1; y < config.h - 1; ++y) {
      for (int x = 1; x < config.w - 1; ++x) {
        int offset = y * config.w + x;
        if (board_a[offset].state == Lulc::kWater) continue;
        if (common::rndd() < config.island_rock_prob) {
          board_a[offset].state = Lulc::kRock;
        } else if (common::rndd() < config.island_tree_prob) {
          if (board_a[offset].state == Lulc::kSand) continue;
          board_a[offset].state = Lulc::kTrees;
        }
      }
    }

    // Grow out the rocks and trees.
    std::vector<Square> temp_state(config.w * config.h,
                                   {.state = Lulc::kWater});
    auto& prev_board = board_a;
    auto& cur_board = temp_state;
    for (int i = 0; i < config.island_grow_cycles; ++i) {
      for (int y = 1; y < config.h - 1; ++y) {
        for (int x = 1; x < config.w - 1; ++x) {
          int offset = y * config.w + x;
          cur_board[offset].state = prev_board[offset].state;
          if (prev_board[offset].state == Lulc::kWater) continue;

          int tree_count = 0;
          int rock_count = 0;
          for (const auto& d : {glm::vec2{0, -1}, glm::vec2{1, 0},
                                glm::vec2{0, 1}, glm::vec2{-1, 0}}) {
            int ox = x + d.x;
            int oy = y + d.y;
            const auto& lulc = prev_board[oy * config.w + ox].state;
            if (lulc == Lulc::kTrees) {
              ++tree_count;
            } else if (lulc == Lulc::kRock) {
              ++rock_count;
            }
          }
          Lulc type;
          int count;
          if (tree_count > rock_count) {
            if (prev_board[offset].state == Lulc::kSand) continue;
            type = Lulc::kTrees;
            count = tree_count;
          } else {
            type = Lulc::kRock;
            count = rock_count;
          }
          float p = static_cast<float>(count) / 4.0f;
          if (common::rndd() >= p) continue;
          cur_board[offset].state = type;
        }
      }
      std::swap(cur_board, prev_board);
    }
    board_a = cur_board;
  }

  static constexpr int kElevationRelaxIterations = 200;
  static constexpr int kElevationRelaxKernelWidth = 3;
  static constexpr int kElevationRelaxKernelHeight = 3;
  static constexpr int kElevationRelaxKernelOffsetX = -1;
  static constexpr int kElevationRelaxKernelOffsetY = -1;
  static constexpr float kElevationScale = 1000.0;
  static constexpr float kElevationRelaxKernel[] = {
      0.0625, 0.125, 0.0625, 0.125, 0.25, 0.125, 0.0625, 0.125, 0.0625};

  static constexpr float kInitTemperature = 22;       // C
  static constexpr float kInitHumidity = 10;          // g/m^3
  static constexpr float kInitInundation = 10e3;      // g/m^3
  static constexpr float kInitPollution = 0;          // g/m^3 = ppm
  static constexpr float kInitNutrientsPlants = 200;  // g/m^3 = ppm
  static constexpr float kInitBiomassTrees = 500;     // g/m^2
  static constexpr float kInitBiomassGrass = 10;      // g/m^2

  void InitFields() {
    // Create elevation surface.
    std::vector<float> init_z(config.w * config.h, 0.0f);
    std::vector<float> fixed_mask(config.w * config.h, false);
    for (int i = 0; i < init_z.size(); ++i) {
      float v = common::rndd();
      float v_sign = std::fabsf(v);
      bool water = board_a[i].state == Lulc::kWater;
      init_z[i] =
          (v * v * v_sign + config.island_height_offset) * (water ? 0.0 : 1.0);
      fixed_mask[i] = static_cast<float>(
          (common::rndd() < config.island_fixed_height_p) || water);
    }

    std::vector<float> z_temp(config.w * config.h * 2, 0.0f);
    for (int i = 0; i < kElevationRelaxIterations; ++i) {
      for (int y = 1; y < config.h - 1; ++y) {
        for (int x = 1; x < config.w - 1; ++x) {
          int offset = y * config.w + 1;
          z_temp[offset * 2 + 1] =
              (1.0 - fixed_mask[offset]) * z_temp[offset * 2] +
              fixed_mask[offset] * init_z[offset];

          int kernel_offset = 0;
          float acc_z = 0.0f;
          for (int dy = 0; dy < kElevationRelaxKernelHeight; ++dy) {
            for (int dx = 0; dx < kElevationRelaxKernelWidth; ++dx) {
              float k = kElevationRelaxKernel[kernel_offset++];
              acc_z += z_temp[offset * 2 + 1 +
                              ((dy + kElevationRelaxKernelOffsetY) * config.w) +
                              (dx + kElevationRelaxKernelOffsetX)] *
                       k;
            }
          }
          z_temp[offset * 2] = acc_z;
        }
      }
    }

    // Populate default field values.
    for (int i = 0; i < board_a.size(); ++i) {
      board_a[i].burning = false;
      board_a[i].elevation = z_temp[2 * i] * kElevationScale;
      board_a[i].temperature = kInitTemperature;
      board_a[i].humidity = kInitHumidity;
      board_a[i].inundation = kInitInundation;
      board_a[i].pollution = kInitPollution;
      auto state = board_a[i].state;
      if ((state == Lulc::kTrees) || (state == Lulc::kGrass)) {
        board_a[i].nutrients = kInitNutrientsPlants;
        board_a[i].biomass =
            (state == Lulc::kTrees) ? kInitBiomassTrees : kInitBiomassGrass;
      } else {
        board_a[i].nutrients = 0.0;
        board_a[i].biomass = 0.0;
      }
    }
  }

  static constexpr int kWarmUpDays = 3650;  // 10 years.

  void InitializeBoard() {
    InitState();
    InitFields();
    for (int i = 0; i < kWarmUpDays; ++i) {
      AdvanceDay();
    }

    // Add two settlements and go!
  }
};

class VisualLand15 : public Land15 {
 public:
  VisualLand15(const VisualLand15&) = delete;
  VisualLand15& operator=(const VisualLand15&) = delete;
  VisualLand15(const Land15Config& config)
      : Land15(config), tiles_(gfx::Image::FromFile(kTileImageFilename)) {}

  void Draw(int frame) const {
    auto& board = parity ? board_a : board_b;
    for (int y = 0; y < config.h; ++y) {
      for (int x = 0; x < config.w; ++x) {
        auto state = board[y * config.w + x].state;
        int x_offset = kLulcToDrawXOffset[static_cast<int>(state)] * kTileSize;
        if (state == Lulc::kWater) {
          x_offset += (frame / kWaterAnimFrames) & 1 ? kTileSize : 0;
        }
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

  land15::gfx::Gfx::Screen({640, 480}, true);

  land15::VisualLand15 sim({.w = 40,
                            .h = 30,
                            .island_radius_p = 0.8,
                            .island_river_aspect_p = 0.1,
                            .island_warp_harmonics_n = 2,
                            .island_warp_harmonic_decay = 0.75,
                            .island_warp_harmonic_amplitude = 0.1,
                            .island_rock_prob = 0.01,
                            .island_tree_prob = 0.1,
                            .island_grow_cycles = 5,
                            .island_height_offset = 1.0,
                            .island_fixed_height_p = 0.05});

  int frame_counter = 0;
  while (!land15::gfx::Gfx::Close() ||
         land15::gfx::Gfx::GetKeyPressed(land15::gfx::Gfx::kEscape)) {
    land15::gfx::Gfx::SyncInputs();

    sim.Draw(frame_counter++);

    land15::gfx::Gfx::Flip();
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }

  return 0;
}
