#include "utils.h"
namespace hn {

std::string Helper::GenImageCaptureName() {
  char buffer[1024];
  time_t now = time(nullptr);
  struct tm* ltime = localtime(&now);
  sprintf(buffer, "/tmp/capture-%d%02d%02d_%02d%02d%02d.png",
          1900 + ltime->tm_year, ltime->tm_mon, ltime->tm_mday, ltime->tm_hour,
          ltime->tm_min, ltime->tm_sec);

  return std::string(buffer);
}

std::string Helper::SearchDefaultFont() {
  return "/usr/share/fonts/truetype/freefont/FreeMonoBold.ttf";
}
}  // namespace hn
