#pragma once

#include "Mapper.h"

namespace hn {
class Mapper_65 : public Mapper {
 public:
  Mapper_65(Cartridge &cart);

  virtual void Reset() override;
  virtual void writePRG(Address addr, Byte value);
  virtual Byte readPRG(Address addr);

  virtual Byte readCHR(Address addr);
  virtual void writeCHR(Address addr, Byte value);

  virtual std::string mapper_name() const { return "AxROM"; }

  virtual void Tick() override;
  virtual void DebugDump() override;

 protected:
 private:
  std::vector<Byte> prgRegs_;
  std::vector<Byte> chrRegs_;
  Word nIRQReload_;
  Word nIRQCounter_;
  bool bIRQEnable_;
  bool prgSwap_;

  Byte prgBank_;
  Byte prgRom_;
  bool chrVRam_;
};

}  // namespace hn
