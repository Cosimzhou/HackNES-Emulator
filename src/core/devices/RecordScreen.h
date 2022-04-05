#pragma once

#include <memory>
#include "../PeripheralDevices.h"

namespace hn {
class RecordScreen : public VirtualScreen {
 public:
  RecordScreen(const std::string& file_path);
  virtual ~RecordScreen();

  virtual void create(unsigned int width, unsigned int height, float pixel_size,
                      Color color);
  virtual void setPixel(std::size_t x, std::size_t y, Color color);
  virtual void resize(float pixel_size);

  virtual void setTip(const std::string& msg);

  void SetOutScreen(VirtualScreen* screen);
  VirtualScreen* OutScreen();

 protected:
  void SaveBMP();

 private:
  Memory buffer_;

  std::string file_path_;
  std::unique_ptr<VirtualScreen> screen_;
};

}  // namespace hn
