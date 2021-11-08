#pragma once
#include <SFML/Graphics.hpp>

namespace hn {
class VirtualScreen : public sf::Drawable {
 public:
  void create(unsigned int width, unsigned int height, float pixel_size,
              sf::Color color);
  void setPixel(std::size_t x, std::size_t y, sf::Color color);
  void resize(float pixel_size);

  void setTip(const std::string &msg);

 private:
  virtual void draw(sf::RenderTarget &target, sf::RenderStates states) const;

  sf::Vector2u screenSize_;
  float pixelSize_;  // virtual pixel size in real pixels
  sf::VertexArray vertices_;

  sf::Font font_;
  mutable sf::Text tipText_;
  mutable int counter_;
};
}  // namespace hn
