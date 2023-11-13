#pragma once

#include "Mapper.h"

namespace hn {
class Mapper_23 : public Mapper {
 public:
  Mapper_23(Cartridge &cart);

  virtual void Reset() override;
  virtual void writePRG(Address addr, Byte value);
  virtual Byte readPRG(Address addr);

  virtual Byte readCHR(Address addr);
  virtual void writeCHR(Address addr, Byte value);

  virtual std::string mapper_name() const { return "VRC4"; }

  virtual void DebugDump() override;

 protected:
 private:
  std::vector<Address> chrBanks_;
  std::vector<Address> prgBanks_;
  bool is_vrc2_;
  bool wram_;
  bool swap_;

  Byte nIRQLatch_;
  Byte nIRQControl_;
  Byte nIRQAcknowledge_;
  // Byte prgBank_;
  // std::vector<Byte> vRam_;
  // Byte prgRom_;
  // bool chrVRam_;
};

}  // namespace hn
