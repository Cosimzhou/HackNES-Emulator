#pragma once

#include "Mapper.h"

namespace hn {
class Mapper_76 : public Mapper {
 public:
  Mapper_76(Cartridge &cart);

  virtual void Reset() override;
  virtual void writePRG(Address addr, Byte value);
  virtual Byte readPRG(Address addr);

  virtual Byte readCHR(Address addr);
  virtual void writeCHR(Address addr, Byte value);

  virtual std::string mapper_name() const { return "NAMCOT-3446"; }

  virtual void DebugDump() override;

 protected:
 private:
  std::vector<Byte> regs_;
  Byte selReg_;
  size_t prgRom_;
};

}  // namespace hn
