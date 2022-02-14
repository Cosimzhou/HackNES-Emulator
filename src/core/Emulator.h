#pragma once
#include <SFML/Graphics.hpp>
#include <chrono>

#include "APU.h"
#include "CPU.h"
#include "Controller.h"
#include "GoldFinger.h"
#include "MainBus.h"
#include "OpRecord.h"
#include "PPU.h"
#include "PictureBus.h"
#include "VirtualScreen.h"
#include "VirtualSpeaker.h"

namespace hn {
using TimePoint = std::chrono::high_resolution_clock::time_point;

const int NESVideoWidth = ScanlineVisibleDots;
const int NESVideoHeight = VisibleScanlines;

class Emulator {
 public:
  Emulator();
  void run();
  void setVideoWidth(int width);
  void setVideoHeight(int height);
  void setVideoScale(float scale);
  void setKeys(const ControllerInputConfig &p1,
               const ControllerInputConfig &p2);

  enum { PLAYING, RECORDING, RECORDED, REPLAY } workMode_;

  bool LoadCartridge(const std::string &rom_path);
  void SetCartridge(const Cartridge &cartridge);

  OperatingRecord record_;

  void Pause();
  void Resume();

 private:
  void DMA(Byte page);
  void XPUTick();
  void Reset();
  Byte ReadJoypad(int no);

  void DebugDump();

  VirtualScreenSfml emulatorScreen_;
  VirtualSpeakerSfml emulatorSpeaker_;

  MainBus bus_;
  PictureBus pictureBus_;
  CPU cpu_;
  PPU ppu_;
  APU apu_;
  Cartridge cartridge_;
  std::unique_ptr<Mapper> mapper_;

  Controller controllers_[2];

  GoldFinger goldfinger_;

  sf::RenderWindow window_;
  float screenScale_;

  TimePoint cycleTimer_;
  std::vector<std::map<size_t, Byte>> joypad_record_;
  std::chrono::high_resolution_clock::duration elapsedTime_;
  std::chrono::nanoseconds cpuCycleDuration_;
};

}  // namespace hn
