#include "PatternViewer.h"
#include "PaletteColors.h"
#include "glog/logging.h"

#include <chrono>
#include <thread>

namespace hn {

constexpr size_t kVROMPageSize = 8192;
constexpr size_t kBytesInOneSprite = 16;
constexpr size_t kTilesInOnePage = kVROMPageSize / kBytesInOneSprite;
constexpr size_t kShowTilesOneLine = 32;

const int kMaskColorPatternIndice[][4] = {
    {15, 0x05, 0x1a, 0x11},
    {15, 0x11, 0x2b, 0x15},
    {15, 0x25, 0x2b, 0x21},
};

PatternViewer::PatternViewer()
    : screenScale_(1),
      colorPattern_(0),
      pageNum_(0),
      pictureBuffer_(256, std::vector<sf::Color>(256, sf::Color::Black)) {}

void PatternViewer::run() {
  window_.create(sf::VideoMode(256 * screenScale_, 256 * screenScale_),
                 "Pattern Viewer", sf::Style::Titlebar | sf::Style::Close);
  window_.setVerticalSyncEnabled(true);
  emulatorScreen_.create(256, 256, screenScale_, sf::Color::White);

  UpdateImage();

  sf::Event event;
  bool focus = true, pause = false;
  while (window_.isOpen()) {
    while (window_.pollEvent(event)) {
      if (event.type == sf::Event::Closed ||
          (event.type == sf::Event::KeyPressed &&
           event.key.code == sf::Keyboard::Escape)) {
        window_.close();
        return;
      } else if (event.type == sf::Event::GainedFocus) {
        focus = true;
      } else if (event.type == sf::Event::LostFocus) {
        focus = false;
      } else if (event.type == sf::Event::KeyPressed &&
                 event.key.code == sf::Keyboard::F2) {
        pause = !pause;
      } else if (focus && event.type == sf::Event::KeyReleased &&
                 event.key.code == sf::Keyboard::Left) {
        colorPattern_--;
        if (colorPattern_ >= LEN_ARRAY(kMaskColorPatternIndice))
          colorPattern_ = LEN_ARRAY(kMaskColorPatternIndice) - 1;
        UpdateImage();
      } else if (focus && event.type == sf::Event::KeyReleased &&
                 event.key.code == sf::Keyboard::Right) {
        colorPattern_++;
        if (colorPattern_ >= LEN_ARRAY(kMaskColorPatternIndice))
          colorPattern_ = 0;
        UpdateImage();
      } else if (focus && event.type == sf::Event::KeyReleased &&
                 event.key.code == sf::Keyboard::PageUp) {
        lastPage();
        UpdateImage();
      } else if (focus && event.type == sf::Event::KeyReleased &&
                 event.key.code == sf::Keyboard::PageDown) {
        nextPage();
        UpdateImage();
      }
    }

    for (int x = 0; x < pictureBuffer_.size(); ++x) {
      for (int y = 0; y < pictureBuffer_[0].size(); ++y) {
        emulatorScreen_.setPixel(x, y, pictureBuffer_[x][y]);
      }
    }

    if (focus && !pause) {
      window_.draw(emulatorScreen_);
      window_.display();
    }
    sf::sleep(sf::milliseconds(100));
  }
}

void PatternViewer::UpdateImage() {
  auto maskIndex = kMaskColorPatternIndice[colorPattern_];
  auto& vrom = cartridge_.getVROM();

  LOG(INFO) << vrom.size() << " ppu mem size: " << pictureBuffer_.size() << "x"
            << pictureBuffer_[0].size();

  size_t cellSize = vrom.size() / kBytesInOneSprite;
  for (size_t cell = pageNum_ * kTilesInOnePage, line = 0;
       cell < cellSize && line < kShowTilesOneLine; cell++) {
    size_t addr = cell << 4, tileX = (cell & 0x1f) << 3, lineX = line << 3;
    for (size_t x = 0, mask = 0x80; x < 8; x++, mask >>= 1) {
      for (size_t y = 0; y < 8; y++) {
        int c = (!!(vrom[addr + y] & mask)) |
                ((!!(vrom[addr + y + 8] & mask)) << 1);
        pictureBuffer_[x + tileX][y + lineX] = sf::Color(colors[maskIndex[c]]);
      }
    }

    if (cell % kShowTilesOneLine == kShowTilesOneLine - 1) {
      line++;
    }
  }
}

void PatternViewer::setCartridge(Cartridge& cartridge) {
  cartridge_ = cartridge;

  char buff[1024];
  sprintf(buff, "There are %lu pages in cartridge.", pageCount());
  LOG(INFO) << buff;
  emulatorScreen_.setTip(buff);
}

inline size_t PatternViewer::pageCount() const {
  return cartridge_.getVROM().size() / kVROMPageSize;
}

void PatternViewer::nextPage() {
  if (pageNum_ + 1 < pageCount()) {
    pageNum_++;

    char buff[1024];
    sprintf(buff, "Page: %lu/%lu", pageNum_ + 1, pageCount());
    emulatorScreen_.setTip(buff);
  }
}

void PatternViewer::lastPage() {
  if (pageNum_ > 0) {
    pageNum_--;

    char buff[1024];
    sprintf(buff, "Page: %lu/%lu", pageNum_ + 1, pageCount());
    emulatorScreen_.setTip(buff);
  }
}

void PatternViewer::setVideoScale(float scale) { screenScale_ = scale; }

}  // namespace hn
