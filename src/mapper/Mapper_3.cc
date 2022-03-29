#include "Mapper_3.h"
#include "glog/logging.h"

namespace hn {
Mapper_3::Mapper_3(Cartridge &cart) : Mapper(cart, 3) {}

void Mapper_3::Reset() {
  selectCHR_ = 0;
  if (cartridge_.getROM().size() == 0x4000) {  // 1 bank
    oneBank_ = true;
  } else {  // 2 banks
    oneBank_ = false;
  }
}

Byte Mapper_3::readPRG(Address addr) {
  if (!oneBank_) {
    return cartridge_.getROM()[addr - 0x8000];
  } else {  // mirrored
    return cartridge_.getROM()[(addr - 0x8000) & 0x3fff];
  }
}

void Mapper_3::writePRG(Address addr, Byte value) { selectCHR_ = value & 0x3; }

Byte Mapper_3::readCHR(Address addr) {
  // selectCHR_ * 0x2000 + addr
  return cartridge_.getVROM()[addr | (selectCHR_ << 13)];
}

void Mapper_3::writeCHR(Address addr, Byte value) {
  LOG(INFO) << "Read-only CHR memory write attempt at " << std::hex << addr;
}

void Mapper_3::Save(std::ostream &os) {
  Mapper::Save(os);

  Write(os, oneBank_);
  Write(os, selectCHR_);
}

void Mapper_3::Restore(std::istream &is) {
  Mapper::Restore(is);

  Read(is, oneBank_);
  Read(is, selectCHR_);
}

}  // namespace hn
