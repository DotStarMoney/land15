#include "glog/logging.h"
#include "gfx/gfx.h"


int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);

  land15::gfx::Gfx::Screen({640, 480});

  while (!land15::gfx::Gfx::Close()) {
    land15::gfx::Gfx::SyncInputs();

    land15::gfx::Gfx::TextLine("Old Country Buffet", {0, 0});
    land15::gfx::Gfx::Flip();
  }

  return 0;
}
