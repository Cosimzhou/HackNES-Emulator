#pragma once
#include <SFML/Window.hpp>
#include <cstdint>
#include <vector>

#include "common.h"

namespace hn {

typedef struct JoystickConfig {
  bool inUse;
  unsigned int index;
  std::vector<int> keyBindings_;
  JoystickConfig();
} JoystickConfig;

typedef std::vector<sf::Keyboard::Key> KeyboardBinding;

typedef struct ControllerInputConfig {
  KeyboardBinding keyboard_;
  JoystickConfig joystick_;

  bool isPressed(int) const;
  ControllerInputConfig();

} ControllerInputConfig;

class Controller {
 public:
  enum Buttons {
    A,
    B,
    Select,
    Start,
    Up,
    Down,
    Left,
    Right,
    TotalButtons,
  };
  Controller();

  void strobe(Byte b);
  Byte read();
  void setKeyBindings(const ControllerInputConfig &keys);

 private:
  bool strobe_;
  unsigned int keyStates_;

  ControllerInputConfig input_;
};
}  // namespace hn
