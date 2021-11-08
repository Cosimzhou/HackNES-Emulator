#include "Mapper.h"

#include "Mapper_0.h"
#include "Mapper_1.h"
#include "Mapper_2.h"
#include "Mapper_3.h"
#include "Mapper_4.h"
#include "Mapper_66.h"
#include "Mapper_7.h"

#include "../core/MainBus.h"
#include "../core/PPU.h"

namespace hn {
NameTableMirroring Mapper::getNameTableMirroring() {
  return static_cast<NameTableMirroring>(cartridge_.getNameTableMirroring());
}

std::unique_ptr<Mapper> Mapper::createMapper(Mapper::Type mapper_t,
                                             Cartridge &cart) {
  std::unique_ptr<Mapper> ret(nullptr);
  switch (mapper_t) {
    case 0:
      ret.reset(new Mapper_0(cart));
      break;
    case 1:
      ret.reset(new Mapper_1(cart));
      break;
    case 2:
      ret.reset(new Mapper_2(cart));
      break;
    case 3:
      ret.reset(new Mapper_3(cart));
      break;
    case 4:
      ret.reset(new Mapper_4(cart));
      break;
    case 7:
      ret.reset(new Mapper_7(cart));
      break;
    case 66:
      ret.reset(new Mapper_66(cart));
      break;
    default:
      break;
  }
  return ret;
}

void Mapper::ChangeNTMirroring(NameTableMirroring mirror) {
  cartridge_.setNameTableMirroring(mirror);
  cartridge_.bus()->ppu()->bus().updateMirroring();
}

}  // namespace hn
