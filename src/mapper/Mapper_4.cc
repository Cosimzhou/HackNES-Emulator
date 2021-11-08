#include "Mapper_4.h"
#include <ios>

#include "glog/logging.h"

namespace hn {

Mapper_4::Mapper_4(Cartridge &cart) : Mapper(cart, 4) {
  if (cart.getVROM().size() == 0) {
    usesCharacterRAM_ = true;
    characterRAM_.resize(0x2000);
    LOG(INFO) << "Uses character RAM";
  } else
    usesCharacterRAM_ = false;

  targetRegister_ = 0x00;
  bPRGBankMode = false;
  bCHRInversion = false;

  bIRQActive = false;
  bIRQEnable = false;
  bIRQUpdate = false;
  nIRQCounter = 0x00;
  nIRQReload = 0x00;

  pRegister.resize(8);
  pCHRBank.resize(8);
  pPRGBank.resize(4);

  pPRGBank[0] = 0;
  pPRGBank[1] = 1;

  rom_num_ = cart.getROM().size() >> 13;
  pPRGBank[2] = rom_num_ - 2;
  pPRGBank[3] = rom_num_ - 1;

  LOG(INFO) << std::hex << pPRGBank[0] << " " << pPRGBank[1] << " "
            << pPRGBank[2] << " " << pPRGBank[3] << " rom: " << rom_num_;
}

Mapper_4::~Mapper_4() {}

void Mapper_4::Reset() {
  if (cartridge_.getVROM().size() == 0) {
    usesCharacterRAM_ = true;
    characterRAM_.resize(0x2000);
    LOG(INFO) << "Uses character RAM";
  } else
    usesCharacterRAM_ = false;

  targetRegister_ = 0x00;
  bPRGBankMode = false;
  bCHRInversion = false;

  bIRQActive = false;
  bIRQEnable = false;
  bIRQUpdate = false;
  nIRQCounter = 0x00;
  nIRQReload = 0x00;

  pRegister.resize(8);
  pCHRBank.resize(8);
  pPRGBank.resize(4);

  pPRGBank[0] = 0;
  pPRGBank[1] = 1;

  rom_num_ = cartridge_.getROM().size() >> 13;
  pPRGBank[2] = rom_num_ - 2;
  pPRGBank[3] = rom_num_ - 1;
}

void Mapper_4::writePRG(Address addr, Byte data) {
  // VLOG(1) << "writePRG " << std::hex << addr << " " << data;
  switch (addr & 0xe001) {
    case 0x8000:
      // 偶数地址修改映射信息
      targetRegister_ = data & 0x07;
      bPRGBankMode = !!(data & 0x40);
      bCHRInversion = !!(data & 0x80);
      break;
    case 0x8001:
      if (targetRegister_ < 6) {
        // Update target register
        pRegister[targetRegister_] = data;
        updatePPUBank();
      } else {
        pRegister[targetRegister_] = data & 0x3f;
        updateCPUBank();
      }
      break;
    case 0xa000:
      ChangeNTMirroring(data & 0x01 ? Horizontal : Vertical);
      break;
    case 0xa001:
      // PRG Ram Protect
      // TODO:
      break;
    case 0xc000:
      nIRQReload = data;
      break;
    case 0xc001:
      nIRQCounter = 0x0000;
      break;
    case 0xe000:
      bIRQEnable = false;
      bIRQActive = false;
      //  IRQ
      //  TODO:
      LOG(INFO) << "irq need";
      break;
    case 0xe001:
      bIRQEnable = true;
      break;
  }
}

Byte Mapper_4::readPRG(Address addr) {  //
  FileAddress prg_addr = 0;
  if (addr >= 0x8000) {  // 0x2000
    prg_addr = pPRGBank[(addr & 0x6000) >> 13];
    prg_addr <<= 13;
    prg_addr += addr & 0x1FFF;
  } else {
    LOG(ERROR) << "readPRG @" << std::hex << addr;
    return 0;
  }

  return cartridge_.getROM()[prg_addr];
}

Byte Mapper_4::readCHR(Address addr) {
  if (cartridge_.getVROM().size() == 0) {
    LOG(ERROR) << "no vrom but read" << std::hex << addr;
    return cartridge_.getVROM()[addr];
  } else {
    FileAddress vrom_addr = pCHRBank[(addr >> 10) & 0x7];
    vrom_addr <<= 10;
    vrom_addr += addr & 0x3ff;

    return cartridge_.getVROM()[vrom_addr];
  }

  return 0;
}

void Mapper_4::writeCHR(Address addr, Byte value) {
  VLOG(1) << "writeCHR " << std::hex << addr << " " << value;
  //
}

void Mapper_4::updateCPUBank() {
  if (bPRGBankMode) {
    pPRGBank[2] = (pRegister[6] & 0x3F) % rom_num_;
    pPRGBank[0] = rom_num_ - 2;
  } else {
    pPRGBank[0] = (pRegister[6] & 0x3F) % rom_num_;
    pPRGBank[2] = rom_num_ - 2;
  }
  pPRGBank[1] = (pRegister[7] & 0x3F) % rom_num_;
  pPRGBank[3] = rom_num_ - 1;
}

void Mapper_4::updatePPUBank() {
  // Update Pointer Table
  if (bCHRInversion) {
    pCHRBank[0] = pRegister[2];
    pCHRBank[1] = pRegister[3];
    pCHRBank[2] = pRegister[4];
    pCHRBank[3] = pRegister[5];
    pCHRBank[4] = (pRegister[0] & 0xFE);
    pCHRBank[5] = pRegister[0] | 1;
    pCHRBank[6] = (pRegister[1] & 0xFE);
    pCHRBank[7] = pRegister[1] | 1;
  } else {
    pCHRBank[0] = (pRegister[0] & 0xFE);
    pCHRBank[1] = pRegister[0] | 1;
    pCHRBank[2] = (pRegister[1] & 0xFE);
    pCHRBank[3] = pRegister[1] | 1;
    pCHRBank[4] = pRegister[2];
    pCHRBank[5] = pRegister[3];
    pCHRBank[6] = pRegister[4];
    pCHRBank[7] = pRegister[5];
  }
}

void Mapper_4::DebugDump() {
  LOG(INFO) << "chrRam:" << std::boolalpha << usesCharacterRAM_
            << " chrRamSz:" << characterRAM_.size() << " 8kRom:" << rom_num_
            << " cReg:" << +targetRegister_ << " prgBankM:" << bPRGBankMode
            << " chrInv:" << bCHRInversion << " irqAct:" << bIRQActive
            << " irqEn:" << bIRQEnable << " irqUpd:" << bIRQUpdate
            << " irqCnt:" << +nIRQCounter << " irqRel:" << +nIRQReload
            << " regs:" << DumpVector(pRegister)
            << " prgBank:" << DumpVector(pPRGBank)
            << " chrBank:" << DumpVector(pCHRBank);
}
};  // namespace hn
