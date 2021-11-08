#include "Emulator.h"
#include "PatternViewer.h"
#include "glog/logging.h"

#include <chrono>
#include <thread>

namespace hn {
Emulator::Emulator()
    : emulatorSpeaker_(),
      cpu_(bus_),
      ppu_(pictureBus_, emulatorScreen_),
      apu_(bus_, cpu_, emulatorSpeaker_),
      goldfinger_(bus_),
      screenScale_(2.f),
      cycleTimer_(),
      cpuCycleDuration_(std::chrono::nanoseconds(560)) {
  if (!bus_.setReadCallback(PPUSTATUS,
                            [&](void) { return ppu_.getStatus(); }) ||
      !bus_.setReadCallback(PPUDATA, [&](void) { return ppu_.getData(); }) ||
      !bus_.setReadCallback(JOY1, [&](void) { return ReadJoypad(0); }) ||
      !bus_.setReadCallback(JOY2, [&](void) { return ReadJoypad(1); }) ||
      !bus_.setReadCallback(OAMDATA, [&](void) { return ppu_.getOAMData(); })) {
    LOG(ERROR) << "Critical error: Failed to set I/O callbacks";
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
                               controllers_[0].strobe(b);
                               controllers_[1].strobe(b);
                             }) ||
      !bus_.setWriteCallback(OAMDATA, [&](Byte b) { ppu_.setOAMData(b); })) {
    LOG(ERROR) << "Critical error: Failed to set I/O callbacks";
  }

  ppu_.setInterruptCallback([&]() { cpu_.interrupt(CPU::NMI); });
}

void Emulator::run() {
  mapper_ = Mapper::createMapper(
      static_cast<Mapper::Type>(cartridge_.getMapper()), cartridge_);
  if (!mapper_) {
    LOG(ERROR) << "Creating Mapper failed. Probably unsupported.";
    return;
  }

  if (!bus_.setMapper(mapper_.get()) || !pictureBus_.setMapper(mapper_.get()))
    return;

  bus_.setAPU(&apu_);
  bus_.setCPU(&cpu_);
  bus_.setPPU(&ppu_);

  cartridge_.setBus(&bus_);

  window_.create(sf::VideoMode(NESVideoWidth * screenScale_,
                               NESVideoHeight * screenScale_),
                 "Hack NES", sf::Style::Titlebar | sf::Style::Close);
  window_.setVerticalSyncEnabled(true);
  emulatorScreen_.create(NESVideoWidth, NESVideoHeight, screenScale_,
                         sf::Color::White);

  Reset();
  sf::Event event;
  bool focus = true, pause = false;
  while (window_.isOpen()) {
    while (window_.pollEvent(event)) {
      if (event.type == sf::Event::Closed ||
          (event.type == sf::Event::KeyPressed &&
           event.key.code == sf::Keyboard::Escape)) {
        window_.close();
        return;
      } else if (event.type == sf::Event::GainedFocus) {
        focus = true;
        cycleTimer_ = std::chrono::high_resolution_clock::now();
        emulatorSpeaker_.play();
      } else if (event.type == sf::Event::LostFocus) {
        focus = false;
        emulatorSpeaker_.stop();
      } else if (event.type == sf::Event::KeyPressed &&
                 event.key.code == sf::Keyboard::F2) {
        pause = !pause;
        if (pause) {
          emulatorSpeaker_.stop();
          emulatorScreen_.setTip("Game is pausing");
        } else {
          emulatorSpeaker_.play();
          emulatorScreen_.setTip("Game resumes");
          cycleTimer_ = std::chrono::high_resolution_clock::now();
        }
      } else if (pause && event.type == sf::Event::KeyReleased &&
                 event.key.code == sf::Keyboard::F3) {
        for (int i = 0; i < 29781; ++i) {  // Around one frame
          XPUTick();
        }
      } else if (focus && event.type == sf::Event::KeyReleased &&
                 event.key.code == sf::Keyboard::F3) {
        // Reset();
        // XPUTick();
        DebugDump();
      } else if (focus && event.type == sf::Event::KeyReleased &&
                 event.key.code == sf::Keyboard::F4) {
        // Log::get().setLevel(Info);
      } else if (focus && event.type == sf::Event::KeyReleased &&
                 event.key.code == sf::Keyboard::F5) {
        // Log::get().setLevel(InfoVerbose);
        switch (workMode_) {
          case PLAYING:
            workMode_ = RECORDING;
            emulatorScreen_.setTip("Start recording");
            break;
          case RECORDING:
            workMode_ = REPLAY;
            emulatorScreen_.setTip("Start replay");
            record_.Save();
            Reset();
            break;
          default:
            break;
        }
      } else if (focus && event.type == sf::Event::KeyReleased &&
                 event.key.code == sf::Keyboard::F6) {
        emulatorSpeaker_.stop();

        PatternViewer pv;
        pv.setCartridge(cartridge_);
        pv.run();

        emulatorSpeaker_.play();
      } else if (focus && event.type == sf::Event::KeyReleased &&
                 event.key.code == sf::Keyboard::F7) {
        goldfinger_.Toggle();
        emulatorScreen_.setTip(goldfinger_.IsWorking() ? "Gold finger enable"
                                                       : "Gold finger disable");
      }
    }

    if (focus && !pause) {
      elapsedTime_ += std::chrono::high_resolution_clock::now() - cycleTimer_;
      cycleTimer_ = std::chrono::high_resolution_clock::now();

      while (elapsedTime_ > cpuCycleDuration_) {
        XPUTick();

        elapsedTime_ -= cpuCycleDuration_;
      }

      window_.draw(emulatorScreen_);
      window_.display();
    } else {
      sf::sleep(sf::milliseconds(1000 / 60));
    }
  }
}

void Emulator::Reset() {
  mapper_->Reset();
  cpu_.Reset();
  ppu_.Reset();
  apu_.Reset();
  cycleTimer_ = std::chrono::high_resolution_clock::now();
  elapsedTime_ = cycleTimer_ - cycleTimer_;
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

  DDCATCH();

  goldfinger_.Patrol();
}

void Emulator::DMA(Byte page) {
  cpu_.skipDMACycles();
  auto page_ptr = bus_.getPagePtr(page);
  ppu_.doDMA(page_ptr);
}

Byte Emulator::ReadJoypad(int no) {
  Byte ret;
  if (workMode_ == REPLAY) {
    return record_.Read(no, cpu_.clock_cycles());
  } else {
    ret = controllers_[no].read();

    if (workMode_ == RECORDING) {
      if (ret & 0xbf) {
        record_.Record(no, cpu_.clock_cycles(), ret);
      }
    }
  }

  return ret;
}

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

void Emulator::setKeys(const ControllerInputConfig &p1,
                       const ControllerInputConfig &p2) {
  controllers_[0].setKeyBindings(p1);
  controllers_[1].setKeyBindings(p2);
}

bool Emulator::LoadCartridge(const std::string &rom_path) {
  if (!cartridge_.loadFromFile(rom_path)) return false;

  goldfinger_.LoadFile(rom_path + ".cht");

  return true;
}

void Emulator::SetCartridge(const Cartridge &cartridge) {
  cartridge_ = cartridge;
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

}  // namespace hn
