#pragma once

#include <functional>
#include <memory>

#include "../core/Cartridge.h"

namespace hn {
enum NameTableMirroring {
  Horizontal = 0,
  Vertical = 1,
  FourScreen = 8,
  OneScreenLower,
  OneScreenHigher,
};

// Reference:
//
//  https://wiki.nesdev.org/w/index.php?title=Mapper
//
class Mapper {
 public:
  enum Type {
    NROM = 0,
    SxROM = 1,
    UxROM = 2,
    CNROM = 3,
    MMC3 = 4,
    AxROM = 7,
    HundredInOne = 15,
    GxROM = 66,
  };

  Mapper(Cartridge &cart, Byte t) : cartridge_(cart), type_(t){};

  virtual void Reset() = 0;
  virtual void writePRG(Address addr, Byte value) = 0;
  virtual Byte readPRG(Address addr) = 0;

  virtual Byte readCHR(Address addr) = 0;
  virtual void writeCHR(Address addr, Byte value) = 0;

  virtual NameTableMirroring getNameTableMirroring();

  bool inline hasExtendedRAM() { return cartridge_.hasExtendedRAM(); }

  static std::unique_ptr<Mapper> createMapper(Type mapper_t, Cartridge &cart);

  virtual std::string mapper_name() const = 0;
  virtual void DebugDump() {}

  Cartridge &cartridge() { return cartridge_; }

  virtual void Hsync(int scanline) {}

 protected:
  void ChangeNTMirroring(NameTableMirroring mirror);
  void FireIRQ();
  void StopIRQ();

  Cartridge &cartridge_;
  Byte type_;
};
}  // namespace hn
