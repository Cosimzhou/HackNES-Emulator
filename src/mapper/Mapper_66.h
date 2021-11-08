#pragma once

#include "Mapper.h"

namespace hn {
class Mapper_66 : public Mapper {
 public:
  Mapper_66(Cartridge &cart);
  ~Mapper_66();

  virtual void Reset() override;
  virtual void writePRG(Address addr, Byte value);
  virtual Byte readPRG(Address addr);

  virtual Byte readCHR(Address addr);
  virtual void writeCHR(Address addr, Byte value);

  virtual std::string mapper_name() const { return "GxROM"; }

  virtual void DebugDump() override;

 protected:
 private:
  Byte prgBank_;
  Byte chrBank_;
  Byte prgRom_;
  Byte chrRom_;
};

}  // namespace hn
