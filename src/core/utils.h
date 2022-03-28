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
  static std::string SequenceImageName(const std::string &hint);
};

extern void parseControllerConf(std::string filepath, JoypadInputConfig &p1,
                                JoypadInputConfig &p2);

}  // namespace hn
