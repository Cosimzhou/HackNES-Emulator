#include "Mapper_65.h"

#include "glog/logging.h"

//
// iKnown Games:
// --------------------------
// Daiku no Gen San 2
// Kaiketsu Yanchamaru 3
// Spartan X 2
//
// Registers:
// --------------------------
//
//   $8000:  PRG Reg 0 (8k @ $8000 -or- 8k @ $C000)
//   $A000:  PRG Reg 1 (8k @ $A000)
//
//   $B000-$B007:  CHR regs
//
//   $9000:  [X... ....]  PRG bank layout
//     Very similar to VRC4 or MMC3
//     %0 = bank at $8000 set by writes to $8000, bank at $C000 always $3E
//     %1 = bank at $C000 set by writes to $8000, bank at $8000 always $3E
//
//     bank at $E000 always $3F
//
//   $9001:  [MM.. ....]  Mirroring
//     %00 = Vert
//     %10 = Horz
//     %01,%11 = 1scA
//
//   $9003:  [E... ....]  IRQ Enable (0=disabled, 1=enabled)
//   $9004:  [.... ....]  Reload IRQ counter
//   $9005:  [IIII IIII]  High 8 bits of IRQ Reload value
//   $9006:  [IIII IIII]  Low 8 bits of IRQ Reload value
//
//
// On Powerup:
// ---------------------------
// On powerup, it appears as though PRG regs are inited to specific values:
//
//   $8000 = $00
//   $A000 = $01
//
// Games do rely on this and will crash otherwise.
//
// CHR Setup:
// ---------------------------
//
//       $0000   $0400   $0800   $0C00   $1000   $1400   $1800   $1C00
//     +-------+-------+-------+-------+-------+-------+-------+-------+
//     | $B000 | $B001 | $B002 | $B003 | $B004 | $B005 | $B006 | $B007 |
//     +-------+-------+-------+-------+-------+-------+-------+-------+
//
//
//
// IRQs:
// ---------------------------
//
// This mapper's IRQ system is very simple.  There's a 16-bit internal down
// counter which (when enabled), decrements by 1 every CPU cycle.  When the
// counter reaches 0, an IRQ is fired.  The counter stops at 0 -- it does not
// wrap and isn't automatically reloaded.
//
// Any write to $9003 or $9004 will acknowledge the pending IRQ.
//
// Any write to $9004 will copy the 16-bit reload value into the counter.
//
// $9006 and $9005 set the reload value, but do not have any effect on the
// actual counter.  Note that $9005 is the HIGH bits, not the low bits.
//

namespace hn {

Mapper_65::Mapper_65(Cartridge &cart) : Mapper(cart, 65) {}

void Mapper_65::Reset() {
  bIRQEnable_ = false;
  nIRQCounter_ = nIRQReload_ = 0;
  chrRegs_.resize(8);
  for (int i = 0; i < 8; i++) {
    chrRegs_[i] = i;
  }

  auto nrom = cartridge_.getROM().size() >> 13;
  prgRegs_.resize(4);
  prgRegs_[0] = 0;
  prgRegs_[1] = 1;
  prgRegs_[2] = nrom - 2;
  prgRegs_[3] = nrom - 1;

  prgRegs_[2] = 0x3e;
  prgRegs_[3] = 0x3f;
  prgSwap_ = false;

  ChangeNTMirroring(Vertical);
}

void Mapper_65::writePRG(Address addr, Byte data) {
  switch (addr) {
    case 0x8000:
      prgRegs_[0] = data;
      break;
    case 0x9000:
      prgSwap_ = data & 0x80;
      break;
    case 0x9001: {
      auto type = (data & 0xc0) >> 6;
      if (type & 1) {
        ChangeNTMirroring(OneScreenLower);
      } else
        ChangeNTMirroring(type & 2 ? Horizontal : Vertical);
    } break;
    case 0x9003:
      bIRQEnable_ = data & 0x80;
      break;
    case 0x9004:
      nIRQCounter_ = nIRQReload_;
      break;
    case 0x9005:
      CLR_BIT(nIRQReload_, 0xff00);
      SET_BIT(nIRQReload_, (static_cast<std::uint16_t>(data) << 8));
      break;
    case 0x9006:
      SWAP_BIT(nIRQReload_, data, 0xff);
      break;
    case 0xa000:
      prgRegs_[1] = data;
      break;
    case 0xb000:
    case 0xb001:
    case 0xb002:
    case 0xb003:
    case 0xb004:
    case 0xb005:
    case 0xb006:
    case 0xb007:
      chrRegs_[addr - 0xb000] = data;
      break;
    default:
      //
      break;
  }
}

Byte Mapper_65::readPRG(Address addr) {
  FileAddress prg_addr = 0;
  auto bank = (addr >> 13) & 3;
  if (bank & 1) {
    prg_addr = prgRegs_[bank];
  } else {
    if (prgSwap_) bank ^= 2;
    prg_addr = prgRegs_[bank];
  }
  prg_addr <<= 13;

  return cartridge_.getROM()[prg_addr];
}

Byte Mapper_65::readCHR(Address addr) {
  FileAddress vaddr = chrRegs_[(addr >> 10) & 7];
  vaddr <<= 10;
  vaddr |= addr & 0x3ff;
  return cartridge_.getVROM()[vaddr];
}

void Mapper_65::writeCHR(Address addr, Byte value) {
  LOG(ERROR) << "writeCHR @" << std::hex << addr << " is not supported";
}

void Mapper_65::Tick() {
  if (bIRQEnable_ && nIRQCounter_ > 0) {
    if (--nIRQCounter_ == 0) {
      FireIRQ();
    }
  }
}

void Mapper_65::DebugDump() {}

};  // namespace hn
