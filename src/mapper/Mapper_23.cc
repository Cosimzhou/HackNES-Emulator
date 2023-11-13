#include "Mapper_23.h"

#include "glog/logging.h"

namespace hn {

Mapper_23::Mapper_23(Cartridge &cart) : Mapper(cart, 23) {}

void Mapper_23::Reset() {
  is_vrc2_ = false;
  chrBanks_.resize(8);
  chrBanks_[0] = 0;
  chrBanks_[1] = 1;

  prgBanks_.resize(4);
  prgBanks_[3] = (cartridge_.getROM().size() >> 13) - 1;
  if (is_vrc2_) {
    prgBanks_[2] = prgBanks_[3] - 1;
  }

  nIRQLatch_ = 0;
  nIRQControl_ = 0;
  nIRQAcknowledge_ = 0;
  // prgRom_ = cartridge_.getROM().size() >> 15;
  // chrVRam_ = cartridge_.getVROM().empty();

  // LOG(INFO) << "chrvram: " << chrVRam_ << " prg:" << +prgRom_;
  // if (chrVRam_) {
  //  vRam_.resize(0x2000);
  //}

  ChangeNTMirroring(OneScreenLower);
}

void Mapper_23::writePRG(Address addr, Byte data) {
  static NameTableMirroring mirrors[] = {Vertical, Horizontal, OneScreenLower,
                                         OneScreenHigher};
  // prgBanks_ = (data & 7) % prgRom_;

  FileAddress faddr;
  if (addr < 0x8000) {
  } else if (addr <= 0x8fff) {
    // 8000 ~ 8003
    prgBanks_[!is_vrc2_ && swap_ ? 2 : 0] = data & 0x1f;
  } else if (addr <= 0x9fff) {
    // 9000 ~ 9004
    Byte kind = addr & 3;
    if (is_vrc2_) {
      ChangeNTMirroring(mirrors[data & 0x01]);
    } else {
      switch (kind) {
        case 0:
          ChangeNTMirroring(mirrors[data & 0x03]);
          break;
        case 1:
          break;
        case 2:
          swap_ = data & 2;
          wram_ = data & 1;
          break;
        case 3:
          // send some low signal
          break;
      }
    }
  } else if (addr <= 0xafff) {
    // a000 ~ a003
    prgBanks_[1] = data & 0x1f;
  } else if (addr <= 0xbfff) {
    // CHR Bank Select
    Byte k = ((addr >> 12) & 0x7) - 3, d = addr & 3;
    k <<= 1;
    if (d & 2) k++;

    if (d & 1) {
      faddr = data & 0x1f;
      faddr <<= 4;
      SWAP_BIT(chrBanks_[k], faddr, 0x1f0);
    } else {
      SWAP_BIT(chrBanks_[k], data, 0x0f);
    }
  } else if (addr < 0xf004) {
    Byte kind = addr & 3;
    switch (kind) {
      case 0:
        // $F000:  IRQ Latch, low 4 bits
        SWAP_BIT(nIRQLatch_, data, 0xf);
        break;
      case 1:
        // $F001:  IRQ Latch, high 4 bits
        data <<= 4;
        SWAP_BIT(nIRQLatch_, data, 0xf0);
        break;
      case 2:
        // $F002:  IRQ Control
        nIRQControl_ = data;
        break;
      case 3:
        // $F003:  IRQ Acknowledge
        nIRQAcknowledge_ = data;
        break;
    }
  }
}

Byte Mapper_23::readPRG(Address addr) {
  FileAddress prg_addr = 0;
  if (addr >= 0x8000) {  // 0x2000
    prg_addr = prgBanks_[(addr >> 13) - 4];
    prg_addr <<= 13;  // 8KB
    prg_addr += addr & 0x1FFF;
  } else {
    LOG(ERROR) << "readPRG @" << std::hex << addr;
    return 0;
  }

  return cartridge_.getROM()[prg_addr];
}

Byte Mapper_23::readCHR(Address addr) {
  Byte bank = (addr >> 10) & 7;
  FileAddress vaddr = chrBanks_[bank];
  vaddr <<= 10;
  vaddr += addr & 0x3ff;

  return cartridge_.getVROM()[vaddr];
}

void Mapper_23::writeCHR(Address addr, Byte value) {
  LOG(ERROR) << "writeChr $" << std::hex << addr << " = " << +value;
  // if (chrVRam_) {
  //  vRam_[addr & 0x1fff] = value;
  //}
}

void Mapper_23::DebugDump() {}

};  // namespace hn
