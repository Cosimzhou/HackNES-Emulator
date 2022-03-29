#include "Mapper_2.h"
#include "glog/logging.h"

namespace hn {
Mapper_2::Mapper_2(Cartridge &cart) : Mapper(cart, 2) {}

void Mapper_2::Reset() {
  selectPRG_ = 0;
  if (cartridge_.getVROM().empty()) {
    usesCharacterRAM_ = true;
    ResetVRam();
    LOG(INFO) << "Uses character RAM";
  } else {
    usesCharacterRAM_ = false;
  }

  lastBankPtr_ = &cartridge_.getROM()[cartridge_.getROM().size() - 0x4000];
}

Byte Mapper_2::readPRG(Address addr) {
  if (addr < 0xc000) {
    return cartridge_.getROM()[((addr - 0x8000) & 0x3fff) | (selectPRG_ << 14)];
  } else {
    return *(lastBankPtr_ + (addr & 0x3fff));
  }
}

void Mapper_2::writePRG(Address addr, Byte value) { selectPRG_ = value; }

Byte Mapper_2::readCHR(Address addr) {
  if (usesCharacterRAM_) {
    return vRam_[addr];
  } else {
    return cartridge_.getVROM()[addr];
  }
}

void Mapper_2::writeCHR(Address addr, Byte value) {
  if (usesCharacterRAM_) {
    vRam_[addr] = value;
  } else {
    LOG(INFO) << "Read-only CHR memory write attempt at " << std::hex << addr;
  }
}
void Mapper_2::Save(std::ostream &os) {
  Mapper::Save(os);

  Write(os, usesCharacterRAM_);
  // const Byte *lastBankPtr_;
  Write(os, selectPRG_);
}

void Mapper_2::Restore(std::istream &is) {
  Mapper::Restore(is);

  Read(is, usesCharacterRAM_);
  // const Byte *lastBankPtr_;
  Read(is, selectPRG_);

  lastBankPtr_ = &cartridge_.getROM()[cartridge_.getROM().size() - 0x4000];
}

}  // namespace hn
