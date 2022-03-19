#pragma once
#include <SFML/Graphics.hpp>
#include <chrono>

#include "Cartridge.h"
#include "devices/SfmlScreen.h"

namespace hn {

class PatternViewer {
 public:
  PatternViewer();

  void run();
  void setVideoScale(float scale);

  void setCartridge(Cartridge& cartridge);
  void setRom(const Memory* vRom);

  void nextPage();
  void lastPage();

  size_t pageCount() const;

 protected:
  void UpdateImage();

 private:
  VirtualScreenSfml emulatorScreen_;

  Cartridge cartridge_;
  const Memory* vRom_;

  sf::RenderWindow window_;
  float screenScale_;
  int colorPattern_;
  int colorShift_;
  size_t pageNum_;
  std::vector<std::vector<Color>> pictureBuffer_;
};

}  // namespace hn
