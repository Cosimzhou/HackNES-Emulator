#pragma once

#include "common.h"
namespace hn {

class Helper {
 public:
  static std::string GenImageCaptureName();
  static std::string SearchDefaultFont();
  static std::string NewFileName(const std::string &hint);
};

}  // namespace hn
