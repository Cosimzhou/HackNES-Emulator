#include "Cartridge.h"

#include <fstream>
#include <string>

#include "glog/logging.h"

//
//
//  +-----------------------------------------+
//  |0x00000     'NES\x1A'                    |
//  |0x00004     PRG-Bank Num                 |
//  |0x00005     CHR-Bank Num                 |
//  |0x00006:-4  Name Table                   |
//  |0x00006~7:8 Mapper                       |
//  |0x00010                                  |
//  |0x00010                                  |
//  |0x00010                                  |
//  |0x00010                                  |
//  |0x00010                                  |
//  +-----------------------------------------+
//  |0x00010     Prg Banks (16KB)             |
//  |0x.....     ......                       |
//  +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~--+
//  |0x00010     Chr Banks  (8KB)             |
//  |0x.....     ......                       |
//  +-----------------------------------------+
//
//
namespace hn {
Cartridge::Cartridge()
    : nameTableMirroring_(0), mapperNumber_(0), extendedRAM_(false) {}
const std::vector<Byte> &Cartridge::getROM() const { return PRG_ROM_; }

const std::vector<Byte> &Cartridge::getVROM() const { return CHR_ROM_; }

Byte Cartridge::getMapper() const { return mapperNumber_; }

Byte Cartridge::getNameTableMirroring() const { return nameTableMirroring_; }

bool Cartridge::hasExtendedRAM() const { return extendedRAM_; }

bool Cartridge::setBus(MainBus *bus) {
  bus_ = bus;

  if (bus_ == nullptr) {
    LOG(ERROR) << "Main bus set null";
    return false;
  }

  return true;
}

MainBus *Cartridge::bus() const { return bus_; }

bool Cartridge::loadFromFile(std::string path) {
  std::ifstream romFile(path, std::ios_base::binary | std::ios_base::in);
  if (!romFile) {
    LOG(ERROR) << "Could not open ROM file from path: " << path;
    return false;
  }

  LOG(INFO) << "Reading ROM from path: " << path;

  CartridgeHeader header;
  if (!romFile.read(reinterpret_cast<char *>(&header), 0x10)) {
    LOG(ERROR) << "Reading iNES header failed.";
    return false;
  }

  if (header.mark() != "NES\x1A") {
    LOG(ERROR) << "Not a valid iNES image. Magic number: " << std::hex
               << header[0] << " " << header[1] << " " << header[2] << " "
               << int(header[3]) << std::endl
               << "Valid magic number : N E S 1a";
    return false;
  }

  LOG(INFO) << "Reading header, it dictates: ";

  Byte banks = header.banks();
  LOG(INFO) << "16KB PRG-ROM Banks: " << +banks;
  if (!banks) {
    LOG(ERROR) << "ROM has no PRG-ROM banks. Loading ROM failed.";
    return false;
  }

  Byte vbanks = header.vbanks();
  LOG(INFO) << "8KB CHR-ROM Banks: " << +vbanks;

  nameTableMirroring_ = header.nameTableMirroring();
  LOG(INFO) << "Name Table Mirroring: " << +nameTableMirroring_;

  mapperNumber_ = header.mapperNumber();
  LOG(INFO) << "Mapper #: " << +mapperNumber_;

  extendedRAM_ = header.extendedRAM();
  LOG(INFO) << "Extended (CPU) RAM: " << std::boolalpha << extendedRAM_;

  if (header.ntsc()) {
    LOG(INFO) << "ROM is NTSC compatible.";
  } else {
    LOG(ERROR) << "PAL ROM not supported.";
    return false;
  }

  if (header.trainer()) {
    trainer_.resize(0x200);
    if (!romFile.read(reinterpret_cast<char *>(&trainer_[0]),
                      trainer_.size())) {
      LOG(ERROR) << "Reading trainer from image file failed.";
      return false;
    }
    LOG(INFO) << "Trainer is present.";
    LOG(ERROR) << "Trainer is not supported.";
    return false;
  }

  // PRG-ROM 16KB banks
  PRG_ROM_.resize(0x4000 * banks);
  if (!romFile.read(reinterpret_cast<char *>(&PRG_ROM_[0]), 0x4000 * banks)) {
    LOG(ERROR) << "Reading PRG-ROM from image file failed.";
    return false;
  }

  // CHR-ROM 8KB banks
  if (vbanks) {
    CHR_ROM_.resize(0x2000 * vbanks);
    if (!romFile.read(reinterpret_cast<char *>(&CHR_ROM_[0]),
                      0x2000 * vbanks)) {
      LOG(ERROR) << "Reading CHR-ROM from image file failed.";
      return false;
    }
  } else {
    LOG(INFO) << "Cartridge with CHR-RAM.";
  }

  LOG(INFO) << "PRG ROM size: " << (PRG_ROM_.size() >> 10)
            << "KB, CHR ROM size: " << (CHR_ROM_.size() >> 10) << "KB.";

  return true;
}

void Cartridge::DebugDump() {
  LOG(INFO) << "Cartridge: prgRom:" << (getROM().size() >> 10)
            << "KB chrRom:" << (getVROM().size() >> 10) << "KB. "
            << "Name Table Mirroring: " << +nameTableMirroring_
            << " Extended (CPU) RAM: " << std::boolalpha << extendedRAM_;
}

}  // namespace hn
