#pragma once

#include "Mapper.h"

namespace hn {
class Mapper_0 : public Mapper {
 public:
  Mapper_0(Cartridge &cart);
  virtual void Reset() override;
  virtual void writePRG(Address addr, Byte value);
  virtual Byte readPRG(Address addr);

  virtual Byte readCHR(Address addr);
  virtual void writeCHR(Address addr, Byte value);

  virtual std::string mapper_name() const { return "NROM"; }
  virtual void DebugDump() override;

  virtual void Save(std::ostream &os) override;
  virtual void Restore(std::istream &is) override;

 private:
  bool oneBank_;
  bool usesCharacterRAM_;
};
}  // namespace hn
