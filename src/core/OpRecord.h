#pragma once

#include <set>
#include <vector>

#include "common.h"

namespace hn {

class OperatingRecord {
 public:
  OperatingRecord();

  void Record(int player, size_t cycle, Byte code);
  Byte Read(int player, size_t cycle) const;

  void Load(const std::string &path);
  void Save(const std::string &path);
  void Save();

 private:
  std::vector<std::set<size_t>> joypad_record_;
  std::string file_path_;
  size_t top_cycle_;
};

}  // namespace hn
