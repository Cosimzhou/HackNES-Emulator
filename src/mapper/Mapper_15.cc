#include "Mapper_15.h"

#include "glog/logging.h"

namespace hn {

Mapper_15::Mapper_15(Cartridge &cart) : Mapper(cart, 15) {}

void Mapper_15::Reset() {
  bankAddr_.resize(4);
  bankAddr_[0] = 0;
  bankAddr_[1] = 1;
  bankAddr_[2] = 2;
  bankAddr_[3] = 3;
  prgBankMode_ = 0;

  vRam_.resize(0x2000);
  std::fill(vRam_.begin(), vRam_.end(), 0);
  chrVRam_ = true;

  ChangeNTMirroring(Vertical);
}

void Mapper_15::writePRG(Address addr, Byte data) {
  prgBankMode_ = addr & 3;
  Byte swap = (data & 0x80) >> 7;
  FileAddress taddr = data & 0x3f;

  switch (prgBankMode_) {
    case 0:
      bankAddr_[0] = (taddr << 1) + swap;
      bankAddr_[1] = (taddr << 1) + (1 ^ swap);
      bankAddr_[2] = ((taddr + 1) << 1) + swap;
      bankAddr_[3] = ((taddr + 1) << 1) + (1 ^ swap);
      break;
    case 1:
      chrVRam_ = true;
    case 3:
      bankAddr_[2] = (taddr << 1) + swap;
      bankAddr_[3] = (taddr << 1) + (1 ^ swap);
      break;
    case 2:
      chrVRam_ = true;
      bankAddr_[0] = (taddr << 1) + swap;
      bankAddr_[1] = (taddr << 1) + swap;
      bankAddr_[2] = (taddr << 1) + swap;
      bankAddr_[3] = (taddr << 1) + swap;
      break;
  }

  ChangeNTMirroring((data & 0x40) ? Horizontal : Vertical);
}

Byte Mapper_15::readPRG(Address addr) {  //
  FileAddress prg_addr = 0;
  if (addr >= 0x8000) {  // 0x2000
    prg_addr = bankAddr_[(addr >> 13) & 0x3] << 13;
    prg_addr += addr & 0x1FFF;
  } else {
    LOG(ERROR) << "readPRG @" << std::hex << addr;
    return 0;
  }

  return cartridge_.getROM()[prg_addr];
}

Byte Mapper_15::readCHR(Address addr) {
  FileAddress vaddr = addr & 0x1fff;
  if (chrVRam_) {
    return vRam_[vaddr];
  } else if (cartridge_.getVROM().empty()) {
    LOG(ERROR) << "No chr rom allowed to read";
  } else {
    return cartridge_.getVROM()[vaddr];
  }

  return 0;
}

void Mapper_15::writeCHR(Address addr, Byte value) {
  if (chrVRam_) {
    vRam_[addr & 0x1fff] = value;
  } else {
    LOG(ERROR) << "No chr rom allowed to write " << std::hex << addr << " "
               << +value;
  }
}

void Mapper_15::DebugDump() {
  LOG(INFO) << "Mapper 15 mode:" << +prgBankMode_ << " bankAddr:" << std::hex
            << DumpVector(bankAddr_);
}
};  // namespace hn
