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

using namespace land15;

namespace {

const std::string kBackgroundFilename = "res/snowscreen.png";
const std::string kFlakesFilename = "res/flakes.png";

class Snowscreen {
 public:
  Snowscreen(const Snowscreen&) = delete;
  Snowscreen& operator=(const Snowscreen&) = delete;

  Snowscreen(int count, glm::vec2 vel, float jitter, int size)
      : vel_(vel), jitter_(jitter), size_(size) {
    auto res = gfx::Gfx::GetResolution();
    float buffer_x = res.y * -vel.x;
    bounds_x_ = buffer_x < 0 ? glm::vec2(buffer_x, res.x)
                             : glm::vec2(0, res.x + buffer_x);
    bounds_y_ = glm::vec2(-kSnowDim_, res.y + kSnowDim_);
    for (int i = 0; i < count; ++i) {
      flakes_.push_back({
          common::rndd(bounds_x_.x, bounds_x_.y),
          common::rndd(bounds_y_.x, bounds_y_.y),
      });
    }
  }

  void Step() {
    for (auto& flake_p : flakes_) {
      flake_p += vel_ + glm::vec2(common::rndd(-jitter_, jitter_),
                                  common::rndd(-jitter_, jitter_));
      if ((flake_p.x < bounds_x_.x) || (flake_p.x > bounds_x_.y) ||
          (flake_p.y < bounds_y_.x) || (flake_p.y > bounds_y_.y)) {
        flake_p.y = 0;
        flake_p.x = common::rndd(bounds_x_.x, bounds_x_.y);
      }
    }
  }

  void Draw(const gfx::Image& flake_texture) const {
    for (const auto& flake_p : flakes_) {
      gfx::Gfx::Put(
          flake_texture, flake_p - glm::vec2(kSnowDim_, kSnowDim_) * 0.5f,
          {(size_ - 1) * kSnowDim_, 0}, {size_ * kSnowDim_ - 1, kSnowDim_ - 1});
    }
  }

 private:
  static constexpr int kSnowDim_ = 8;

  const glm::vec2 vel_;
  const float jitter_;
  const int size_;
  glm::vec2 bounds_x_;
  glm::vec2 bounds_y_;
  std::vector<glm::vec2> flakes_;
};

}  // namespace

constexpr int kFps = 60;
constexpr int kBaseFlakeCount = 500;

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);

  gfx::Gfx::Screen({320, 200}, true, "It's Snowtime!", {640, 400});

  auto bg = gfx::Image::FromFile(kBackgroundFilename);
  auto flakes = gfx::Image::FromFile(kFlakesFilename);

  Snowscreen snow_back(kBaseFlakeCount, {0.5, 1}, 0.25, 1);
  Snowscreen snow_mid(kBaseFlakeCount * 0.25, {1, 2}, 0.5, 2);
  Snowscreen snow_front(kBaseFlakeCount * 0.1, {2, 4}, 1, 3);


  while (!gfx::Gfx::Close() || gfx::Gfx::GetKeyPressed(gfx::Gfx::kEscape)) {
    gfx::Gfx::SyncInputs();

    snow_back.Step();
    snow_mid.Step();
    snow_front.Step();


    // Backdrop
    gfx::Gfx::Put(*bg, {0, 0}, {0, 0}, {319, 199});
    snow_back.Draw(*flakes);

    // Trees
    gfx::Gfx::Put(*bg, {0, 0}, {320, 0}, {639, 199});
    snow_mid.Draw(*flakes);

    // Mounds
    gfx::Gfx::Put(*bg, {0, 0}, {640, 0}, {959, 199});
    snow_front.Draw(*flakes);


    gfx::Gfx::Flip();
    std::this_thread::sleep_for(
        std::chrono::milliseconds(static_cast<int>(1000.0 / kFps)));
  }

  return 0;
}
