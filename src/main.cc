#include "core/EmulatorSfml.h"
#include "core/utils.h"
#include "gflags/gflags.h"
#include "glog/logging.h"

DEFINE_bool(replaying, false, "Specify replaying mode");
DEFINE_bool(print, false, "Specify the verboss inform output");
DEFINE_string(record, "", "Specify recording file");
DEFINE_double(vrate, -1, "Set the pixel scale of the emulation screen");
DEFINE_int32(width, -1,
             "Set the width of the emulation screen (height is set "
             "automatically to fit the aspect ratio)");
DEFINE_int32(height, -1,
             "Set the height of the emulation screen (width is set "
             "automatically to fit the aspect ratio)");

int main(int argc, char **argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  hn::Cartridge cart;
  for (int i = 1; i < argc; i++) {
    if (!cart.loadFromFile(argv[i])) {
      LOG(ERROR) << "Load ROM failed: " << argv[i] << std::endl;

      return 1;
    }
  }

  if (FLAGS_print) {
    return 0;
  }

  // Default keybindings
  hn::JoypadInputConfig p1, p2;
  p1.keyboard_ = {sf::Keyboard::J,      sf::Keyboard::K, sf::Keyboard::RShift,
                  sf::Keyboard::Return, sf::Keyboard::W, sf::Keyboard::S,
                  sf::Keyboard::A,      sf::Keyboard::D};
  p2.keyboard_ = {sf::Keyboard::Numpad5, sf::Keyboard::Numpad6,
                  sf::Keyboard::Numpad8, sf::Keyboard::Numpad9,
                  sf::Keyboard::Up,      sf::Keyboard::Down,
                  sf::Keyboard::Left,    sf::Keyboard::Right};
  hn::parseControllerConf("keybindings.conf", p1, p2);

  hn::EmulatorSfml emulator;
  emulator.setKeys(p1, p2);
  emulator.setVideoScale(FLAGS_vrate);
  emulator.setVideoWidth(FLAGS_width);
  emulator.setVideoHeight(FLAGS_height);
  emulator.setCartridge(cart);

  emulator.SetRecordMode(FLAGS_replaying, FLAGS_record);

  emulator.run();

  return 0;
}
