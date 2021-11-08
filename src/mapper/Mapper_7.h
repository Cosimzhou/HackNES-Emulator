#pragma once

#include "Mapper.h"

namespace hn {
class Mapper_7 : public Mapper {
 public:
  Mapper_7(Cartridge &cart);
  ~Mapper_7();

  virtual void Reset() override;
  virtual void writePRG(Address addr, Byte value);
  virtual Byte readPRG(Address addr);

  virtual Byte readCHR(Address addr);
  virtual void writeCHR(Address addr, Byte value);

  virtual std::string mapper_name() const { return "AxROM"; }

  virtual void DebugDump() override;

 protected:
 private:
  Byte prgBank_;
  std::vector<Byte> vRam_;
  Byte prgRom_;
  bool chrVRam_;
};

}  // namespace hn
