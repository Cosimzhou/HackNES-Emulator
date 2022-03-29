#pragma once
#include <chrono>
#include <map>
#include <memory>

#include "APU.h"
#include "CPU.h"
#include "GoldFinger.h"
#include "MainBus.h"
#include "OpRecord.h"
#include "PPU.h"
#include "PeripheralDevices.h"
#include "PictureBus.h"
#include "common.h"

namespace hn {

const int NESVideoWidth = ScanlineVisibleDots;
const int NESVideoHeight = VisibleScanlines;

class Emulator : public Serialize {
 public:
  Emulator();

  virtual void run() = 0;
  virtual void FrameRefresh() = 0;

  void setVideoWidth(int width);
  void setVideoHeight(int height);
  void setVideoScale(float scale);
  void setKeys(const JoypadInputConfig &p1, const JoypadInputConfig &p2);

  enum { PLAYING, RECORDING, RECORDED, REPLAY } workMode_;

  bool LoadCartridge(const std::string &rom_path);
  void setCartridge(const Cartridge &cartridge);

  void SetRecordMode(bool recording, const std::string &record_file);

  void Pause();
  void Resume();
  void Reset();
  void HintText(const std::string &text);

  virtual void Save(std::ostream &os) override;
  virtual void Restore(std::istream &is) override;

 protected:
  void DebugDump();

  bool HardwareSetup();
  void RunTick(bool running);
  void RestoreRecord();
  void SaveRecord();

  void OnPause();
  void OnResume();
  void ToggleWorkMode();

  OperatingRecord record_;

  std::unique_ptr<VirtualScreen> emulatorScreen_;
  std::unique_ptr<VirtualSpeaker> emulatorSpeaker_;
  std::unique_ptr<VirtualJoypad> emulatorJoypads_[2];

  float screenScale_;
  TimePoint cycleTimer_;

  GoldFinger goldfinger_;
  Cartridge cartridge_;

  std::unique_ptr<Mapper> mapper_;

 private:
  void DMA(Byte page);
  void XPUTick();
  Byte ReadJoypad(int no);

  MainBus bus_;
  PictureBus pictureBus_;
  CPU cpu_;
  PPU ppu_;
  APU apu_;

  size_t frameIdx_;
  std::string record_file_;
  std::chrono::high_resolution_clock::duration elapsedTime_;
  std::chrono::nanoseconds cpuCycleDuration_;
};

}  // namespace hn
