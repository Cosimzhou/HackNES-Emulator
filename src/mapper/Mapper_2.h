#pragma once
#include "Mapper.h"

namespace hn {
class Mapper_2 : public Mapper {
 public:
  Mapper_2(Cartridge &cart);
  virtual void Reset() override;
  virtual void writePRG(Address addr, Byte value);
  virtual Byte readPRG(Address addr);

  virtual Byte readCHR(Address addr);
  virtual void writeCHR(Address addr, Byte value);

  virtual std::string mapper_name() const { return "UxROM"; }

 private:
  bool usesCharacterRAM_;

  const Byte *lastBankPtr_;
  Address selectPRG_;

  std::vector<Byte> characterRAM_;
};
}  // namespace hn
