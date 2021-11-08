#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include "../mapper/Mapper.h"
#include "Cartridge.h"

namespace hn {
enum IORegisters {
  PPUCTRL = 0x2000,
  PPUMASK,
  PPUSTATUS,
  OAMADDR,
  OAMDATA,
  PPUSCROL,
  PPUADDR,
  PPUDATA,
  // 0x4000+
  OAMDMA = 0x4014,
  APUCTRL = 0x4015,
  JOY1 = 0x4016,
  JOY2 = 0x4017,
};

class APU;
class CPU;
class PPU;
class MainBus {
 public:
  MainBus();
  Byte read(Address addr);
  Address readAddress(Address addr);
  void write(Address addr, Byte value);
  bool setMapper(Mapper *mapper);
  bool setAPU(APU *apu);
  bool setCPU(CPU *cpu);
  bool setPPU(PPU *ppu);
  bool setWriteCallback(IORegisters reg, std::function<void(Byte)> callback);
  bool setReadCallback(IORegisters reg, std::function<Byte(void)> callback);
  const Byte *getPagePtr(Byte page);

  CPU *cpu() const;
  PPU *ppu() const;

  void DebugDump();
  std::string getPageContent(Address page);

 private:
  std::vector<Byte> RAM_;
  std::vector<Byte> extRAM_;
  Mapper *mapper_;
  APU *apu_;
  CPU *cpu_;
  PPU *ppu_;

  std::unordered_map<IORegisters, std::function<void(Byte)>> writeCallbacks_;
  std::unordered_map<IORegisters, std::function<Byte(void)>> readCallbacks_;
};

}  // namespace hn
