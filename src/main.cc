#include "core/EmulatorSfml.h"
#include "core/devices/VirtualJoypad.h"
#include "gflags/gflags.h"
#include "glog/logging.h"

DEFINE_string(path, "", "Specify ROM path");
DEFINE_bool(recording, false, "Specify recording mode");
DEFINE_bool(print, false, "Specify recording mode");
DEFINE_string(record, "", "Specify recording file");
DEFINE_string(savefile, "", "Specify save recording file");
DEFINE_double(vrate, 0,
              "Set the width of the emulation screen (height is set "
              "automatically to fit the aspect ratio)");
DEFINE_int32(width, 0,
             "Set the width of the emulation screen (height is set "
             "automatically to fit the aspect ratio)");
DEFINE_int32(height, 0,
             "Set the height of the emulation screen (width is set "
             "automatically to fit the aspect ratio)");

namespace hn {
void parseControllerConf(std::string filepath, JoypadInputConfig &p1,
                         JoypadInputConfig &p2);
}

int main(int argc, char **argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  // Default keybindings
  hn::ControllerInputConfig p1, p2;
  p1.keyboard_ = {sf::Keyboard::J,      sf::Keyboard::K, sf::Keyboard::RShift,
                  sf::Keyboard::Return, sf::Keyboard::W, sf::Keyboard::S,
                  sf::Keyboard::A,      sf::Keyboard::D};
  p2.keyboard_ = {sf::Keyboard::Numpad5, sf::Keyboard::Numpad6,
                  sf::Keyboard::Numpad8, sf::Keyboard::Numpad9,
                  sf::Keyboard::Up,      sf::Keyboard::Down,
                  sf::Keyboard::Left,    sf::Keyboard::Right};

  hn::EmulatorSfml emulator;
  if (FLAGS_vrate > 0) {
    emulator.setVideoScale(FLAGS_vrate);
  }

  if (FLAGS_width) {
    emulator.setVideoWidth(FLAGS_width);
  }

  if (FLAGS_height) {
    emulator.setVideoHeight(FLAGS_height);
  }

  if (FLAGS_path.empty()) {
    LOG(ERROR) << "Argument required: ROM path" << std::endl;
    return 1;
  }

  if (!emulator.LoadCartridge(FLAGS_path)) {
    LOG(ERROR) << "Load ROM failed: " << FLAGS_path << std::endl;
    return 1;
  }

  if (FLAGS_print) {
    return 0;
  }

  if (!FLAGS_record.empty()) {
    if (FLAGS_recording) {
      emulator.workMode_ = hn::Emulator::RECORDING;
      emulator.record_.Save(FLAGS_record);
    } else {
      emulator.workMode_ = hn::Emulator::REPLAY;
      emulator.record_.Load(FLAGS_record);
    }
  } else {
    if (FLAGS_recording) {
      emulator.workMode_ = hn::Emulator::RECORDING;
    }
    emulator.record_.Save("/tmp/test.rcd");
  }

  emulator.SetRecordFile(FLAGS_savefile);

  hn::parseControllerConf("keybindings.conf", p1, p2);
  emulator.setKeys(p1, p2);
  emulator.run();

  return 0;
}
