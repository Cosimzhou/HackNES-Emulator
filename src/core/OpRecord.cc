#include "OpRecord.h"

#include <fstream>

#include "glog/logging.h"

namespace hn {

OperatingRecord::OperatingRecord() : joypad_record_(2) {}

void OperatingRecord::Record(int player, size_t cycle, Byte code) {
  joypad_record_[player].emplace(cycle);
}

Byte OperatingRecord::Read(int player, size_t cycle) const {
  if (cycle > top_cycle_) {
    if (finish_event_) finish_event_();
    return 0x40;
  }

  auto &record = joypad_record_[player];
  auto iter = record.find(cycle);
  if (iter != record.end()) {
    return 0x41;
  }

  return 0x40;
}

void OperatingRecord::Load(const std::string &path) {
  std::ifstream file(path, std::ios::in | std::ios::binary);
  Restore(file);
  file.close();

  LOG(INFO) << "Load record done! " << joypad_record_.size()
            << " Players, and joy1 acts " << joypad_record_[0].size()
            << " joy2 acts " << joypad_record_[1].size();

  file_path_ = path;
}

void OperatingRecord::Save() { Save(file_path_); }

void OperatingRecord::Save(const std::string &path) {
  std::ofstream file(path, std::ios::out | std::ios::binary);
  Save(file);
  file.close();

  LOG(INFO) << "Load save done! " << joypad_record_.size()
            << " Players, and joy1 acts " << joypad_record_[0].size()
            << " joy2 acts " << joypad_record_[1].size();
  file_path_ = path;
}

void OperatingRecord::Save(std::ostream &os) {
  WriteNum(os, joypad_record_.size());
  WriteNum(os, joypad_record_[0].size());
  WriteNum(os, joypad_record_[1].size());
  WriteLNum(os, top_cycle_);

  for (int i = 0; i < 2; i++) {
    for (auto rcd : joypad_record_[i]) {
      WriteLNum(os, rcd);
    }
  }
}

void OperatingRecord::Restore(std::istream &is) {
  auto size = ReadNum(is);
  if (size != 2) {
    LOG(ERROR) << "Mismatch size: " << size;
    return;
  }

  joypad_record_.resize(2);
  joypad_record_[0].clear();
  joypad_record_[1].clear();

  auto size1 = ReadNum(is);
  size = ReadNum(is);
  top_cycle_ = ReadLNum(is);

  while (size1-- > 0) {
    size_t cycle = ReadLNum(is);
    joypad_record_[0].emplace(cycle);
  }

  while (size-- > 0) {
    size_t cycle = ReadLNum(is);
    joypad_record_[1].emplace(cycle);
  }
}

}  // namespace hn
