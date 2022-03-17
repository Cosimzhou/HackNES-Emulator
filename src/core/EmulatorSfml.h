#pragma once

#include <SFML/Graphics.hpp>

#include "Emulator.h"

namespace hn {

class EmulatorSfml : public Emulator {
 public:
  EmulatorSfml();

  virtual void run() override;
  virtual void FrameRefresh() override;

 protected:
  void CaptureImage();

 private:
  sf::RenderWindow window_;
};

}  // namespace hn
