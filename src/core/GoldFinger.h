#pragma once

#include <unordered_map>

#include "MainBus.h"

namespace hn {

class GoldFinger {
 public:
  GoldFinger(MainBus &bus);

  void LoadFile(const std::string &filepath);
  void Enable();
  void Disable();
  void Toggle();

  void Patrol();

  void SetPatch(Address addr, Byte val);
  void ReleasePatch(Address addr);

  bool IsWorking() const { return working_; }

 private:
  MainBus &bus_;
  bool working_;

  std::unordered_map<Address, Byte> memory_patches_;
};

}  // namespace hn
