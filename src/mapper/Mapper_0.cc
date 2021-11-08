#include "Mapper_0.h"
#include "glog/logging.h"

namespace hn {
Mapper_0::Mapper_0(Cartridge &cart) : Mapper(cart, 0) {}
void Mapper_0::Reset() {
  if (cartridge_.getROM().size() == 0x4000) {  // 1 bank
    oneBank_ = true;
  } else {  // 2 banks
    oneBank_ = false;
  }

  usesCharacterRAM_ = cartridge_.getVROM().size() == 0;

  if (usesCharacterRAM_) {
    characterRAM_.resize(0x2000);
    LOG(INFO) << "Uses character RAM";
  }
}

Byte Mapper_0::readPRG(Address addr) {
  if (!oneBank_) {
    return cartridge_.getROM()[addr - 0x8000];
  } else {  // mirrored
    return cartridge_.getROM()[(addr - 0x8000) & 0x3fff];
  }
}

void Mapper_0::writePRG(Address addr, Byte value) {
  VLOG(2) << "ROM memory write attempt at " << +addr << " to set " << +value;
}

Byte Mapper_0::readCHR(Address addr) {
  if (usesCharacterRAM_) {
    return characterRAM_[addr];
  } else {
    return cartridge_.getVROM()[addr];
  }
}

void Mapper_0::writeCHR(Address addr, Byte value) {
  if (usesCharacterRAM_) {
    characterRAM_[addr] = value;
  } else {
    LOG(ERROR) << "Read-only CHR memory write attempt at " << std::hex << addr;
    // DDREPORT();
  }
}

void Mapper_0::DebugDump() {
  LOG(INFO) << "Mapper: 0 onbank:" << oneBank_
            << " chrRam:" << usesCharacterRAM_;
}

}  // namespace hn
