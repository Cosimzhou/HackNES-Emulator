#pragma once
#include <SFML/Window.hpp>
#include <cstdint>

#include "PeripheralDevices.h"
#include "common.h"

namespace hn {

typedef struct JoystickConfig {
  bool inUse;
  unsigned int index;
  std::vector<int> keyBindings_;
  JoystickConfig();
} JoystickConfig;

typedef std::vector<sf::Keyboard::Key> KeyboardBinding;

class ControllerInputConfig : public JoypadInputConfig {
 public:
  ControllerInputConfig();
  virtual bool isPressed(int) const;

  KeyboardBinding keyboard_;
  JoystickConfig joystick_;
};

class Controller : public VirtualJoypad {
 public:
  Controller();

  virtual void strobe(Byte b);
  virtual Byte read();
  virtual void setKeyBindings(const JoypadInputConfig &keys);

 private:
  bool strobe_;
  unsigned int keyStates_;

  ControllerInputConfig input_;
};
}  // namespace hn
