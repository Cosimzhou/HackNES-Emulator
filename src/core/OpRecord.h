#pragma once

#include <functional>
#include <set>
#include <vector>

#include "common.h"

namespace hn {

class OperatingRecord : public Serialize {
 public:
  OperatingRecord();

  void Record(int player, size_t cycle, Byte code);
  Byte Read(int player, size_t cycle) const;

  void Load(const std::string& path);
  void Save(const std::string& path);
  void Save();

  void setFinishCallback(std::function<void()> finish) {
    finish_event_ = finish;
  }

  virtual void Save(std::ostream& os) override;
  virtual void Restore(std::istream& is) override;

 private:
  std::vector<std::set<size_t>> joypad_record_;
  std::string file_path_;
  size_t top_cycle_;

  std::function<void()> finish_event_;
};

}  // namespace hn
