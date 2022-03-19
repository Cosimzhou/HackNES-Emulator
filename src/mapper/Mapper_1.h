#pragma once
#include "Mapper.h"

namespace hn {
class Mapper_1 : public Mapper {
 public:
  Mapper_1(Cartridge &cart);

  virtual void Reset() override;
  virtual void writePRG(Address addr, Byte value);
  virtual Byte readPRG(Address addr);

  virtual Byte readCHR(Address addr);
  virtual void writeCHR(Address addr, Byte value);

  virtual std::string mapper_name() const { return "MMC1"; }

 private:
  void calculatePRGPointers();

  bool usesCharacterRAM_;
  int modeCHR_;
  int modePRG_;

  Byte tempRegister_;
  int writeCounter_;

  Byte regPRG_;
  Byte regCHR0_;
  Byte regCHR1_;

  const Byte *firstBankPRG_;
  const Byte *secondBankPRG_;
  const Byte *firstBankCHR_;
  const Byte *secondBankCHR_;
};
}  // namespace hn
