#include "AudioChannel.h"

#include "glog/logging.h"

namespace hn {

constexpr float kNtscCPURate = 1789773.0;
constexpr int kSamplesPerSecond = 44100;
constexpr int kSamplesPerFRAME = kSamplesPerSecond / 60;
constexpr float kCpuCyclePerSample = kNtscCPURate / kSamplesPerSecond;

// 长度计数器映射表
static const uint8_t LENGTH_COUNTER_TABLE[] = {
    0x0A, 0xFE, 0x14, 0x02, 0x28, 0x04, 0x50, 0x06, 0xA0, 0x08, 0x3C,
    0x0A, 0x0E, 0x0C, 0x1A, 0x0E, 0x0C, 0x10, 0x18, 0x12, 0x30, 0x14,
    0x60, 0x16, 0xC0, 0x18, 0x48, 0x1A, 0x10, 0x1C, 0x20, 0x1E,
};

static const uint16_t DMC_PERIOD_LIST_NP[] = {
    // NTSC
    428, 380, 340, 320, 286, 254, 226, 214,
    190, 160, 142, 128, 106, 84,  72,  54};

void AudioChannel::setEnable(bool enable) {
  enable_ = enable;
  if (!enable) {
    lengthCounter_ = volume_ = 0;
  }
}

void TriangleChannel::setEnable(bool enable) {
  AudioChannel::setEnable(enable);

  if (enable_) volume_ = 1;
}

void DMCChannel::setEnable(bool enable) {
  AudioChannel::setEnable(enable);

  if (enable_) {
    Reset();
  } else {
    bytesRemaining_ = 0;
    // cpu_.ClearIRQ();
  }
}

// 包络单元
void EnvelopedChannel::ProcessEnvelope() {
  // 写入了第四个寄存器(START 标记了)
  if (start_flag_) {
    start_flag_ = false;
    decay_ = 15;
    divider_ = ctrl6_ & 0x0F;
  } else {
    // 对时钟分频器发送一个时钟信号
    if (divider_) {
      divider_--;
    } else {  // 时钟分频器输出一个信号
      divider_ = ctrl6_ & 0x0F;
      //设置了循环包络位
      const Byte loop = ctrl6_ & 0x20;
      if (decay_ | loop) {
        --decay_;
        decay_ &= 0xf;
      }
    }
  }
}

void EnvelopedChannel::Save(std::ostream &os) {
  AudioChannel::Save(os);

  Write(os, start_flag_);
  Write(os, divider_);
  Write(os, decay_);
  Write(os, ctrl6_);
}

void EnvelopedChannel::Restore(std::istream &is) {
  AudioChannel::Restore(is);

  Read(is, start_flag_);
  Read(is, divider_);
  Read(is, decay_);
  Read(is, ctrl6_);
}

Byte TriangleChannel::MakeSamples() {
  // static const Byte TRI_SEQ[] = {
  // 15, 14, 13, 12, 11, 10, 9,  8,  7,  6, 5,
  // 4,  3,  2,  1,  0,  0,  1,  2,  3,  4, 5,
  // 6,  7,  8,  9,  10, 11, 12, 13, 14, 15};

  cycle_ += kCpuCyclePerSample;
  const uint16_t cycle = cycle_ / (period_ + 1);
  const uint8_t inc = incMask_ & cycle;
  cycle_ -= cycle * (period_ + 1);
  seqIndex_ += inc;
  seqIndex_ &= 31;

  Byte volume = volume_;
  if (seqIndex_ < 16) {
    volume *= 15 - seqIndex_;
  } else {
    volume *= 0x10 ^ seqIndex_;
  }

  return volume;
}

Byte PulseChannel::MakeSamples() {
  static const Byte PL_SEQ[4] = {2, 6, 0x1e, 0xf9};
  //{0, 1, 0, 0, 0, 0, 0, 0},
  //{0, 1, 1, 0, 0, 0, 0, 0},
  //{0, 1, 1, 1, 1, 0, 0, 0},
  //{1, 0, 0, 1, 1, 1, 1, 1}};
  cycle_ += kCpuCyclePerSample * 0.5f;
  const int cycle = (cycle_ / (period_ + 1));
  cycle_ -= cycle * (period_ + 1);
  seqIndex_ += cycle;
  seqIndex_ &= 7;
  return volume_ * ((PL_SEQ[ctrl_ >> 6] >> seqIndex_) & 1);
}

static inline uint16_t lfsrChange(uint16_t v, uint8_t c) {
  const uint16_t bit = ((v >> 0) ^ (v >> c)) & 1;
  return (uint16_t)(v >> 1) | (uint16_t)(bit << 14);
}

uint8_t NoiseChannel::MakeSamples() {
  cycle_ += kCpuCyclePerSample;
  while (cycle_ >= period_) {
    cycle_ -= period_;
    lfsr_ = lfsrChange(lfsr_, bitsRemaining_);
  }

  // 为0输出
  uint8_t mask = lfsr_ & 1;
  --mask;
  return volume_ & mask;
}

uint8_t DMCChannel::MakeSamples() {
  clock_ += kCpuCyclePerSample;
  const uint16_t period = period_;
  while (clock_ >= period) {
    clock_ -= period;
    UpdateDmcBit();
  }
  return volume_;
}

void DMCChannel::UpdateDmcBit() {
  if (!enable_) return;
  //如果当前BIT已经播放完毕
  if (bitsRemaining_ <= 0) {
    //如果还有BYTE长度剩余
    if (bytesRemaining_) {
      bitsRemaining_ = 8;
      sampleBuffer_ = bus_.read(address_);
      address_ = (uint16_t)(address_ + 1) | (uint16_t)0x8000;
      --bytesRemaining_;
      //读取完毕
      if (bytesRemaining_ <= 0) {
        if (!irqLoop_) {
          //设置了循环播放的标志，则重置();
          if (irqEnable_) {
            interruptFlag_ = true;
            VLOG(2) << "cpu try irq";
            // famicom->cpu->TryIrq(IRQ_DMC_INT);
          }
        } else {
        }
      }
    }
  }

  if (bitsRemaining_) {
    if (sampleBuffer_ & 1) {
      if (volume_ <= 125) volume_ += 2;
    } else {
      if (volume_ >= 2) volume_ -= 2;
    }

    sampleBuffer_ >>= 1;
    --bitsRemaining_;
  }
}

// 更新三角波的状态
void TriangleChannel::UpdateState() {
  // mTriangle.volume = mTriangle.enable;
  // 递增掩码: 长度计数器/线性计数器 有效
  incMask_ = (lengthCounter_ && linearCounter_) ? 0xff : 0;
}

// 更新脉冲（方波）的状态
void PulseChannel::UpdateState() {
  // 使能
  if (!enable_) {
    volume_ = 0;
    return;
  }
  // 长度计数器为0
  if (lengthCounter_ == 0) {
    volume_ = 0;
    return;
  }
  // 输出频率
  if (period_ < 8 || period_ > 0x7ff) {
    volume_ = 0;
  }

  // state.duty = famicom->apu.square1.ctrl >> 6;
  if (ctrl6_ & 0x10)  // 固定音量
    volume_ = ctrl6_ & 0xf;
  else  // 包络音量
    volume_ = decay_;
}

// 更新噪音的状态
void NoiseChannel::UpdateState() {
  static const uint16_t NOISE_PERIOD_LIST[] = {
      4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068,
  };
  // 短模式
  bitsRemaining_ = shortMode_ ? 6 : 1;
  period_ = NOISE_PERIOD_LIST[periodIndex_ & 0xF];

  if (!enable_) {
    volume_ = 0;
    return;
  }
  // 长度计数器有效
  if (!lengthCounter_) {
    volume_ = 0;
    return;
  }
  // Fixed volume
  if (ctrl6_ & 0x10) {
    volume_ = ctrl6_ & 0xf;
  } else {  // envelop volume
    volume_ = decay_;
  }
}

void PulseChannel::ProcessSweepUnit(bool isOne) {
  do {
    if (sweep_.reload) {
      sweep_.reload = 0;
      const uint8_t oldDivider = sweep_.divider;
      sweep_.divider = sweep_.period;
      if (sweep_.enable && !oldDivider) break;
      return;
    }

    if (!sweep_.enable) return;

    if (sweep_.divider) {
      sweep_.divider--;
      return;
    } else {
      sweep_.divider = sweep_.period;
    }
  } while (false);

  if (sweep_.shift) {
    uint16_t val1 = period_ >> sweep_.shift;
    if (sweep_.negative) {
      period_ -= val1;
      if (isOne) period_--;  //额外减去1
    } else {
      period_ += val1;
    }
  }
}

void PulseChannel::ProcessLengthCounter() {
  if (!(ctrl_ & 0x20) && lengthCounter_) lengthCounter_--;
}

void TriangleChannel::ProcessLengthCounter() {
  if (!(flagHalt_) && lengthCounter_) --lengthCounter_;
}

void NoiseChannel::ProcessLengthCounter() {
  if (!(ctrl6_ & (Byte)0x20) && lengthCounter_) --lengthCounter_;
}

void PulseChannel::WriteControl(Address address, Byte data) {
  switch (address & 3) {
    case 0x000:  //方波1 寄存器1
      // $4000 / $4004  DDLC VVVV
      ctrl_ = data;
      volume_ = data & 0x0f;
      ctrl6_ = data & 0x3f;
      break;
    case 0x001:  //方波1 寄存器2
      // $4001 / $4005  EPPP NSSS
      sweep_.enable = data >> 7;
      sweep_.period = (data >> 4) & 0x7;
      sweep_.negative = (data >> 3) & 0x1;
      sweep_.shift = data & 0x7;
      sweep_.reload = 1;
      break;
    case 0x002:  //方波1 寄存器3
      // $4002 / $4006  TTTT TTTT
      period_ &= 0xff00;
      period_ |= data;
      break;
    case 0x003:  //方波1 寄存器4
      // $4003 / $4007  LLLL LTTT
      period_ &= 0xff;
      period_ |= ((data & 0x7) << 8);
      //方波使能才重置长度计数器
      if (enable_) lengthCounter_ = LENGTH_COUNTER_TABLE[data >> 3];
      start_flag_ = true;
      seqIndex_ = 0;
      break;
  }
}

void TriangleChannel::WriteControl(Address address, Byte data) {
  switch (address) {
    case 0x4008:  // $4008 CRRR RRRR
      reloadValue_ = data & 0x7F;
      flagHalt_ = data >> 7;
      break;
    case 0x4009:
      break;
    case 0x400A:  // $400A TTTT TTTT
      period_ = (period_ & 0xff00) | data;
      break;
    case 0x400B:    // $400B LLLL LTTT
      if (enable_)  // 禁止状态不会重置
        lengthCounter_ = LENGTH_COUNTER_TABLE[data >> 3];
      period_ = (period_ & 0x00ff) | ((data & 0x07) << 8);
      reload_ = true;
      break;
  }
}

void NoiseChannel::WriteControl(Address address, Byte data) {
  switch (address) {
    case 0x400C:  // $400C --LC NNNN
      ctrl6_ = data & 0x3F;
      break;
    case 0x400D:
      break;
    case 0x400E:  // $400E S--- PPPP
      shortMode_ = data >> 7;
      periodIndex_ = data & 0xf;
      break;
    case 0x400F:  // $400E LLLL L---
      // 禁止状态不会重置
      if (enable_) lengthCounter_ = LENGTH_COUNTER_TABLE[data >> 3];
      start_flag_ = true;
      break;
  }
}

void DMCChannel::WriteControl(Address address, Byte data) {
  switch (address) {
    case 0x4010:  // $4010 IL-- RRRR
      irqEnable_ = (data >> 7) & 0x1;
      irqLoop_ = (data >> 6) & 0x1;
      period_ = DMC_PERIOD_LIST_NP[data & 0xF];
      clock_ = 0;
      if (!irqEnable_) {
        interruptFlag_ = false;
        // famicom->cpu->ClrIrq(IRQ_DMC_INT);
        VLOG(2) << "cpu clear irq";
      }
      break;
    case 0x4011:  // $4011 -DDD DDDD
      volume_ = data & 0x7F;
      break;
    case 0x4012:  // $4012 AAAA AAAA
                  // 11AAAAAA.AA000000
      orgaddr_ = 0xC000 | ((uint16_t)data << 6);
      break;
    case 0x4013:  // $4013 LLLL LLLL
                  // 0000LLLL.LLLL0001
      length_ = ((uint16_t)data << 4) | 1;
      break;
  }
}
void DMCChannel::Reset() {
  address_ = orgaddr_;
  bytesRemaining_ = length_;
}

// 线性计数单元
void TriangleChannel::ProcessLinearCounter() {
  if (reload_) {
    linearCounter_ = reloadValue_;
  } else if (linearCounter_) {
    --linearCounter_;
  }
  if (!flagHalt_) reload_ = false;
}

void AudioChannel::Save(std::ostream &os) {
  Write(os, enable_);
  Write(os, volume_);
  Write(os, lengthCounter_);
  Write(os, period_);
}

void AudioChannel::Restore(std::istream &is) {
  Read(is, enable_);
  Read(is, volume_);
  Read(is, lengthCounter_);
  Read(is, period_);
}

void DMCChannel::Save(std::ostream &os) {
  AudioChannel::Save(os);

  Write(os, orgaddr_);
  Write(os, address_);
  Write(os, length_);
  Write(os, bytesRemaining_);
  Write(os, clock_);
  Write(os, irqEnable_);
  Write(os, irqLoop_);
  Write(os, bitsRemaining_);
  Write(os, sampleBuffer_);
  Write(os, interruptFlag_);
}

void DMCChannel::Restore(std::istream &is) {
  AudioChannel::Restore(is);

  Read(is, orgaddr_);
  Read(is, address_);
  Read(is, length_);
  Read(is, bytesRemaining_);
  Read(is, clock_);
  Read(is, irqEnable_);
  Read(is, irqLoop_);
  Read(is, bitsRemaining_);
  Read(is, sampleBuffer_);
  Read(is, interruptFlag_);
}

void TriangleChannel::Save(std::ostream &os) {
  AudioChannel::Save(os);

  Write(os, cycle_);
  Write(os, linearCounter_);
  Write(os, reloadValue_);
  Write(os, reload_);
  Write(os, flagHalt_);
  Write(os, seqIndex_);
  Write(os, incMask_);
}

void TriangleChannel::Restore(std::istream &is) {
  AudioChannel::Restore(is);

  Read(is, cycle_);
  Read(is, linearCounter_);
  Read(is, reloadValue_);
  Read(is, reload_);
  Read(is, flagHalt_);
  Read(is, seqIndex_);
  Read(is, incMask_);
}

void NoiseChannel::Save(std::ostream &os) {
  EnvelopedChannel::Save(os);

  Write(os, cycle_);
  Write(os, bitsRemaining_);
  Write(os, lfsr_);
  Write(os, shortMode_);
  Write(os, periodIndex_);
}

void NoiseChannel::Restore(std::istream &is) {
  EnvelopedChannel::Restore(is);

  Read(is, cycle_);
  Read(is, bitsRemaining_);
  Read(is, lfsr_);
  Read(is, shortMode_);
  Read(is, periodIndex_);
}

void PulseChannel::Save(std::ostream &os) {
  EnvelopedChannel::Save(os);

  Write(os, sweep_);

  Write(os, curTimer_);
  Write(os, ctrl_);
  Write(os, seqIndex_);
  Write(os, cycle_);
}

void PulseChannel::Restore(std::istream &is) {
  EnvelopedChannel::Restore(is);

  Read(is, sweep_);

  Read(is, curTimer_);
  Read(is, ctrl_);
  Read(is, seqIndex_);
  Read(is, cycle_);
}

}  // namespace hn
