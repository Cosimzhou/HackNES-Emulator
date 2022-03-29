#pragma once

#include <vector>
#include "../mapper/Mapper.h"
#include "Cartridge.h"

namespace hn {
class PictureBus : public Serialize {
 public:
  PictureBus();
  Byte read(Address addr);
  void write(Address addr, Byte value);

  bool setMapper(Mapper* mapper);
  Byte readPalette(Byte paletteAddr);

  void updateMirroring();

  virtual void Save(std::ostream& os) override;
  virtual void Restore(std::istream& is) override;

 private:
  Memory RAM_;
  std::vector<size_t> NameTable_;  // indices where they start in RAM vector

  std::vector<Byte> palette_;

  Mapper* mapper_;
};

}  // namespace hn
