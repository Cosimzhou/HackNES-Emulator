#include "Mapper_202.h"

#include "glog/logging.h"

namespace hn {

Mapper_202::Mapper_202(Cartridge &cart) : Mapper(cart, 202) {}

void Mapper_202::Reset() {
  b32KMode_ = false;
  firstWrite_ = true;
  prgBank_ = 0;

  ChangeNTMirroring(Vertical);
}

void Mapper_202::writePRG(Address addr, Byte data) {
  LOG(INFO) << "write prg " << std::hex << addr << " " << +data;
  if (firstWrite_) {
    b32KMode_ = TEST_BITS(addr, 9);
  } else {
    prgBank_ = (addr >> 1) & 7;
    ChangeNTMirroring((addr & 0x1) ? Horizontal : Vertical);
  }
  firstWrite_ = !firstWrite_;
}

Byte Mapper_202::readPRG(Address addr) {  //
  FileAddress vaddr = prgBank_;

  if (b32KMode_) {
    vaddr <<= 15;  // 32KB
    vaddr |= addr & 0x7fff;
  } else {
    vaddr <<= 14;  // 16KB
    vaddr |= addr & 0x3fff;
  }

  return cartridge_.getROM()[vaddr];
}

Byte Mapper_202::readCHR(Address addr) {
  FileAddress vaddr = prgBank_;
  vaddr <<= 13;
  vaddr |= addr & 0x1fff;
  return cartridge_.getVROM()[vaddr];
}

void Mapper_202::writeCHR(Address addr, Byte value) {
  LOG(INFO) << "writeCHR " << std::hex << addr << " val:" << +value;
}

void Mapper_202::DebugDump() {
  LOG(INFO) << "Mapper 202 b32:" << b32KMode_ << " bank:" << +prgBank_
            << " 1stW:" << firstWrite_;
}
};  // namespace hn
