#pragma once
#include <SFML/Graphics.hpp>
#include <chrono>

#include "Cartridge.h"
#include "VirtualScreen.h"

namespace hn {

class PatternViewer {
 public:
  PatternViewer();

  void run();
  void setVideoScale(float scale);

  void setCartridge(Cartridge& cartridge);

  void nextPage();
  void lastPage();

  size_t pageCount() const;

 protected:
  void UpdateImage();

 private:
  VirtualScreen emulatorScreen_;

  Cartridge cartridge_;

  sf::RenderWindow window_;
  float screenScale_;
  size_t colorPattern_;
  size_t pageNum_;
  std::vector<std::vector<sf::Color>> pictureBuffer_;
};

}  // namespace hn
