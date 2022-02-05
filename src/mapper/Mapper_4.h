#pragma once

#include "Mapper.h"

namespace hn {
class Mapper_4 : public Mapper {
 public:
  Mapper_4(Cartridge &cart);

  virtual void Reset() override;
  virtual void writePRG(Address addr, Byte value);
  virtual Byte readPRG(Address addr);

  virtual Byte readCHR(Address addr);
  virtual void writeCHR(Address addr, Byte value);

  virtual std::string mapper_name() const { return "MMC3"; }

  virtual void DebugDump() override;
  virtual void Hsync(int scanline) override;

 protected:
  void updatePPUBank();
  void updateCPUBank();

 private:
  bool usesCharacterRAM_;
  size_t rom_num_;

  std::vector<Byte> characterRAM_;
  // Data: 0b 1   1 ---   111
  //         CHR PRG    Register_Index
  Byte targetRegister_;
  bool bPRGBankMode;
  bool bCHRInversion;  // Invert

  std::vector<Byte> pRegister;

  std::vector<Address> pCHRBank;
  std::vector<Address> pPRGBank;

  bool bIRQActive;
  bool bIRQEnable;
  bool bIRQUpdate;

  Byte nIRQCounter;
  Byte nIRQReload;
  Byte nLatch_;
};

}  // namespace hn
