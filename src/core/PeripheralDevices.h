#pragma once

#include "common.h"
namespace hn {

class VirtualScreen {
 public:
  virtual void create(unsigned int width, unsigned int height, float pixel_size,
                      Color color) = 0;
  virtual void setPixel(std::size_t x, std::size_t y, Color color) = 0;
  virtual void resize(float pixel_size) = 0;

  virtual void setTip(const std::string &msg) = 0;
};

class VirtualSpeaker {
 public:
  VirtualSpeaker(unsigned int channel = 1, unsigned int sample_rate = 44100) {}
  virtual ~VirtualSpeaker() = default;

  virtual void PushSample(std::int16_t *data, size_t count) = 0;
  virtual void Play() = 0;
  virtual void Stop() = 0;
};

class JoypadInputConfig {
 public:
  virtual bool isPressed(int) const = 0;
};

class VirtualJoypad {
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

  virtual void strobe(Byte b) = 0;
  virtual Byte read() = 0;
  virtual void setKeyBindings(const JoypadInputConfig &keys) = 0;
};

}  // namespace hn