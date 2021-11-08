#pragma once

#include "Mapper.h"

namespace hn {
class Mapper_3 : public Mapper {
 public:
  Mapper_3(Cartridge &cart);
  virtual void Reset() override;
  virtual void writePRG(Address addr, Byte value);
  virtual Byte readPRG(Address addr);

  virtual Byte readCHR(Address addr);
  virtual void writeCHR(Address addr, Byte value);

  virtual std::string mapper_name() const { return "CNROM"; }

 private:
  bool oneBank_;

  Address selectCHR_;
};
}  // namespace hn
