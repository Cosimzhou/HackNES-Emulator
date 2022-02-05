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
//          bit 4-7   Four higher bits of ROM Mapper Type.
// 8        Number of 8kB RAM banks. For compatibility with the previous
//          versions of the .NES format, assume 1x8kB RAM page when this
//          byte is zero.
// 9        bit 0     1 for PAL cartridges, otherwise assume NTSC.
//          bit 1-7   Reserved, must be zeroes!
// 10-15    Reserved, must be zeroes!
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
  Byte mapperNumber() const {
    return ((bytes[6] >> 4) & 0xf) | (bytes[7] & 0xf0);
  }
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

 private:
  std::vector<Byte> trainer_;
  std::vector<Byte> PRG_ROM_;
  std::vector<Byte> CHR_ROM_;
  Byte nameTableMirroring_;
  Byte mapperNumber_;
  bool extendedRAM_;
  bool chrRAM_;

  MainBus *bus_;
};

};  // namespace hn
