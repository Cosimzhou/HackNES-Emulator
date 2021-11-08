#pragma once

#include "common.h"

namespace hn {

struct CartridgeHeader {
  Byte bytes[16];

  Byte operator[](int subs) const { return bytes[subs]; }
  std::string mark() const { return std::string{&bytes[0], &bytes[4]}; }
  Byte banks() const { return bytes[4]; }
  Byte vbanks() const { return bytes[5]; }
  Byte nameTableMirroring() const { return bytes[6] & 0xB; }
  Byte mapperNumber() const {
    return ((bytes[6] >> 4) & 0xf) | (bytes[7] & 0xf0);
  }
  Byte extendedRAM() const { return bytes[6] & 0x2; }

  Byte mode() const {
    return ((bytes[0xA] & 0x3) == 0x2 || (bytes[0xA] & 0x1));
  }
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
  std::vector<Byte> PRG_ROM_;
  std::vector<Byte> CHR_ROM_;
  Byte nameTableMirroring_;
  Byte mapperNumber_;
  bool extendedRAM_;
  bool chrRAM_;

  MainBus *bus_;
};

};  // namespace hn
