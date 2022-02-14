#include "Mapper_76.h"

#include "glog/logging.h"

//  Registers:
//  ---------------------------
//  Mask: $E001
//
//    $8000:  [.... .AAA]
//      A = Address for use with $8001
//
//    $8001:  [..DD DDDD]    Data port:
//        R:2 ->  CHR reg 0  (2k @ $0000)
//        R:3 ->  CHR reg 1  (2k @ $0800)
//        R:4 ->  CHR reg 2  (2k @ $1000)
//        R:5 ->  CHR reg 3  (2k @ $1800)
//        R:6 ->  PRG reg 0  (8k @ $8000)
//        R:7 ->  PRG reg 1  (8k @ $a000)
//
//  CHR Setup:
//  ---------------------------
//       $0000   $0400   $0800   $0C00   $1000   $1400   $1800   $1C00
//      +---------------+---------------+---------------+---------------+
//      |      R:2      |      R:3      |      R:4      |      R:5      |
//      +---------------+---------------+---------------+---------------+
//
//  PRG Setup:
//  ---------------------------
//       $8000   $A000   $C000   $E000
//      +-------+-------+-------+-------+
//      |  R:6  |  R:7  | { -2} | { -1} |
//      +-------+-------+-------+-------+
//
namespace hn {

Mapper_76::Mapper_76(Cartridge &cart) : Mapper(cart, 76) {}

void Mapper_76::Reset() {
  prgRom_ = cartridge_.getROM().size() >> 13;

  selReg_ = 0;
  regs_.resize(8);
  regs_[2] = 0;
  regs_[3] = 1;
  regs_[4] = 2;
  regs_[5] = 3;
  regs_[6] = 0;
  regs_[7] = 1;
}

void Mapper_76::writePRG(Address addr, Byte data) {
  switch (addr & 0xe001) {
    case 0x8000:
      selReg_ = data & 7;
      break;
    case 0x8001:
      regs_[selReg_] = data & 0x3f;
      break;
    default:
      LOG(ERROR) << "writePRG @" << std::hex << addr << " is not supported";
      break;
  }
}

Byte Mapper_76::readPRG(Address addr) {
  FileAddress prg_addr = 0;

  if (addr >= 0xc000) {  // 0x2000
    prg_addr = prgRom_ - (addr >= 0xe000 ? 1 : 2);
    prg_addr <<= 13;
    prg_addr |= addr & 0x1fff;
  } else if (addr >= 0x8000) {  // 0x2000
    prg_addr = regs_[6 + ((addr >> 13) & 1)];
    prg_addr <<= 13;  // 8KB
    prg_addr |= addr & 0x1FFF;
  } else {
    LOG(ERROR) << "readPRG @" << std::hex << addr;
    return 0;
  }

  return cartridge_.getROM()[prg_addr];
}

Byte Mapper_76::readCHR(Address addr) {
  FileAddress vaddr;
  vaddr = regs_[2 + ((addr >> 11) & 3)];
  vaddr <<= 11;
  vaddr |= addr & 0x7ff;
  return cartridge_.getVROM()[vaddr];
}

void Mapper_76::writeCHR(Address addr, Byte value) {
  LOG(ERROR) << "writeCHR @" << std::hex << addr << " is not supported";
}

void Mapper_76::DebugDump() {}

};  // namespace hn
