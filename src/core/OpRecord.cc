#include "OpRecord.h"

#include <fstream>

#include "glog/logging.h"

namespace hn {

#define FREAD(f, var) f.read(reinterpret_cast<char *>(&var), sizeof(size_t))
#define FWRITE(f, var) f.write(reinterpret_cast<char *>(&var), sizeof(size_t))

OperatingRecord::OperatingRecord() : joypad_record_(2) {}

void OperatingRecord::Record(int player, size_t cycle, Byte code) {
  joypad_record_[player].emplace(cycle);
}

Byte OperatingRecord::Read(int player, size_t cycle) const {
  auto &record = joypad_record_[player];
  auto iter = record.find(cycle);
  if (iter != record.end()) {
    return 0x41;
  }

  return 0x40;
}

void OperatingRecord::Load(const std::string &path) {
  std::ifstream file(path, std::ios::in | std::ios::binary);

  auto size = joypad_record_.size();
  FREAD(file, size);
  if (size != 2) {
    LOG(ERROR) << "Mismatch size: " << size;
    return;
  }

  joypad_record_.resize(2);
  joypad_record_[0].clear();
  joypad_record_[1].clear();

  auto size1 = size;
  FREAD(file, size1);
  FREAD(file, size);

  top_cycle_ = 0;
  while (size1-- > 0) {
    size_t cycle;
    FREAD(file, cycle);
    joypad_record_[0].emplace(cycle);
    top_cycle_ = std::max(top_cycle_, cycle);
  }

  while (size-- > 0) {
    size_t cycle;
    FREAD(file, cycle);
    joypad_record_[1].emplace(cycle);
    top_cycle_ = std::max(top_cycle_, cycle);
  }

  file.close();

  LOG(INFO) << "Load record done! " << joypad_record_.size()
            << " Players, and joy1 acts " << joypad_record_[0].size()
            << " joy2 acts " << joypad_record_[1].size();

  file_path_ = path;
}

void OperatingRecord::Save() { Save(file_path_); }

void OperatingRecord::Save(const std::string &path) {
  std::ofstream file(path, std::ios::out | std::ios::binary);

  size_t buf = joypad_record_.size();
  FWRITE(file, buf);
  buf = joypad_record_[0].size();
  FWRITE(file, buf);
  buf = joypad_record_[1].size();
  FWRITE(file, buf);

  for (int i = 0; i < 2; i++) {
    for (auto rcd : joypad_record_[i]) {
      buf = rcd;
      FWRITE(file, buf);
    }
  }

  file.close();

  LOG(INFO) << "Load save done! " << joypad_record_.size()
            << " Players, and joy1 acts " << joypad_record_[0].size()
            << " joy2 acts " << joypad_record_[1].size();
  file_path_ = path;
}

}  // namespace hn
