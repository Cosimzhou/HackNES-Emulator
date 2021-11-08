#pragma once

#include "MainBus.h"
#include "common.h"

namespace hn {
class AudioChannel {
 public:
  AudioChannel() {}

  virtual Byte MakeSamples() = 0;
  virtual void ProcessLengthCounter() {}
  virtual void UpdateState() {}
  virtual void WriteControl(Address address, Byte data) = 0;

  virtual void setEnable(bool enable);

  bool enable_;
  Byte volume_;
  Byte lengthCounter_;
  uint16_t period_;
};

struct EnvelopedChannel : public AudioChannel {
  bool start_flag_;  //是否重载
  uint8_t divider_;  // 时钟分频器
  uint8_t decay_;    // 计数器
  uint8_t ctrl6_;    // 控制器低6位

  void ProcessEnvelope();
};

struct NoiseChannel : public EnvelopedChannel {
  // EnvelopedChannel
  uint32_t cycle_;
  uint8_t bitsRemaining_;
  uint16_t lfsr_;        // 线性反馈移位寄存器
  uint8_t shortMode_;    // 短模式D7
  uint8_t periodIndex_;  //周期索引 D0~D3

  virtual Byte MakeSamples() override;
  virtual void ProcessLengthCounter() override;
  virtual void UpdateState() override;
  virtual void WriteControl(Address address, Byte data) override;
};

/// DMC
struct DMCChannel : public AudioChannel {
  Address orgaddr_;          // 原始地址
  Address address_;          // 当前地址
  uint16_t length_;          // 原始长度
  uint16_t bytesRemaining_;  // 剩余长度
  uint16_t clock_;           // CPU时钟
  uint8_t irqEnable_;        // 中断
  uint8_t irqLoop_;          // 中断[D1]/循环[D0]
  uint8_t bitsRemaining_;    // 8步计数
  uint8_t sampleBuffer_;     // 字节数据[8字节位移寄存器]
  uint8_t interruptFlag_;    // 字节数据[8字节位移寄存器]

  DMCChannel(MainBus &bus) : bus_(bus) {}

  virtual Byte MakeSamples() override;
  virtual void WriteControl(Address address, Byte data) override;
  virtual void setEnable(bool enable) override;
  // virtual void ProcessLengthCounter() override;
  // virtual void UpdateState() override;

  void UpdateDmcBit();
  void Reset();

  MainBus &bus_;
};

// Triangle wave
struct TriangleChannel : public AudioChannel {
  uint32_t cycle_;
  uint8_t linearCounter_;
  uint8_t reloadValue_;
  uint8_t reload_;
  uint8_t flagHalt_;
  uint8_t seqIndex_;  // Current sequence index
  uint8_t incMask_;   // Volume

  virtual Byte MakeSamples() override;
  virtual void ProcessLengthCounter() override;
  virtual void UpdateState() override;
  virtual void WriteControl(Address address, Byte data) override;

  virtual void setEnable(bool enable) override;

  void ProcessLinearCounter();
};

/// Linear sweep unit
typedef struct {
  uint8_t enable;    //扫描单元使能 E
  uint8_t period;    //扫描单元周期（分频） PPP
  uint8_t negative;  //是否负向扫描 N
  uint8_t shift;     //扫描单元位移次数(位移位数) SSS
  uint8_t divider;
  uint8_t reload;
} Sweep;

// Pulse square wave
struct PulseChannel : public EnvelopedChannel {
  // EnvelopedChannel
  //$4001/$4005 EPPPNSSS
  Sweep sweep_;

  //长度计数器 5BIT
  //
  //声道周期 11BIT  $4002/$4006 D7~D0 低8位+
  //$4003/$4007 D2~D0 高3位
  uint16_t curTimer_;
  //$4000	DDLC NNNN
  uint8_t ctrl_;      //占空比 D
  uint8_t seqIndex_;  //序列索引
  uint32_t cycle_;

  virtual Byte MakeSamples() override;
  virtual void ProcessLengthCounter() override;
  virtual void UpdateState() override;
  virtual void WriteControl(Address address, Byte data) override;

  void ProcessSweepUnit(bool isOne);
};

}  // namespace hn
