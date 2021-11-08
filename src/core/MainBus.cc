#include "MainBus.h"

#include <iomanip>

#include "APU.h"
#include "glog/logging.h"

namespace hn {

//
//  +-----------------------------------------------+
//  |                                               |
//  | 0x00000                                       |
//  |    .                                          |
//  |    .              R A M                       |
//  |    .                                          |
//  | 0x01fff                                       |
//  +-----------------------------------------------+
//  | 0x02000                                       |
//  |    .                                          |
//  |    .              PPU Mem                     |
//  |    .                                          |
//  | 0x03fff                                       |
//  +-----------------------------------------------+
//  | 0x04000                                       |
//  |    .                                          |
//  |    .              APU REG(W)                  |
//  |    .                                          |
//  | 0x04007                                       |
//  | 0x04000                                       |
//  | 0x04000                                       |
//  | 0x04000                                       |
//  | 0x0400C           APU Noise                   |
//  | 0x0400D                                       |
//  | 0x0400E           APU Noise                   |
//  | 0x0400F           APU Noise                   |
//  | 0x04000                                       |
//  | 0x04000                                       |
//  | 0x04000                                       |
//  | 0x04014           OAMDMA                      |
//  | 0x04015           APU (R)                     |
//  | 0x04016           JOY 1                       |
//  | 0x04017           JOY 2(R)  APU(W)            |
//  | 0x04000                                       |
//  | 0x04020                                       |
//  |    .                                          |
//  |    .              PRG ROM                     |
//  |    .                                          |
//  | 0x05fff                                       |
//  +-----------------------------------------------+
//  | 0x06000                                       |
//  |    .                                          |
//  |    .              Ext RAM                     |
//  |    .                                          |
//  | 0x07fff                                       |
//  +-----------------------------------------------+
//  | 0x08000                                       |
//  |    .                                          |
//  |    .              PRG ROM                     |
//  |    .                                          |
//  |                                               |
//  +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
//
//
//
//
//

constexpr size_t kExtRAMSize = 0x2000;       // 8KB
constexpr size_t kRAMSize = 0x800;           // 2KB
constexpr size_t kRAMEndAddr = 0x2000;       // 8KB
constexpr size_t kPPMemEndAddr = 0x4000;     // 16KB
constexpr size_t kExtRAMStartAddr = 0x6000;  // 24KB
constexpr size_t kExtRAMEndAddr = 0x8000;    // 32KB

constexpr Address kRAMMask = kRAMEndAddr - 1;  // 2KB

MainBus::MainBus() : RAM_(kRAMSize, 0), mapper_(nullptr) {}

Byte MainBus::read(Address addr) {
  if (addr < kRAMEndAddr) {
    return RAM_[addr & kRAMMask];
  } else if (addr < 0x4020) {
    if (addr < kPPMemEndAddr) {
      // PPU registers, mirrored

      auto it = readCallbacks_.find(static_cast<IORegisters>(addr & 0x2007));
      if (it != readCallbacks_.end()) {
        return (it->second)();
        // Second object is the pointer to the function object
        // Dereference the function pointer and call it
      } else {
        VLOG(2) << "No read callback registered for I/O register at: "
                << std::hex << +addr;
      }
    } else if (0x4014 == addr || (0x4016 <= addr && addr < 0x4018)) {
      // Only *some* IO registers
      // OAMDMA & 2 JOYS

      auto it = readCallbacks_.find(static_cast<IORegisters>(addr));
      if (it != readCallbacks_.end()) {
        return (it->second)();
        // Second object is the pointer to the function object
        // Dereference the function pointer and call it
      } else {
        VLOG(2) << "No read callback registered for I/O register at: "
                << std::hex << +addr;
      }
    } else {
      return apu_->Read(addr);
      VLOG(2) << "Read access attempt at: " << std::hex << +addr;
    }
  } else if (addr < kExtRAMStartAddr) {
    VLOG(2) << "Expansion ROM read attempted. This is currently unsupported";
  } else if (addr < kExtRAMEndAddr) {
    if (mapper_->hasExtendedRAM()) {
      return extRAM_[addr - kExtRAMStartAddr];
    }
  } else {
    // Which addr is greater 0x8000.
    return mapper_->readPRG(addr);
  }

  return 0;
}

Address MainBus::readAddress(Address addr) {
  return read(addr) | read(addr + 1) << 8;
}

void MainBus::write(Address addr, Byte value) {
  if (addr < kRAMEndAddr) {
    RAM_[addr & kRAMMask] = value;
  } else if (addr < 0x4020) {
    if (addr < kPPMemEndAddr) {
      // PPU registers, mirrored
      auto it = writeCallbacks_.find(static_cast<IORegisters>(addr & 0x2007));
      if (it != writeCallbacks_.end()) {
        (it->second)(value);
        // Second object is the pointer to the function object
        // Dereference the function pointer and call it
      } else {
        VLOG(2) << "No write callback registered for I/O register at: "
                << std::hex << +addr;
      }
    } else if (0x4014 == addr || (0x4016 <= addr && addr < 0x4017)) {
      // only some registers
      // OAMDMA & 2 JOYS

      auto it = writeCallbacks_.find(static_cast<IORegisters>(addr));
      if (it != writeCallbacks_.end()) {
        (it->second)(value);
        // Second object is the pointer to the function object
        // Dereference the function pointer and call it
      } else {
        VLOG(2) << "No write callback registered for I/O register at: "
                << std::hex << +addr;
      }
    } else {
      apu_->Write(addr, value);
      // VLOG(2) << "Write access attmept at: " << std::hex << +addr;
    }
  } else if (addr < kExtRAMStartAddr) {
    VLOG(2) << "Expansion ROM access attempted. This is currently unsupported";
  } else if (addr < kExtRAMEndAddr) {
    if (mapper_->hasExtendedRAM()) {
      extRAM_[addr - kExtRAMStartAddr] = value;
    }
  } else {
    // Which addr is greater 0x8000.
    mapper_->writePRG(addr, value);
  }
}

const Byte *MainBus::getPagePtr(Byte page) {
  Address addr = page << 8;
  if (addr < kRAMEndAddr) {
    return &RAM_[addr & kRAMMask];
  } else if (addr < 0x4020) {
    LOG(ERROR) << "Register address memory pointer access attempt";
  } else if (addr < kExtRAMStartAddr) {
    LOG(ERROR) << "Expansion ROM access attempted, which is unsupported";
  } else if (addr < kExtRAMEndAddr) {
    if (mapper_->hasExtendedRAM()) {
      return &extRAM_[addr - kExtRAMStartAddr];
    }
    LOG(ERROR) << "Expansion ROM access attempted, which is unsupported";
  } else {
    // Get page pointer from cartridge
    LOG(ERROR) << "Unknown address " << std::hex << addr
               << " memory pointer access attempt";
  }

  return nullptr;
}

bool MainBus::setAPU(APU *apu) {
  apu_ = apu;

  if (!apu) {
    LOG(ERROR) << "APU pointer is nullptr";
    return false;
  }

  return true;
}

CPU *MainBus::cpu() const { return cpu_; }
bool MainBus::setCPU(CPU *cpu) {
  cpu_ = cpu;

  if (!cpu) {
    LOG(ERROR) << "CPU pointer is nullptr";
    return false;
  }

  return true;
}

PPU *MainBus::ppu() const { return ppu_; }
bool MainBus::setPPU(PPU *ppu) {
  ppu_ = ppu;

  if (!ppu) {
    LOG(ERROR) << "CPU pointer is nullptr";
    return false;
  }

  return true;
}

bool MainBus::setMapper(Mapper *mapper) {
  mapper_ = mapper;

  if (!mapper) {
    LOG(ERROR) << "Mapper pointer is nullptr";
    return false;
  }

  if (mapper->hasExtendedRAM()) {
    extRAM_.resize(kExtRAMSize);
  }

  return true;
}

bool MainBus::setWriteCallback(IORegisters reg,
                               std::function<void(Byte)> callback) {
  if (!callback) {
    LOG(ERROR) << "callback argument is nullptr";
    return false;
  }
  return writeCallbacks_.emplace(reg, callback).second;
}

bool MainBus::setReadCallback(IORegisters reg,
                              std::function<Byte(void)> callback) {
  if (!callback) {
    LOG(ERROR) << "callback argument is nullptr";
    return false;
  }
  return readCallbacks_.emplace(reg, callback).second;
}

std::string MainBus::getPageContent(Address page) {
  std::stringstream ss;
  Address addr = page << 8;
  ss << std::hex << std::setfill('0');

  for (int i = 0; i < 16; i++) {
    ss << "0x00" << std::setw(2) << addr << "  ";
    for (int j = 0; j < 16; j++) {
      if (j == 8) ss << ' ';
      ss << std::setw(2) << +read(addr++) << ' ';
    }

    ss << std::endl;
  }

  return ss.str();
}

void MainBus::DebugDump() { LOG(INFO) << "ZeroPage:\n" << getPageContent(0); }

};  // namespace hn
