#include "Mapper.h"

#include "Mapper_0.h"
#include "Mapper_1.h"
#include "Mapper_15.h"
#include "Mapper_2.h"
#include "Mapper_202.h"
#include "Mapper_3.h"
#include "Mapper_4.h"
#include "Mapper_65.h"
#include "Mapper_66.h"
#include "Mapper_7.h"
#include "Mapper_76.h"

#include "../core/CPU.h"
#include "../core/MainBus.h"
#include "../core/PPU.h"

namespace hn {
NameTableMirroring Mapper::getNameTableMirroring() {
  return static_cast<NameTableMirroring>(cartridge_.getNameTableMirroring());
}

std::unique_ptr<Mapper> Mapper::createMapper(Byte mapper_t, Cartridge &cart) {
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
    case 15:
      ret.reset(new Mapper_15(cart));
      break;
    case 65:
      ret.reset(new Mapper_65(cart));
      break;
    case 66:
      ret.reset(new Mapper_66(cart));
      break;
    case 76:
      ret.reset(new Mapper_76(cart));
      break;
    case 202:
      ret.reset(new Mapper_202(cart));
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

void Mapper::FireIRQ() { cartridge_.bus()->cpu()->TryIRQ(); }
void Mapper::StopIRQ() { cartridge_.bus()->cpu()->ClearIRQ(); }

}  // namespace hn
