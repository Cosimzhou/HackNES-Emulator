#pragma once

#include <vector>
#include "../mapper/Mapper.h"
#include "Cartridge.h"

namespace hn {
class PictureBus {
 public:
  PictureBus();
  Byte read(Address addr);
  void write(Address addr, Byte value);

  bool setMapper(Mapper *mapper);
  Byte readPalette(Byte paletteAddr);

  void updateMirroring();

 private:
  std::vector<Byte> RAM_;
  std::size_t NameTable0, NameTable1, NameTable2,
      NameTable3;  // indices where they start in RAM vector

  std::vector<Byte> palette_;

  Mapper *mapper_;
};

}  // namespace hn
