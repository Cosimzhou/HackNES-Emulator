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

  void OnGoldFingerToggle();
  void OnSaveRecord();
  void OnPatternView();
  void OnDebug();
  virtual void OnPause() override;

  void OnPauseToggle();

 private:
  sf::RenderWindow window_;

  class VirtualScreenSfml* sfScreen_;
};

}  // namespace hn
