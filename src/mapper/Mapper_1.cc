#include "Mapper_1.h"

#include "glog/logging.h"

namespace hn {
constexpr size_t kPRGPageSize = 0x4000;

Mapper_1::Mapper_1(Cartridge &cart)
    : Mapper(cart, 1),
      modeCHR_(0),
      modePRG_(3),
      tempRegister_(0),
      writeCounter_(0),
      regPRG_(0),
      regCHR0_(0),
      regCHR1_(0),
      firstBankPRG_(nullptr),
      secondBankPRG_(nullptr),
      firstBankCHR_(nullptr),
      secondBankCHR_(nullptr) {
  if (cart.getVROM().empty()) {
    usesCharacterRAM_ = true;
    characterRAM_.resize(0x2000);
    LOG(INFO) << "Uses character RAM";
  } else {
    LOG(INFO) << "Using CHR-ROM";
    usesCharacterRAM_ = false;
    firstBankCHR_ = &cart.getVROM()[0];
    secondBankCHR_ = &cart.getVROM()[0x1000 * regCHR1_];
  }

  firstBankPRG_ = &cart.getROM()[0];  // first bank
  secondBankPRG_ = &cart.getROM()[cart.getROM().size() - kPRGPageSize];
  /*0x2000 * 0x0e*/  // last bank
}
void Mapper_1::Reset() {
  modeCHR_ = 0;
  modePRG_ = 3;
  tempRegister_ = 0;
  writeCounter_ = 0;
  regPRG_ = 0;
  regCHR0_ = 0;
  regCHR1_ = 0;
  firstBankPRG_ = nullptr;
  secondBankPRG_ = nullptr;
  firstBankCHR_ = nullptr;
  secondBankCHR_ = nullptr;

  if (cartridge_.getVROM().empty()) {
    usesCharacterRAM_ = true;
    characterRAM_.resize(0x2000);
    LOG(INFO) << "Uses character RAM";
  } else {
    LOG(INFO) << "Using CHR-ROM";
    usesCharacterRAM_ = false;
    firstBankCHR_ = &cartridge_.getVROM()[0];
    secondBankCHR_ = &cartridge_.getVROM()[0x1000 * regCHR1_];
  }

  firstBankPRG_ = &cartridge_.getROM()[0];  // first bank
  secondBankPRG_ =
      &cartridge_.getROM()[cartridge_.getROM().size() - kPRGPageSize];
  /*0x2000 * 0x0e*/  // last bank
}

Byte Mapper_1::readPRG(Address addr) {
  if (addr < 0xc000) {
    return *(firstBankPRG_ + (addr & 0x3fff));
  } else {
    return *(secondBankPRG_ + (addr & 0x3fff));
  }
}

void Mapper_1::writePRG(Address addr, Byte value) {
  if (TEST_BITS(value, 0x80)) {  // if reset bit is set
                                 // reset reg
    tempRegister_ = 0;
    writeCounter_ = 0;
    modePRG_ = 3;
    calculatePRGPointers();
  } else {
    tempRegister_ = (tempRegister_ >> 1) | ((value & 1) << 4);
    ++writeCounter_;

    if (writeCounter_ == 5) {
      if (addr <= 0x9fff) {
        NameTableMirroring mirroring;
        switch (tempRegister_ & 0x3) {
          case 0:
            mirroring = OneScreenLower;
            break;
          case 1:
            mirroring = OneScreenHigher;
            break;
          case 2:
            mirroring = Vertical;
            break;
          case 3:
            mirroring = Horizontal;
            break;
        }
        ChangeNTMirroring(mirroring);

        modeCHR_ = (tempRegister_ & 0x10) >> 4;
        modePRG_ = (tempRegister_ & 0xc) >> 2;
        calculatePRGPointers();

        // Recalculate CHR pointers
        if (modeCHR_ == 0) {  // one 8KB bank
          firstBankCHR_ =
              &cartridge_
                   .getVROM()[0x1000 * (regCHR0_ | 1)];  // ignore last bit
          secondBankCHR_ = firstBankCHR_ + 0x1000;
        } else {  // two 4KB banks
          firstBankCHR_ = &cartridge_.getVROM()[0x1000 * regCHR0_];
          secondBankCHR_ = &cartridge_.getVROM()[0x1000 * regCHR1_];
        }
      } else if (addr <= 0xbfff) {  // CHR Reg 0
        regCHR0_ = tempRegister_;
        firstBankCHR_ =
            &cartridge_
                 .getVROM()[0x1000 * (tempRegister_ |
                                      (1 - modeCHR_))];  // OR 1 if 8KB mode
        if (modeCHR_ == 0) secondBankCHR_ = firstBankCHR_ + 0x1000;
      } else if (addr <= 0xdfff) {
        regCHR1_ = tempRegister_;
        if (modeCHR_ == 1)
          secondBankCHR_ = &cartridge_.getVROM()[0x1000 * tempRegister_];
      } else {
        // TODO PRG-RAM
        if ((tempRegister_ & 0x10) == 0x10) {
          LOG(INFO) << "PRG-RAM activated";
        }

        tempRegister_ &= 0xf;
        regPRG_ = tempRegister_;
        calculatePRGPointers();
      }

      tempRegister_ = 0;
      writeCounter_ = 0;
    }
  }
}

void Mapper_1::calculatePRGPointers() {
  if (modePRG_ <= 1) {  // 32KB changeable
    // equivalent to multiplying 0x8000 * (regPRG_ >> 1)
    firstBankPRG_ = &cartridge_.getROM()[kPRGPageSize * (regPRG_ & ~1)];
    secondBankPRG_ = firstBankPRG_ + kPRGPageSize;  // add 16KB
  } else if (modePRG_ == 2) {                       // fix first switch second
    firstBankPRG_ = &cartridge_.getROM()[0];
    secondBankPRG_ = firstBankPRG_ + kPRGPageSize * regPRG_;
  } else {  // switch first fix second
    firstBankPRG_ = &cartridge_.getROM()[kPRGPageSize * regPRG_];
    secondBankPRG_ =
        &cartridge_.getROM()[cartridge_.getROM().size() - kPRGPageSize];
    /*0x2000 * 0x0e*/
  }
}

Byte Mapper_1::readCHR(Address addr) {
  if (usesCharacterRAM_) {
    return characterRAM_[addr];
  } else if (addr < 0x1000) {
    return *(firstBankCHR_ + addr);
  } else {
    return *(secondBankCHR_ + (addr & 0xfff));
  }
}

void Mapper_1::writeCHR(Address addr, Byte value) {
  if (usesCharacterRAM_) {
    characterRAM_[addr] = value;
  } else {
    LOG(INFO) << "Read-only CHR memory write attempt at " << std::hex << addr;
  }
}
}  // namespace hn
