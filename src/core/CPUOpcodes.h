#pragma once

#define OPADDRMODE(op, n) \
  static_cast<AddrMode##n>((op & AddrModeMask) >> AddrModeShift)
#define OPACTION(op, n) \
  static_cast<Operation##n>((op & OperationMask) >> OperationShift)
#define OPNOTTYPE(op, n) ((op & InstructionModeMask) != n)
#define OPTYPE(op, n) ((op & InstructionModeMask) == n)
#define OPNOTBRANCH(op) \
  ((op & BranchInstructionMask) != BranchInstructionMaskResult)

namespace hn {
const auto InstructionModeMask = 0x3;

const auto OperationMask = 0xe0;
const auto OperationShift = 5;

const auto AddrModeMask = 0x1c;
const auto AddrModeShift = 2;

const auto BranchInstructionMask = 0x1f;
const auto BranchInstructionMaskResult = 0x10;
const auto BranchConditionMask = 0x20;
const auto BranchOnFlagShift = 6;

const auto NMIVector = 0xfffa;
const auto ResetVector = 0xfffc;
const auto IRQVector = 0xfffe;

enum InterruptType {
  IT_BRK = 0x01,
  IT_NMI = 0x02,
  IT_IRQ = 0x04,
};

enum BranchOnFlag {
  Negative = 0x00,
  Overflow = 0x01,
  Carry = 0x02,
  Zero = 0x03
};

// 状态标志位
enum StatusFlag {
  C = (1 << 0),  // Carry
  Z = (1 << 1),  // Zero
  I = (1 << 2),  // Disable Interrupts
  D = (1 << 3),  // Decimal Mode（No Use）
  B = (1 << 4),  // Break
  U = (1 << 5),  // Unused(Unknown)
  V = (1 << 6),  // Overflow
  N = (1 << 7),  // Negative
  INIT_FLAG = U | I,
};

enum Operation1 {
  ORA,
  AND,
  EOR,
  ADC,
  STA,
  LDA,
  CMP,
  SBC,
};

enum AddrMode1 {
  IndexedIndirectX,
  ZeroPage,
  Immediate,
  Absolute,
  IndirectY,
  IndexedX,
  AbsoluteY,
  AbsoluteX,
};

enum Operation2 {
  ASL,
  ROL,
  LSR,
  ROR,
  STX,
  LDX,
  DEC,
  INC,
};

enum AddrMode2 {
  Immediate_,
  ZeroPage_,
  Accumulator,
  Absolute_,
  Indexed = 5,
  AbsoluteIndexed = 7,
};

enum Operation0 {
  BIT = 1,
  STY = 4,
  LDY,
  CPY,
  CPX,
};

enum OperationImplied {
  NOP = 0xea,
  BRK = 0x00,
  JSR = 0x20,
  RTI = 0x40,
  RTS = 0x60,

  JMP = 0x4C,
  JMPI = 0x6C,  // JMP Indirect

  PHP = 0x08,
  PLP = 0x28,
  PHA = 0x48,
  PLA = 0x68,

  DEY = 0x88,
  DEX = 0xca,
  TAY = 0xa8,
  INY = 0xc8,
  INX = 0xe8,

  CLC = 0x18,
  SEC = 0x38,
  CLI = 0x58,
  SEI = 0x78,
  TYA = 0x98,
  CLV = 0xb8,
  CLD = 0xd8,
  SED = 0xf8,

  TXA = 0x8a,
  TXS = 0x9a,
  TAX = 0xaa,
  TSX = 0xba,
};

// 0 implies unused opcode
extern const int OperationCycles[0x100];

}  // namespace hn
