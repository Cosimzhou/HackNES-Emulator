#include "Mapper_66.h"

#include "glog/logging.h"

namespace hn {

Mapper_66::Mapper_66(Cartridge &cart) : Mapper(cart, 66) {
  prgBank_ = 0;
  chrBank_ = 0;

  prgRom_ = cart.getROM().size() >> 10;
  chrRom_ = cart.getVROM().size() >> 10;
}

Mapper_66::~Mapper_66() {}

void Mapper_66::Reset() {
  prgBank_ = 0;
  chrBank_ = 0;

  prgRom_ = cartridge_.getROM().size() >> 10;
  chrRom_ = cartridge_.getVROM().size() >> 10;
}

void Mapper_66::writePRG(Address addr, Byte data) {
  prgBank_ = (data >> 4) & 3;
  chrBank_ = data & 3;
}

Byte Mapper_66::readPRG(Address addr) {  //
  FileAddress prg_addr = 0;
  if (addr >= 0x8000) {  // 0x2000
    prg_addr = prgBank_;
    prg_addr <<= 15;  // 32KB
    prg_addr += addr & 0x7FFF;
  } else {
    LOG(ERROR) << "readPRG @" << std::hex << addr;
    return 0;
  }

  return cartridge_.getROM()[prg_addr];
}

Byte Mapper_66::readCHR(Address addr) {
  if (cartridge_.getVROM().size() == 0) {
    LOG(ERROR) << "no vrom but read" << std::hex << addr;
    return cartridge_.getVROM()[addr];
  } else {
    FileAddress vrom_addr = chrBank_;
    vrom_addr <<= 13;  // 8KB
    vrom_addr += addr & 0x1fff;

    return cartridge_.getVROM()[vrom_addr];
  }

  return 0;
}

void Mapper_66::writeCHR(Address addr, Byte value) {
  VLOG(1) << "writeCHR " << std::hex << addr << " " << value;
  //
}

void Mapper_66::DebugDump() {
  // LOG(INFO) << "chrRam:" << std::boolalpha << usesCharacterRAM_
  //          << " chrRamSz:" << characterRAM_.size() << " 8kRom:" << rom_num_
  //          << " cReg:" << +targetRegister_ << " prgBankM:" << bPRGBankMode
  //          << " chrInv:" << bCHRInversion << " irqAct:" << bIRQActive
  //          << " irqEn:" << bIRQEnable << " irqUpd:" << bIRQUpdate
  //          << " irqCnt:" << +nIRQCounter << " irqRel:" << +nIRQReload
  //          << " regs:" << DumpVector(pRegister)
  //          << " prgBank:" << DumpVector(pPRGBank)
  //          << " chrBank:" << DumpVector(pCHRBank);
}
};  // namespace hn
