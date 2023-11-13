#pragma once

#include "Mapper.h"

namespace hn {
class Mapper_226 : public Mapper {
 public:
  Mapper_226(Cartridge &cart);

  virtual void Reset() override;
  virtual void writePRG(Address addr, Byte value);
  virtual Byte readPRG(Address addr);

  virtual Byte readCHR(Address addr);
  virtual void writeCHR(Address addr, Byte value);

  virtual std::string mapper_name() const { return "42in1"; }

  virtual void DebugDump() override;

 protected:
 private:
  Byte prgBank_;
  Byte pageSizeBit_;
  std::vector<Byte> vRam_;
  Byte prgRom_;
  bool chrVRam_;
  bool swap_;
  bool writeProtected_;
};

}  // namespace hn
