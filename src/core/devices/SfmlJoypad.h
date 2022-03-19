#pragma once
#include <SFML/Window.hpp>
#include <cstdint>

#include "../PeripheralDevices.h"
#include "../common.h"

namespace hn {

class VirtualJoypadSfml : public VirtualJoypad {
 public:
  VirtualJoypadSfml();

  virtual void strobe(Byte b);
  virtual Byte read();
  virtual void setKeyBindings(const JoypadInputConfig &keys);

 protected:
  bool isPressed(int key) const;

 private:
  bool strobe_;
  unsigned int keyStates_;

  JoypadInputConfig input_;
};
}  // namespace hn
