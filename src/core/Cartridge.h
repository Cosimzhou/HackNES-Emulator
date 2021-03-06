#pragma once

#include "common.h"

namespace hn {

// Byte     Contents
//---------------------------------------------------------------------------
// 0-3      String "NES^Z" used to recognize .NES files.
// 4        Number of 16kB ROM banks.
// 5        Number of 8kB VROM banks.
// 6        bit 0     1 for vertical mirroring, 0 for horizontal mirroring.
//          bit 1     1 for battery-backed RAM at $6000-$7FFF.
//          bit 2     1 for a 512-byte trainer at $7000-$71FF.
//          bit 3     1 for a four-screen VRAM layout.
//          bit 4-7   Four lower bits of ROM Mapper Type.
// 7        bit 0     1 for VS-System cartridges.
//          bit 1-3   Reserved, must be zeroes!
//             bit 2,3 0x0c == 0x08 it is a NES 2.0
//          bit 4-7   Four higher bits of ROM Mapper Type.
// 8        Number of 8kB RAM banks. For compatibility with the previous
//          versions of the .NES format, assume 1x8kB RAM page when this
//          byte is zero.
// 9        bit 0     1 for PAL cartridges, otherwise assume NTSC.
//          bit 1-7   Reserved, must be zeroes!
// 10-15    Reserved, must be zeroes!
//
// 16-...   ROM banks, in ascending order. If a trainer is present, its
//          512 bytes precede the ROM bank contents.
//...-EOF   VROM banks, in ascending order.
//---------------------------------------------------------------------------
struct CartridgeHeader {
  Byte bytes[16];

  Byte operator[](int subs) const { return bytes[subs]; }
  std::string mark() const { return std::string(&bytes[0], &bytes[4]); }
  Byte banks() const { return bytes[4]; }
  Byte vbanks() const { return bytes[5]; }
  Byte nameTableMirroring() const { return bytes[6] & 0xB; }
  Word mapperNumber() const {
    Word type = ((bytes[6] >> 4) & 0xf) | (bytes[7] & 0xf0);
    if (isNES2_0()) {
      type |= static_cast<Word>(bytes[8] & 0x0f) << 8;
    }
    return type;
  }
  Byte subMapperNumber() const {
    if (isNES2_0()) {
      return (bytes[8] >> 4) & 0xf;
    }

    return 0;
  }

  Byte consoleType() const {
    // 0: Nintendo Entertainment System/Family Computer
    // 1: Nintendo Vs. System
    // 2: Nintendo Playchoice 10
    // 3: Extended Console Type
    return 0x3 & bytes[7];
  }
  bool isNES2_0() const { return (bytes[7] & 0xc) == 0x8; }

  Byte extendedRAM() const { return bytes[6] & 0x2; }
  bool trainer() const { return bytes[6] & 0x4; }
  // bool ntsc() const { return (bytes[9] & 0x1); }
  bool ntsc() const { return (bytes[10] & 0x3) == 0; }

  // PAL or NTSC
  // Byte mode() const {
  //  return ((bytes[0xA] & 0x3) == 0x2 || (bytes[0xA] & 0x1));
  //}
};

class MainBus;
class Cartridge {
 public:
  Cartridge();

  bool loadFromFile(std::string path);
  const std::vector<Byte> &getROM() const;
  const std::vector<Byte> &getVROM() const;
  Byte getMapper() const;
  Byte getNameTableMirroring() const;
  bool hasExtendedRAM() const;

  void setNameTableMirroring(Byte mirror) { nameTableMirroring_ = mirror; }

  void DebugDump();

  bool setBus(MainBus *bus);
  MainBus *bus() const;

  std::string nes_path() const { return nesPath_; }
  const CartridgeHeader &header() const { return header_; }

 private:
  CartridgeHeader header_;

  Memory trainer_;
  Memory PRG_ROM_;
  Memory CHR_ROM_;
  Byte nameTableMirroring_;
  Byte mapperNumber_;
  bool extendedRAM_;
  bool chrRAM_;

  MainBus *bus_;

  std::string nesPath_;
};

};  // namespace hn
