#include "utils.h"

namespace hn {
std::string Helper::root_path_ = "record";
std::string Helper::tag_ = "fcgame";
std::string Helper::rootPath() { return root_path_; }
void Helper::setRootPath(const std::string& rootPath) { root_path_ = rootPath; }
std::string Helper::tag() { return tag_; }
void Helper::setTag(const std::string& tag) { tag_ = tag; }

std::string Helper::Timemark() {
  char buffer[1024];
  time_t now = time(nullptr);
  struct tm* ltime = localtime(&now);
  sprintf(buffer, "%d%02d%02d_%02d%02d%02d", 1900 + ltime->tm_year,
          ltime->tm_mon + 1, ltime->tm_mday, ltime->tm_hour, ltime->tm_min,
          ltime->tm_sec);

  return std::string(buffer);
}

std::string Helper::SequenceImageName() {
  char buffer[1024];
  sprintf(buffer, "%s/tmp/%s-capture-%s.bmp", root_path_.c_str(), tag_.c_str(),
          Timemark().c_str());

  return std::string(buffer);
}

std::string Helper::GenSoundRecordName() {
  char buffer[1024];
  sprintf(buffer, "%s/snds/%s-capture-%s.wav", root_path_.c_str(), tag_.c_str(),
          Timemark().c_str());

  return std::string(buffer);
}

std::string Helper::GenImageCaptureName() {
  char buffer[1024];
  sprintf(buffer, "%s/pics/%s-capture-%s.png", root_path_.c_str(), tag_.c_str(),
          Timemark().c_str());

  return std::string(buffer);
}

std::string Helper::SearchDefaultFont() {
  return "/usr/share/fonts/truetype/freefont/FreeMonoBold.ttf";
}

std::string Helper::NewFileName(const std::string& hint) {
  char buffer[1024];
  sprintf(buffer, "%s/save/%s-%s.sav", root_path_.c_str(), tag_.c_str(),
          Timemark().c_str());
  return std::string(buffer);
}

}  // namespace hn
