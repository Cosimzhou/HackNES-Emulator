#include "Emulator.h"

#include <chrono>
#include <fstream>
#include <thread>

#include "glog/logging.h"
#include "utils.h"

namespace hn {
Emulator::Emulator()
    : cpu_(bus_),
      ppu_(bus_, pictureBus_),
      apu_(bus_),
      goldfinger_(bus_),
      screenScale_(2.f),
      cycleTimer_(),
      cpuCycleDuration_(std::chrono::nanoseconds(560)) {}

void Emulator::Reset() {
  frameIdx_ = 0;

  mapper_->Reset();
  cpu_.Reset();
  ppu_.Reset();
  apu_.Reset();
  cycleTimer_ = std::chrono::high_resolution_clock::now();
  elapsedTime_ = cycleTimer_ - cycleTimer_;
}

bool Emulator::HardwareSetup() {
  ppu_.SetScreen(emulatorScreen_.get());
  apu_.SetSpeaker(emulatorSpeaker_.get());

  if (!bus_.setReadCallback(PPUSTATUS,
                            [&](void) { return ppu_.getStatus(); }) ||
      !bus_.setReadCallback(PPUDATA, [&](void) { return ppu_.getData(); }) ||
      !bus_.setReadCallback(JOY1, [&](void) { return ReadJoypad(0); }) ||
      !bus_.setReadCallback(JOY2, [&](void) { return ReadJoypad(1); }) ||
      !bus_.setReadCallback(OAMDATA, [&](void) { return ppu_.getOAMData(); })) {
    LOG(ERROR) << "Critical error: Failed to set I/O callbacks";
    return false;
  }

  if (!bus_.setWriteCallback(PPUCTRL, [&](Byte b) { ppu_.control(b); }) ||
      !bus_.setWriteCallback(PPUMASK, [&](Byte b) { ppu_.setMask(b); }) ||
      !bus_.setWriteCallback(OAMADDR, [&](Byte b) { ppu_.setOAMAddress(b); }) ||
      !bus_.setWriteCallback(PPUADDR,
                             [&](Byte b) { ppu_.setDataAddress(b); }) ||
      !bus_.setWriteCallback(PPUSCROL, [&](Byte b) { ppu_.setScroll(b); }) ||
      !bus_.setWriteCallback(PPUDATA, [&](Byte b) { ppu_.setData(b); }) ||
      !bus_.setWriteCallback(OAMDMA, [&](Byte b) { DMA(b); }) ||
      !bus_.setWriteCallback(JOY1,
                             [&](Byte b) {
                               emulatorJoypads_[0]->strobe(b);
                               emulatorJoypads_[1]->strobe(b);
                             }) ||
      !bus_.setWriteCallback(OAMDATA, [&](Byte b) { ppu_.setOAMData(b); })) {
    LOG(ERROR) << "Critical error: Failed to set I/O callbacks";
    return false;
  }

  record_.setFinishCallback([&]() {
    workMode_ = REPLAY;
    emulatorScreen_->setTip("Start replay");
  });

  mapper_ = Mapper::createMapper(cartridge_.getMapper(), cartridge_);
  if (!mapper_) {
    LOG(ERROR) << "Creating Mapper failed. Probably unsupported.";
    return false;
  }

  if (!bus_.setMapper(mapper_.get()) || !pictureBus_.setMapper(mapper_.get())) {
    return false;
  }

  bus_.setAPU(&apu_);
  bus_.setCPU(&cpu_);
  bus_.setPPU(&ppu_);
  cartridge_.setBus(&bus_);

  return true;
}

void Emulator::RunTick(bool running) {
  if (running) {
    elapsedTime_ += std::chrono::high_resolution_clock::now() - cycleTimer_;
    cycleTimer_ = std::chrono::high_resolution_clock::now();

    while (elapsedTime_ > cpuCycleDuration_) {
      XPUTick();

      elapsedTime_ -= cpuCycleDuration_;
    }

    if (frameIdx_ < ppu_.frameIndex()) {
      FrameRefresh();
      frameIdx_ = ppu_.frameIndex();
    }
  } else {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 60));
  }
}

void Emulator::XPUTick() {
  DDTRY();
  // PPU
  ppu_.Step();
  ppu_.Step();
  ppu_.Step();
  // CPU
  cpu_.Step();
  // APU
  apu_.Step();

  // clock tick
  bus_.Tick();
  DDCATCH();

  goldfinger_.Patrol();
}

void Emulator::DMA(Byte page) {
  cpu_.skipDMACycles();
  ppu_.doDMA(bus_.getPagePtr(page));
}

Byte Emulator::ReadJoypad(int no) {
  Byte ret;
  if (workMode_ == REPLAY) {
    return record_.Read(no, cpu_.clock_cycles());
  } else {
    ret = emulatorJoypads_[no]->read();

    if (workMode_ == RECORDING) {
      if (ret & 0xbf) {
        record_.Record(no, cpu_.clock_cycles(), ret);
      }
    }
  }

  return ret;
}

void Emulator::ToggleWorkMode() {
  switch (workMode_) {
    case PLAYING:
      workMode_ = RECORDING;
      HintText("Start recording");
      break;
    case RECORDING:
      workMode_ = REPLAY;
      HintText("Start replay");
      record_.Save();
      Reset();
      break;
    default:
      break;
  }
}

void Emulator::HintText(const std::string &text) {
  emulatorScreen_->setTip(text);
}
void Emulator::Pause() {}
void Emulator::Resume() {}
void Emulator::OnPause() { emulatorSpeaker_->Stop(); }
void Emulator::OnResume() { emulatorSpeaker_->Play(); }
void Emulator::setVideoHeight(int height) {
  screenScale_ = height / float(NESVideoHeight);
  LOG(INFO) << "Scale: " << screenScale_ << " set. Screen: "
            << static_cast<int>(NESVideoWidth * screenScale_) << "x"
            << static_cast<int>(NESVideoHeight * screenScale_);
}

void Emulator::setVideoWidth(int width) {
  screenScale_ = width / float(NESVideoWidth);
  LOG(INFO) << "Scale: " << screenScale_ << " set. Screen: "
            << static_cast<int>(NESVideoWidth * screenScale_) << "x"
            << static_cast<int>(NESVideoHeight * screenScale_);
}

void Emulator::setVideoScale(float scale) {
  screenScale_ = scale;
  LOG(INFO) << "Scale: " << screenScale_ << " set. Screen: "
            << static_cast<int>(NESVideoWidth * screenScale_) << "x"
            << static_cast<int>(NESVideoHeight * screenScale_);
}

void Emulator::setKeys(const JoypadInputConfig &p1,
                       const JoypadInputConfig &p2) {
  emulatorJoypads_[0]->setKeyBindings(p1);
  emulatorJoypads_[1]->setKeyBindings(p2);
}

bool Emulator::LoadCartridge(const std::string &rom_path) {
  if (!cartridge_.loadFromFile(rom_path)) return false;

  goldfinger_.LoadFile(rom_path + ".cht");

  return true;
}

void Emulator::SetCartridge(const Cartridge &cartridge) {
  cartridge_ = cartridge;
}

void Emulator::RestoreRecord() {
  if (record_file_.empty()) {
    return;
  }

  std::ifstream is(record_file_);
  if (is.good()) {
    Restore(is);
  }
}

void Emulator::SaveRecord() {
  record_file_ = Helper::NewFileName(record_file_);

  std::ofstream file(record_file_);
  Save(file);
  file.close();
}

void Emulator::DebugDump() {
  LOG(INFO) << "\n=======================\nDebugDump\n=======================";
  cpu_.DebugDump();
  ppu_.DebugDump();
  apu_.DebugDump();
  bus_.DebugDump();
  mapper_->DebugDump();
  cartridge_.DebugDump();
}

void Emulator::Save(std::ostream &os) {
  WriteNum(os, 1);

  Write(os, frameIdx_);

  bus_.Save(os);
  pictureBus_.Save(os);
  cpu_.Save(os);
  ppu_.Save(os);
  apu_.Save(os);
  // Cartridge cartridge_;
  mapper_->Save(os);

  // GoldFinger goldfinger_;

  // TimePoint cycleTimer_;
  // std::vector<std::map<size_t, Byte>> joypad_record_;
  // std::chrono::high_resolution_clock::duration elapsedTime_;
  // std::chrono::nanoseconds cpuCycleDuration_;
}

void Emulator::Restore(std::istream &is) {
  uint32_t version = ReadNum(is);
  if (version != 1) {
    LOG(ERROR) << "Unsupport recording file version";
    return;
  }

  Read(is, frameIdx_);

  bus_.Restore(is);
  pictureBus_.Restore(is);
  cpu_.Restore(is);
  ppu_.Restore(is);
  apu_.Restore(is);
  // Cartridge cartridge_;
  mapper_->Restore(is);
}

}  // namespace hn
