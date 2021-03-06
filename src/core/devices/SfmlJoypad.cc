#include "SfmlJoypad.h"

#include "glog/logging.h"

namespace hn {
VirtualJoypadSfml::VirtualJoypadSfml() : keyStates_(0), input_() {}

void VirtualJoypadSfml::setKeyBindings(const JoypadInputConfig &keys) {
  input_ = keys;

  if (input_.joystick_.inUse) {
    if (!sf::Joystick::isConnected(input_.joystick_.index)) {
      LOG(WARNING) << "Joystick " << input_.joystick_.index << " not connected";
    }
  }
}

void VirtualJoypadSfml::strobe(Byte b) {
  strobe_ = (b & 1);
  if (!strobe_) {
    keyStates_ = 0;
    for (int button = A, shift = 0; button < TotalButtons; ++button, ++shift) {
      keyStates_ |= isPressed(button) << shift;
    }
  }
}

Byte VirtualJoypadSfml::read() {
  Byte ret;
  if (strobe_) {
    ret = isPressed(A);
  } else {
    ret = (keyStates_ & 1);
    keyStates_ >>= 1;
  }
  return ret | 0x40;
}

bool VirtualJoypadSfml::isPressed(int key) const {
  if (sf::Keyboard::isKeyPressed(
          static_cast<sf::Keyboard::Key>(input_.keyboard_[key]))) {
    return true;
  }

  if (input_.joystick_.inUse) {
    if (key >= VirtualJoypad::Buttons::Up) {
      int val = input_.joystick_.keyBindings_[key];
      if (val < 0) {
        return sf::Joystick::getAxisPosition(
                   input_.joystick_.index,
                   static_cast<sf::Joystick::Axis>(-(val + 1))) < -50;
      } else {
        return sf::Joystick::getAxisPosition(
                   input_.joystick_.index,
                   static_cast<sf::Joystick::Axis>(val)) > 50;
      }
    } else {
      return sf::Joystick::isButtonPressed(input_.joystick_.index,
                                           input_.joystick_.keyBindings_[key]);
    }
  }

  return false;
}

}  // namespace hn
