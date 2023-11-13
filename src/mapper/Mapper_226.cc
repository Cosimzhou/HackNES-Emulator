#include "Mapper_226.h"

#include "glog/logging.h"

// Here are Disch's original notes:
// ========================
// =  Mapper 226          =
// ========================
//
// Example Games:
// --------------------------
//  76-in-1
//  Super 42-in-1
//
//
//  Registers:
// ---------------------------
//
// Range, Mask:  $8000-FFFF, $8001
//
//  $8000:  [PMOP PPPP]
//     P = Low 6 bits of PRG Reg
//     M = Mirroring (0=Horz, 1=Vert)
//     O = PRG Mode
//
//  $8001:  [.... ...H]
//     H = high bit of PRG
//
//
// PRG Setup:
// ---------------------------
//
// Low 6 bits of the PRG Reg come from $8000, high bit comes from $8001
//
//
//               $8000   $A000   $C000   $E000
//              +-------------------------------+
// PRG Mode 0:  |             <Reg>             |
//              +-------------------------------+
// PRG Mode 1:  |      Reg      |      Reg      |
//              +---------------+---------------+
//
namespace hn {

Mapper_226::Mapper_226(Cartridge &cart) : Mapper(cart, 226) {}

void Mapper_226::Reset() {
  prgBank_ = 0;
  writeProtected_ = false;
  pageSizeBit_ = 15;

  // prgRom_ = cartridge_.getROM().size() >> 15;
  chrVRam_ = cartridge_.getVROM().empty();

  // LOG(INFO) << "chrvram: " << chrVRam_ << " prg:" << +prgRom_;
  if (chrVRam_) {
    ResetVRam();
  }

  ChangeNTMirroring(Horizontal);
}

void Mapper_226::writePRG(Address addr, Byte data) {
  static Address oaddr = 0;
  static Byte odata = 0;
  if (oaddr != addr || odata != data) {
    LOG(INFO) << "writePRG @ 0x" << std::hex << addr << " = " << +data;
    oaddr = addr;
    odata = data;
  }

  if (addr & 1) {
    // Reg 1
    writeProtected_ = data & 0x2;

    data <<= 5;
    SWAP_BIT(prgBank_, data, 0x20);
  } else {
    // Reg 0
    ChangeNTMirroring((data & 0x40) ? Vertical : Horizontal);
    pageSizeBit_ = (data & 0x20) ? 14 : 15;
    swap_ = data & 1;

    data >>= 1;
    SWAP_BIT(prgBank_, data, 0xf);
    data >>= 2;
    SWAP_BIT(prgBank_, data, 0x10);
  }
}

Byte Mapper_226::readPRG(Address addr) {
  FileAddress prg_addr = 0;
  if (addr >= 0x8000) {  // 0x2000
    addr &= 0x7fff;
    if (pageSizeBit_ == 15) {
      prg_addr = prgBank_;
      prg_addr <<= 15;
      prg_addr += addr;
    } else {
      if (swap_) addr ^= 0x4000;
      prg_addr = prgBank_;
      prg_addr <<= 15;
      prg_addr += addr;
    }
  } else {
    LOG(ERROR) << "readPRG @" << std::hex << addr;
    return 0;
  }

  return cartridge_.getROM()[prg_addr];
}

Byte Mapper_226::readCHR(Address addr) {
  FileAddress vaddr = addr & 0x1fff;
  if (chrVRam_) {
    return vRam_[vaddr];
  } else {
    return cartridge_.getVROM()[vaddr];
  }

  return 0;
}

void Mapper_226::writeCHR(Address addr, Byte value) {
  if (!writeProtected_ && addr < 0x2000) {
    vRam_[addr & 0x1fff] = value;
  }
}

void Mapper_226::DebugDump() {
  LOG(INFO) << "bank:" << std::hex << +prgBank_ << " page:" << +pageSizeBit_
            << " swap:" << swap_ << "";
}

};  // namespace hn
