#pragma once

#include "Mapper.h"

namespace hn {
class Mapper_15 : public Mapper {
 public:
  Mapper_15(Cartridge &cart);

  virtual void Reset() override;
  virtual void writePRG(Address addr, Byte value);
  virtual Byte readPRG(Address addr);

  virtual Byte readCHR(Address addr);
  virtual void writeCHR(Address addr, Byte value);

  virtual std::string mapper_name() const { return "100InROM"; }

  virtual void DebugDump() override;

 protected:
 private:
  Byte prgBankMode_;
  std::vector<FileAddress> bankAddr_;
  Byte prgRom_;
  bool chrVRam_;
  bool protectCHR_;
};

}  // namespace hn
