#pragma once
#include <SFML/Window.hpp>
#include <cstdint>

#include "../PeripheralDevices.h"
#include "../common.h"

namespace hn {

class ControllerInputConfig : public JoypadInputConfig {
 public:
  ControllerInputConfig();

  virtual bool isPressed(int) const;
};

class VirtualJoypadSfml : public VirtualJoypad {
 public:
  VirtualJoypadSfml();

  virtual void strobe(Byte b);
  virtual Byte read();
  virtual void setKeyBindings(const JoypadInputConfig &keys);

 private:
  bool strobe_;
  unsigned int keyStates_;

  ControllerInputConfig input_;
};
}  // namespace hn
