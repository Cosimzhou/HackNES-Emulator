#include "utils.h"
namespace hn {

std::string Helper::Timemark() {
  char buffer[1024];
  time_t now = time(nullptr);
  struct tm* ltime = localtime(&now);
  sprintf(buffer, "%d%02d%02d_%02d%02d%02d", 1900 + ltime->tm_year,
          ltime->tm_mon, ltime->tm_mday, ltime->tm_hour, ltime->tm_min,
          ltime->tm_sec);

  return std::string(buffer);
}

std::string Helper::SequenceImageName(const std::string& hint) {
  char buffer[1024];
  sprintf(buffer, "/tmp/capture-%s.png", Timemark().c_str());

  return std::string(buffer);
}

std::string Helper::GenImageCaptureName() {
  char buffer[1024];
  sprintf(buffer, "/tmp/capture-%s.png", Timemark().c_str());

  return std::string(buffer);
}

std::string Helper::SearchDefaultFont() {
  return "/usr/share/fonts/truetype/freefont/FreeMonoBold.ttf";
}

std::string Helper::NewFileName(const std::string& hint) {
  return "/tmp/game.sav";
}

}  // namespace hn
