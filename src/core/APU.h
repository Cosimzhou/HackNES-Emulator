#pragma once

#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>

#include "AudioChannel.h"
#include "MainBus.h"
#include "PeripheralDevices.h"

namespace hn {

class APU {
 public:
  APU(MainBus &bus, VirtualSpeaker &speaker);

  void Reset();
  void Step();

  void Write(Address address, Byte sampleBuffer);
  Byte Read(Address address);

  void ProcessFrameCounter();

  uint8_t MakeSamples(uint32_t cpuCycle);

  std::vector<short> output_samples_;

  void DebugDump();

 private:
  uint8_t mFrameClock = 0;
  uint8_t mFrame5Step = 0;
  bool mFrameInterrupt = false;
  bool mIRQDisable = false;

  std::size_t frame_cycle_ = 0;
  std::size_t sample_cycle_ = 0;
  std::size_t sample_segment_ = 0;

  void ProcessEnvelope();
  void ProcessSweepUnit();
  void ProcessLengthCounter();

  int16_t SoundMixer(Byte pulse1, Byte pulse2, Byte triangle, Byte noise,
                     Byte dmc);

  PulseChannel pulses_[2];
  TriangleChannel triangle_;
  NoiseChannel noise_;
  DMCChannel dmc_channel_;

  MainBus &bus_;

  VirtualSpeaker &speaker_;
};

}  // namespace hn
