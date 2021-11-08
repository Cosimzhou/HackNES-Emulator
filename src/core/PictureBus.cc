#include "PictureBus.h"
#include "glog/logging.h"

namespace hn {

//
//  +-----------------------------------------------+
//  |                                               |
//  | 0x00000                                       |
//  |    :                 Pattern Table 0          |
//  |    :        CHR ROM  ------------- 0x01000    |
//  |    :                 Pattern Table 1          |
//  | 0x01fff                                       |
//  +- - - - - - - - - - - - - - - - - - - - - - - -+
//  | 0x02000     Tables in RAM                     |
//  |    :                 Name Table 0 (0x3c0)     |
//  |    :        Table 0  ----------- 0x23c0       |
//  |    :                 Attribute Table 0 (0x40) |
//  | 0x02400                                       |
//  |    :                 Name Table 1             |
//  |    :        Table 1  ----------- 0x27c0       |
//  |    :                 Attribute Table 1        |
//  | 0x02800                                       |
//  |    :                 Name Table 2             |
//  |    :        Table 2  ----------- 0x2bc0       |
//  |    :                 Attribute Table 2        |
//  | 0x02c00                                       |
//  |    :                 Name Table 3             |
//  |    :        Table 3  ----------- 0x2fc0       |
//  |    :                 Attribute Table 3        |
//  | 0x03000                                       |
//  |    :                                          |
//  |    :        Mirrors (0x2000-0x2eff)x1         |
//  |    :                                          |
//  | 0x03eff                                       |
//  +- - - - - - - - - - - - - - - - - - - - - - - -+
//  | 0x03f00     Palettes (256B)                   |
//  |    :                Image Palette (16B)       |
//  |    :        Palette -------------- 0x3f10     |
//  |    :                Spirite Palette (16B)     |
//  | 0x03f20                                       |
//  |    :        Mirrors (0x3f00-0x3f1f)x7         |
//  | 0x03fff                                       |
//  +- - - - - - - - - - - - - - - - - - - - - - - -+
//  | 0x04000                                       |
//  |    :                                          |
//  |    :        Mirrors (0x0000-0x3fff)x3         |
//  |    :                                          |
//  | 0x0ffff                                       |
//  +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
//
PictureBus::PictureBus() : RAM_(0x800), palette_(0x20), mapper_(nullptr) {}

Byte PictureBus::read(Address addr) {
  addr &= 0x3fff;
  if (addr < 0x2000) {
    return mapper_->readCHR(addr);
  } else if (addr < 0x3f00) {
    // Name tables upto 0x3000, then mirrored upto 3eff
    auto index = addr & 0x3ff;
    if (addr < 0x2400) {  // NT0
      return RAM_[NameTable0 + index];
    } else if (addr < 0x2800) {  // NT1
      return RAM_[NameTable1 + index];
    } else if (addr < 0x2c00) {  // NT2
      return RAM_[NameTable2 + index];
    } else {  // NT3
      return RAM_[NameTable3 + index];
    }
  } else if (addr < 0x4000) {
    return palette_[addr & 0x1f];
  } else {
    LOG(ERROR) << "read PPM: 0x" << std::hex << addr;
  }

  return 0;
}

Byte PictureBus::readPalette(Byte paletteAddr) { return palette_[paletteAddr]; }

void PictureBus::write(Address addr, Byte value) {
  addr &= 0x3fff;
  if (addr < 0x2000) {
    mapper_->writeCHR(addr, value);
  } else if (addr < 0x3f00) {
    // Name tables upto 0x3000, then mirrored upto 3eff
    auto index = addr & 0x3ff;
    if (addr < 0x2400) {  // NT0
      RAM_[NameTable0 + index] = value;
    } else if (addr < 0x2800) {  // NT1
      RAM_[NameTable1 + index] = value;
    } else if (addr < 0x2c00) {  // NT2
      RAM_[NameTable2 + index] = value;
    } else {  // NT3
      RAM_[NameTable3 + index] = value;
    }
  } else if (addr < 0x4000) {
    addr &= 0x1f;
    if (addr == 0x10) {
      // if (addr & 0x3 == 0) {
      //  palette_[0] = palette_[4] = palette_[8] = palette_[12] = palette_[16]
      //  = palette_[20] = palette_[24] = palette_[28] = value;
      //} else {
      addr = 0;
    }
    palette_[addr] = value;
  } else {
    LOG(ERROR) << "Write PPM: 0x" << std::hex << addr;
  }
}

//
// Name Tables used for background
//
// +-----------------+-----------------+
// |                 |                 |
// |  Name Table 2   |  Name Table 3   |
// |    (0x2800)     |    (0x2c00)     |
// |                 |                 |
// +-----------------+-----------------+
// |                 |                 |
// |  Name Table 0   |  Name Table 1   |
// |    (0x2000)     |    (0x2400)     |
// |                 |                 |
// +-----------------+-----------------+
void PictureBus::updateMirroring() {
  switch (mapper_->getNameTableMirroring()) {
    case Horizontal:
      NameTable0 = NameTable1 = 0;
      NameTable2 = NameTable3 = 0x400;
      VLOG(2) << "Horizontal Name Table mirroring set. (Vertical Scrolling)";
      break;
    case Vertical:
      NameTable0 = NameTable2 = 0;
      NameTable1 = NameTable3 = 0x400;
      VLOG(2) << "Vertical Name Table mirroring set. (Horizontal Scrolling)";
      break;
    case OneScreenLower:
      NameTable0 = NameTable1 = NameTable2 = NameTable3 = 0;
      VLOG(2) << "Single Screen mirroring set with lower bank.";
      break;
    case OneScreenHigher:
      NameTable0 = NameTable1 = NameTable2 = NameTable3 = 0x400;
      VLOG(2) << "Single Screen mirroring set with higher bank.";
      break;
    default:
      NameTable0 = NameTable1 = NameTable2 = NameTable3 = 0;
      LOG(ERROR) << "Unsupported Name Table mirroring : "
                 << mapper_->getNameTableMirroring();
  }
}

bool PictureBus::setMapper(Mapper *mapper) {
  if (!mapper) {
    LOG(ERROR) << "Mapper argument is nullptr";
    return false;
  }

  mapper_ = mapper;
  updateMirroring();
  return true;
}

}  // namespace hn
