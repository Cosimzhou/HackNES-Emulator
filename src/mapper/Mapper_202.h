#pragma once

#include "Mapper.h"

namespace hn {
class Mapper_202 : public Mapper {
 public:
  Mapper_202(Cartridge &cart);

  virtual void Reset() override;
  virtual void writePRG(Address addr, Byte value);
  virtual Byte readPRG(Address addr);

  virtual Byte readCHR(Address addr);
  virtual void writeCHR(Address addr, Byte value);

  virtual std::string mapper_name() const { return "150InROM"; }

  virtual void DebugDump() override;

 protected:
 private:
  bool b32KMode_;
  Byte prgBank_;
  bool firstWrite_;
};

}  // namespace hn
