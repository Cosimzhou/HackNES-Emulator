#include "SfmlScreen.h"

#include "../PaletteColors.h"
#include "../utils.h"
#include "glog/logging.h"

namespace hn {
void VirtualScreenSfml::create(unsigned int w, unsigned int h, float pixel_size,
                               Color bgColor) {
  vertices_.resize(w * h * 6);
  screenSize_ = {w, h};
  vertices_.setPrimitiveType(sf::Triangles);
  pixelSize_ = pixel_size;
  sf::Color color(colors[bgColor]);
  for (std::size_t x = 0; x < w; ++x) {
    for (std::size_t y = 0; y < h; ++y) {
      auto index = (x * screenSize_.y + y) * 6;
      sf::Vector2f coord2d(x * pixelSize_, y * pixelSize_);

      // Triangle-1
      // top-left
      vertices_[index].position = coord2d;
      vertices_[index].color = color;

      // top-right
      vertices_[index + 1].position = coord2d + sf::Vector2f{pixelSize_, 0};
      vertices_[index + 1].color = color;

      // bottom-right
      vertices_[index + 2].position =
          coord2d + sf::Vector2f{pixelSize_, pixelSize_};
      vertices_[index + 2].color = color;

      // Triangle-2
      // bottom-right
      vertices_[index + 3].position =
          coord2d + sf::Vector2f{pixelSize_, pixelSize_};
      vertices_[index + 3].color = color;

      // bottom-left
      vertices_[index + 4].position = coord2d + sf::Vector2f{0, pixelSize_};
      vertices_[index + 4].color = color;

      // top-left
      vertices_[index + 5].position = coord2d;
      vertices_[index + 5].color = color;
    }
  }

  if (!font_.loadFromFile(Helper::SearchDefaultFont())) {
    LOG(ERROR) << "FreeMono.ttf";
  } else {
    tipText_.setFont(font_);
    tipText_.setPosition(5, screenSize_.y * pixelSize_ - 30);
  }
}

void VirtualScreenSfml::resize(float pixel_size) {
  pixelSize_ = pixel_size;
  for (std::size_t x = 0; x < screenSize_.x; ++x) {
    for (std::size_t y = 0; y < screenSize_.y; ++y) {
      auto index = (x * screenSize_.y + y) * 6;
      sf::Vector2f coord2d(x * pixelSize_, y * pixelSize_);

      // Triangle-1
      // top-left
      vertices_[index].position = coord2d;

      // top-right
      vertices_[index + 1].position = coord2d + sf::Vector2f{pixelSize_, 0};

      // bottom-right
      vertices_[index + 2].position =
          coord2d + sf::Vector2f{pixelSize_, pixelSize_};

      // Triangle-2
      // bottom-right
      vertices_[index + 3].position = vertices_[index + 2].position;

      // bottom-left
      vertices_[index + 4].position = coord2d + sf::Vector2f{0, pixelSize_};

      // top-left
      vertices_[index + 5].position = vertices_[index].position;
    }
  }
}

void VirtualScreenSfml::setPixel(std::size_t x, std::size_t y, Color pcolor) {
  auto index = (x * screenSize_.y + y) * 6;
  if (index < vertices_.getVertexCount()) {
    sf::Color color(colors[pcolor]);
    // Triangle-1
    vertices_[index].color = color;      // top-left
    vertices_[index + 1].color = color;  // top-right
    vertices_[index + 2].color = color;  // bottom-right

    // Triangle-2
    vertices_[index + 3].color = color;  // bottom-right
    vertices_[index + 4].color = color;  // bottom-left
    vertices_[index + 5].color = color;  // top-left
  }
}

void VirtualScreenSfml::draw(sf::RenderTarget &target,
                             sf::RenderStates states) const {
  target.draw(vertices_, states);

  if (counter_ > 0) {
    target.draw(tipText_, states);
    counter_--;
  }
}

void VirtualScreenSfml::setTip(const std::string &msg) {
  counter_ = 60;

  tipText_.setFont(font_);
  tipText_.setCharacterSize(14);
  tipText_.setStyle(sf::Text::Regular);

  tipText_.setColor(sf::Color::Cyan);
  tipText_.setPosition(5, screenSize_.y * pixelSize_ - 30);

  tipText_.setString(msg);
}

}  // namespace hn
