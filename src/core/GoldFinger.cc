#include "GoldFinger.h"

#include <cstdlib>
#include <fstream>
#include "glog/logging.h"
namespace hn {

GoldFinger::GoldFinger(MainBus& bus) : bus_(bus), working_(false) {}
void GoldFinger::LoadFile(const std::string& filepath) {
  std::ifstream ifs(filepath);
  if (!ifs.good()) {
    return;
  }

  std::string addr;
  int val;
  char* p;
  while (!ifs.eof()) {
    ifs >> addr >> val;

    Address vaddr = strtol(addr.c_str(), &p, 16);
    VLOG(2) << "Address " << vaddr << " val: " << val;

    SetPatch(vaddr, val);
  }
}

void GoldFinger::Enable() {
  VLOG(2) << "GoldFinger enabled";
  working_ = true;
}

void GoldFinger::Disable() {
  VLOG(2) << "GoldFinger disabled";
  working_ = false;
}

void GoldFinger::Toggle() {
  working_ = !working_;
  VLOG(2) << "GoldFinger " << (working_ ? "enabled" : "disabled");
}

void GoldFinger::Patrol() {
  if (!working_) return;

  for (auto memval : memory_patches_) {
    // Write memval
    bus_.write(memval.first, memval.second);
  }
}

void GoldFinger::SetPatch(Address addr, Byte val) {
  memory_patches_[addr] = val;
  VLOG(2) << "GoldFinger set memory patch at " << addr << " value as " << val;
}

void GoldFinger::ReleasePatch(Address addr) {
  auto iter = memory_patches_.find(addr);
  if (iter != memory_patches_.end()) {
    VLOG(2) << "GoldFinger release memory patch at " << addr;
    memory_patches_.erase(iter);
  }
}

}  // namespace hn
