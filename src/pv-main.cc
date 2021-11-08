#include "core/Cartridge.h"
#include "core/PatternViewer.h"
#include "gflags/gflags.h"
#include "glog/logging.h"

DEFINE_string(path, "", "Specify ROM path");
DEFINE_double(vrate, 1, "Specify video scale");

int main(int argc, char **argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  hn::Cartridge cartridge;
  if (FLAGS_path.empty()) {
    LOG(ERROR) << "Argument required: ROM path" << std::endl;
    return 1;
  }

  if (!cartridge.loadFromFile(FLAGS_path)) return 1;

  hn::PatternViewer emulator;
  emulator.setVideoScale(FLAGS_vrate);
  emulator.setCartridge(cartridge);
  emulator.run();

  return 0;
}
