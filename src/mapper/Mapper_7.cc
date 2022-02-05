#include "Mapper_7.h"

#include "glog/logging.h"

namespace hn {

Mapper_7::Mapper_7(Cartridge &cart) : Mapper(cart, 7) {}

void Mapper_7::Reset() {
  prgBank_ = 0;
  prgRom_ = cartridge_.getROM().size() >> 15;
  chrVRam_ = cartridge_.getVROM().empty();

  LOG(INFO) << "chrvram: " << chrVRam_ << " prg:" << +prgRom_;
  if (chrVRam_) {
    vRam_.resize(0x2000);
  }

  ChangeNTMirroring(OneScreenLower);
}

void Mapper_7::writePRG(Address addr, Byte data) {
  prgBank_ = (data & 7) % prgRom_;

  ChangeNTMirroring((data & 0x10) ? OneScreenHigher : OneScreenLower);
}

Byte Mapper_7::readPRG(Address addr) {  //
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

Byte Mapper_7::readCHR(Address addr) {
  FileAddress vaddr = addr & 0x1fff;
  if (chrVRam_) {
    return vRam_[vaddr];
  } else {
    return cartridge_.getVROM()[vaddr];
  }

  return 0;
}

void Mapper_7::writeCHR(Address addr, Byte value) {
  if (chrVRam_) {
    vRam_[addr & 0x1fff] = value;
  }

  // VLOG(1) << "writeCHR " << std::hex << addr << " " << value;
  //
}

void Mapper_7::DebugDump() {}
};  // namespace hn
