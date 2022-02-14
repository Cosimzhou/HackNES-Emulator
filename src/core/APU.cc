#include "APU.h"

#include <iomanip>
#include <sstream>

#include "CPU.h"
#include "glog/logging.h"

// Ref:
//    https://wiki.nesdev.org/w/index.php?title=APU
//

namespace hn {
constexpr float kNtscCPURate = 1789773.0;
constexpr int kSamplesPerSecond = 44100;
constexpr int kFramesPerSecond = 60;
constexpr int kSamplesPerFRAME = kSamplesPerSecond / kFramesPerSecond;
constexpr float kCpuCyclePerSample = kNtscCPURate / kSamplesPerSecond;
constexpr float kCpuCyclePerFrame = kNtscCPURate / kFramesPerSecond;
constexpr float kCpuCyclePerFrameSegment = kCpuCyclePerFrame / 3;

enum {
  DMCEnable = 1 << 4,
  NoiseEnable = 1 << 3,
  TriangleEnable = 1 << 2,
  Pulse2Enable = 1 << 1,
  Pulse1Enable = 1
};

APU::APU(MainBus &bus, VirtualSpeaker &speaker)
    : bus_(bus), dmc_channel_(bus), speaker_(speaker) {}

void APU::Write(Address address, Byte data) {
  if (0x4000 <= address && address < 0x4008) {
    auto *pulse = &pulses_[address < 0x4004 ? 0 : 1];
    pulse->WriteControl(address, data);
  } else if (0x4008 <= address && address < 0x400C) {
    triangle_.WriteControl(address, data);
  } else if (0x400C <= address && address < 0x4010) {
    noise_.WriteControl(address, data);
  } else if (0x4010 <= address && address < 0x4014) {
    dmc_channel_.WriteControl(address, data);
  } else if (address == 0x4015) {
    // $4015 write ---D NT21
    pulses_[0].setEnable(data & Pulse1Enable);
    pulses_[1].setEnable(data & Pulse2Enable);
    noise_.setEnable(data & NoiseEnable);
    triangle_.setEnable(data & TriangleEnable);

    dmc_channel_.setEnable(data & DMCEnable);
    if (!dmc_channel_.enable_) {
      bus_.cpu()->ClearIRQ();
    }
    dmc_channel_.interruptFlag_ = false;
  } else if (address == 0x4017) {
    // $4017 MI-- ----
    //帧计数器(Frame Counter)

    mFrame5Step = data >> 7;  //是否五步模式
    mIRQDisable = !!(data & 0x40);

    if (mIRQDisable) {
      bus_.cpu()->ClearIRQ();
      mFrameInterrupt = 0;
    }

    if (data == 0) {
      mFrameInterrupt = 1;
      bus_.cpu()->TryIRQ();
    }

    if (mFrame5Step) {
      ProcessLengthCounter();
      ProcessSweepUnit();
      ProcessEnvelope();
    }
  } else {
    LOG(ERROR) << "unknown address " << std::hex << address;
  }
}

void APU::Reset() {
  VLOG(2) << "APU reset";
  speaker_.Stop();
  noise_.lfsr_ = 1;
  dmc_channel_.period_ = 428;

  sample_cycle_ = 0;
  frame_cycle_ = 0;
  sample_segment_ = 0;

  speaker_.Play();
}

uint8_t APU::Read(uint16_t address) {
  if (address == 0x4015) {
    uint8_t state = 0;
    if (pulses_[0].lengthCounter_) state |= 0x1;
    if (pulses_[1].lengthCounter_) state |= 0x2;
    if (triangle_.lengthCounter_) state |= 0x4;
    if (noise_.lengthCounter_) state |= 0x8;
    if (dmc_channel_.bytesRemaining_) state |= 0x10;
    if (mFrameInterrupt) state |= 0x40;
    if (dmc_channel_.interruptFlag_) state |= 0x80;

    // 清除中断标记
    mFrameInterrupt = 0;
    bus_.cpu()->ClearIRQ();

    return state;
  }

  return 0;
}

inline void APU::ProcessSweepUnit() {
  pulses_[0].ProcessSweepUnit(true);
  pulses_[1].ProcessSweepUnit(false);
}

void APU::ProcessFrameCounter() {
  if (mFrame5Step) {
    // l - l - - ->   ++%5   ->   1 2 3 4 0
    // e e e e - ->   ++%5   ->   1 2 3 4 0
    auto mod = mFrameClock % 5;
    if (mod < 4) {
      if (mod & 1) {
        ProcessLengthCounter();  // 120hz
        ProcessSweepUnit();
      }
      ProcessEnvelope();
    }
  } else {
    auto mod = mFrameClock % 5;
    if (mod == 3) {
      if (!mIRQDisable) {
        mFrameInterrupt = 1;
        bus_.cpu()->TryIRQ();
      }
    }
    if (mod & 1) {
      ProcessLengthCounter();  // 120hz
      ProcessSweepUnit();
    }
    ProcessEnvelope();
  }
  ++mFrameClock;
}

int16_t APU::SoundMixer(Byte pulse1, Byte pulse2, Byte triangle, Byte noise,
                        Byte dmc) {
#if 1
  //混合
  float square_out = 95.88f / ((8128.f / (pulse1 + pulse2)) + 100.f);
  float tnd_out =
      159.79f /
      (1.f / (triangle / 8227.f + noise / 12241.f + dmc / 22638.f) + 100.f);
  float outputf = (float)(square_out + tnd_out);
#else
  //线性近似
  float pulse_out = 0.00752 * ((int64_t)mOutputs[0] + mOutputs[1]);
  float tnd_out =
      0.00851 * mOutputs[2] + 0.00494 * mOutputs[3] + 0.00335 * mOutputs[4];
  // if ((float)(pulse_out + tnd_out) > 1.f)
  //{
  //	DebugPrintf("square_out + tnt_output:%f\n", pulse_out + tnd_out);
  //}
  float outputf = (float)(pulse_out + tnd_out);

#endif

  outputf *= 32767.f;
  static float maxv = -1e30, minv = 1e30;
  if (maxv < outputf || minv > outputf) {
    if (maxv < outputf) maxv = outputf;
    if (minv > outputf) minv = outputf;

    VLOG(2) << "mixer range(" << minv << "," << maxv << ")";
  }

  if (outputf > 0x7FFF)
    outputf = 0x7FFF;
  else if (outputf < -0x8000)
    outputf = -0x8000;

  return static_cast<int16_t>(outputf);
}

void APU::Step() {
  if (++frame_cycle_ >= kCpuCyclePerFrame / 4) {
    frame_cycle_ = 0;
    ProcessFrameCounter();
  }

  if (++sample_cycle_ < kCpuCyclePerFrameSegment) {
    return;
  }

  sample_cycle_ = 0;

  MakeSamples(0);

  speaker_.PushSample(output_samples_.data(), output_samples_.size());
}

uint8_t APU::MakeSamples(uint32_t cpuCycle) {
  pulses_[0].UpdateState();
  pulses_[1].UpdateState();
  triangle_.UpdateState();
  noise_.UpdateState();
#if 0
  static int remnantsCycle = 0;
	const uint32_t nSampleSize = (remnantsCycle + cpuCycle) / kCpuCyclePerSample;
	remnantsCycle = (remnantsCycle + cpuCycle) - nSampleSize * kCpuCyclePerSample;
	Sample* sample = &output_samples_;
	sample->size = nSampleSize;
#else
  const uint32_t nSampleSize = 245;
  auto &sample = output_samples_;
  sample.resize(nSampleSize);
#endif
  for (size_t i = 0; i < nSampleSize; i++) {
    sample[i] = SoundMixer(pulses_[0].MakeSamples(),   // input 1
                           pulses_[1].MakeSamples(),   // input 2
                           triangle_.MakeSamples(),    // input 3
                           noise_.MakeSamples(),       // input 4
                           dmc_channel_.MakeSamples()  // input 5
    );
  }

  return 0;
}

inline void APU::ProcessLengthCounter() {
  pulses_[0].ProcessLengthCounter();
  pulses_[1].ProcessLengthCounter();
  triangle_.ProcessLengthCounter();
  noise_.ProcessLengthCounter();
}

inline void APU::ProcessEnvelope() {
  pulses_[0].ProcessEnvelope();
  pulses_[1].ProcessEnvelope();
  noise_.ProcessEnvelope();
  //
  triangle_.ProcessLinearCounter();
}

void APU::DebugDump() {}
}  // namespace hn
