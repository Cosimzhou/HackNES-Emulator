#include "Controller.h"

#include "glog/logging.h"

namespace hn {
Controller::Controller() : keyStates_(0), input_() {}

void Controller::setKeyBindings(const JoypadInputConfig &keys) {
  input_ = dynamic_cast<const ControllerInputConfig &>(keys);

  if (input_.joystick_.inUse) {
    if (!sf::Joystick::isConnected(input_.joystick_.index)) {
      LOG(WARNING) << "Joystick " << input_.joystick_.index << " not connected";
    }
  }
}

void Controller::strobe(Byte b) {
  strobe_ = (b & 1);
  if (!strobe_) {
    keyStates_ = 0;
    for (int button = A, shift = 0; button < TotalButtons; ++button, ++shift) {
      keyStates_ |= input_.isPressed(button) << shift;
    }
  }
}

Byte Controller::read() {
  Byte ret;
  if (strobe_) {
    ret = input_.isPressed(A);
  } else {
    ret = (keyStates_ & 1);
    keyStates_ >>= 1;
  }
  return ret | 0x40;
}

JoystickConfig::JoystickConfig()
    : inUse(false), index(0), keyBindings_(Controller::TotalButtons) {}

ControllerInputConfig::ControllerInputConfig()
    : keyboard_(Controller::TotalButtons), joystick_() {}

bool ControllerInputConfig::isPressed(int key) const {
  if (sf::Keyboard::isKeyPressed(keyboard_[key])) {
    return true;
  }

  if (joystick_.inUse) {
    if (key > 3) {
      int val = joystick_.keyBindings_[key];
      if (val < 0) {
        return sf::Joystick::getAxisPosition(
                   joystick_.index,
                   static_cast<sf::Joystick::Axis>(-(val + 1))) < 0;
      } else {
        return sf::Joystick::getAxisPosition(
                   joystick_.index, static_cast<sf::Joystick::Axis>(val)) > 0;
      }
    } else {
      return sf::Joystick::isButtonPressed(joystick_.index,
                                           joystick_.keyBindings_[key]);
    }
  }

  return false;
}

}  // namespace hn
