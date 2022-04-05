#pragma once

#include "PeripheralDevices.h"
#include "common.h"
namespace hn {

class Helper {
 public:
  static std::string GenImageCaptureName();
  static std::string SearchDefaultFont();
  static std::string NewFileName(const std::string &hint);

  static std::string Timemark();
  static std::string SequenceImageName();
  static std::string GenSoundRecordName();

  static std::string rootPath();
  static void setRootPath(const std::string &rootPath);

  static std::string tag();
  static void setTag(const std::string &tag);

 private:
  static std::string root_path_;
  static std::string tag_;
};

extern void parseControllerConf(std::string filepath, JoypadInputConfig &p1,
                                JoypadInputConfig &p2);

}  // namespace hn
