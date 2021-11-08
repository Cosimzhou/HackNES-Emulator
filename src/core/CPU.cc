#include "CPU.h"

#include <iomanip>

#include "Disassembler.h"
#include "glog/logging.h"

namespace hn {
// 0 implies unused opcode
const int OperationCycles[0x100] = {
    7, 6, 0, 0, 0, 3, 5, 0, 3, 2, 2, 0, 0, 4, 6, 0, 2, 5, 0, 0, 0, 4, 6, 0,
    2, 4, 0, 0, 0, 4, 7, 0, 6, 6, 0, 0, 3, 3, 5, 0, 4, 2, 2, 0, 4, 4, 6, 0,
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0, 6, 6, 0, 0, 0, 3, 5, 0,
    3, 2, 2, 0, 3, 4, 6, 0, 2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
    6, 6, 0, 0, 0, 3, 5, 0, 4, 2, 2, 0, 5, 4, 6, 0, 2, 5, 0, 0, 0, 4, 6, 0,
    2, 4, 0, 0, 0, 4, 7, 0, 0, 6, 0, 0, 3, 3, 3, 0, 2, 0, 2, 0, 4, 4, 4, 0,
    2, 6, 0, 0, 4, 4, 4, 0, 2, 5, 2, 0, 0, 5, 0, 0, 2, 6, 2, 0, 3, 3, 3, 0,
    2, 2, 2, 0, 4, 4, 4, 0, 2, 5, 0, 0, 4, 4, 4, 0, 2, 4, 2, 0, 4, 4, 4, 0,
    2, 6, 0, 0, 3, 3, 5, 0, 2, 2, 2, 0, 4, 4, 6, 0, 2, 5, 0, 0, 0, 4, 6, 0,
    2, 4, 0, 0, 0, 4, 7, 0, 2, 6, 0, 0, 3, 3, 5, 0, 2, 2, 2, 2, 4, 4, 6, 0,
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
};

CPU::CPU(MainBus &mem) : bus_(mem) {}

void CPU::Reset() { Reset(readAddress(ResetVector)); }

void CPU::Reset(Address start_addr) {
  skipCycles_ = cycles_ = 0;
  reg_A_ = reg_X_ = reg_Y_ = 0;
#ifdef PSW_IN_BYTE
  psw_ = static_cast<Byte>(StatusFlag::INIT_FLAG);
#else   // PSW_IN_BYTE
  flag_I_ = true;
  flag_C_ = flag_D_ = flag_N_ = flag_V_ = flag_Z_ = false;
#endif  // PSW_IN_BYTE

  reg_PC_ = start_addr;
  reg_SP_ = 0xfd;  // documented startup state
}

void CPU::interrupt(InterruptType type) {
#ifdef PSW_IN_BYTE
  if (getFlag(StatusFlag::I) && type != NMI && type != BRK)
#else   // PSW_IN_BYTE
  if (flag_I_ && type != NMI && type != BRK)
#endif  // PSW_IN_BYTE
  {
    return;
  }

  if (type == BRK) {
    // Add one if BRK, a quirk of 6502
    ++reg_PC_;
  }

  pushStack(reg_PC_ >> 8);
  pushStack(reg_PC_);

#ifdef PSW_IN_BYTE
  pushStack(psw_ | ((type == BRK) << 4));
  setFlag(true, StatusFlag::I);
#else   // PSW_IN_BYTE
  Byte flags = flag_N_ << 7 | flag_V_ << 6 |
               1 << 5 |              // unused bit, supposed to be always 1
               (type == BRK) << 4 |  // B flag set if BRK
               flag_D_ << 3 | flag_I_ << 2 | flag_Z_ << 1 | flag_C_;
  pushStack(flags);

  flag_I_ = true;
#endif  // PSW_IN_BYTE

  switch (type) {
    case IRQ:
    case BRK:
      reg_PC_ = readAddress(IRQVector);
      break;
    case NMI:
      reg_PC_ = readAddress(NMIVector);
      break;
  }

  skipCycles_ += 7;
}

void CPU::pushStack(Byte value) {
  bus_.write(0x100 | reg_SP_, value);
  --reg_SP_;  // Hardware stacks grow downward!
}

Byte CPU::popStack() { return bus_.read(0x100 | ++reg_SP_); }

void CPU::setZN(Byte value) {
#ifdef PSW_IN_BYTE
  setFlag(!value, StatusFlag::Z);
  setFlag(value & 0x80, StatusFlag::N);
#else   // PSW_IN_BYTE
  flag_Z_ = !value;
  flag_N_ = value & 0x80;
#endif  // PSW_IN_BYTE
}

void CPU::setPageCrossed(Address a, Address b, int inc) {
  // Page is determined by the high byte
  if ((a & 0xff00) != (b & 0xff00)) {
    skipCycles_ += inc;
  }
}

void CPU::skipDMACycles() {
  skipCycles_ += 513;            // 256 read + 256 write + 1 dummy read
  skipCycles_ += (cycles_ & 1);  //+1 if on odd cycle
}

void CPU::Step() {
  ++cycles_;

  if (skipCycles_-- > 1) return;

  skipCycles_ = 0;

  int psw =
#ifdef PSW_IN_BYTE
      psw_;
#else   // PSW_IN_BYTE
      flag_N_ << 7 | flag_V_ << 6 | 1 << 5 | flag_D_ << 3 | flag_I_ << 2 |
      flag_Z_ << 1 | flag_C_;
#endif  // PSW_IN_BYTE
  VLOG(8) << std::hex << std::setfill('0') << std::uppercase << std::setw(4)
          << +reg_PC_ << "  " << std::setw(2) << +bus_.read(reg_PC_) << "  "
          << "A:" << std::setw(2) << +reg_A_ << " "
          << "X:" << std::setw(2) << +reg_X_ << " "
          << "Y:" << std::setw(2) << +reg_Y_ << " "
          << "P:" << std::setw(2) << psw << " "
          << "SP:" << std::setw(2) << +reg_SP_ << " "
          << "CYC:" << std::setw(3) << std::setfill(' ') << std::dec
          << ((cycles_ - 1) * 3) % 341 << std::endl;

  // Check IRQ or NMI
  if (TEST_BITS(irq_flag_, 1)) {
    CLR_BIT(irq_flag_, 1);
    interrupt(IRQ);
    return;
  }

  old_PC_ = reg_PC_;
  Byte opcode = bus_.read(reg_PC_++);
  // LOG(INFO) << "opcode " << int(opcode);

  auto CycleLength = OperationCycles[opcode];

  // Using short-circuit evaluation, call the other function only if the first
  // failed ExecuteImplied must be called first and ExecuteBranch must be before
  // ExecuteType0
  if (CycleLength &&
      (executeImplied(opcode) || executeBranch(opcode) ||
       executeType1(opcode) || executeType2(opcode) || executeType0(opcode))) {
    skipCycles_ += CycleLength;
    // cycles_ %= 340; //compatibility with Nintendulator log
    // skipCycles_ = 0; //for TESTING
  } else {
    LOG(ERROR) << "Unrecognized opcode:" << std::hex << int(opcode)
               << " @pc: " << reg_PC_;
    DDREPORT();
  }
}

bool CPU::executeImplied(Byte opcode) {
  switch (static_cast<OperationImplied>(opcode)) {
    case NOP:
      break;
    case BRK:
      interrupt(BRK);
      break;
    case JSR:
      // Push address of next instruction - 1, thus reg_PC_ + 1 instead of
      // reg_PC_
      // + 2 since reg_PC_ and reg_PC_ + 1 are address of subroutine
      pushStack(static_cast<Byte>((reg_PC_ + 1) >> 8));
      pushStack(static_cast<Byte>(reg_PC_ + 1));
      reg_PC_ = readAddress(reg_PC_);
      break;
    case RTS:
      reg_PC_ = popStack();
      reg_PC_ |= static_cast<Address>(popStack()) << 8;
      ++reg_PC_;
      break;
    case RTI:
#ifdef PSW_IN_BYTE
      psw_ = popStack();
      setFlag(false, StatusFlag::U);
      setFlag(false, StatusFlag::B);
#else   // PSW_IN_BYTE
    {
      Byte flags = popStack();

      flag_N_ = flags & 0x80;
      flag_V_ = flags & 0x40;
      flag_D_ = flags & 0x8;
      flag_I_ = flags & 0x4;
      flag_Z_ = flags & 0x2;
      flag_C_ = flags & 0x1;
    }
#endif  // PSW_IN_BYTE
      reg_PC_ = popStack();
      reg_PC_ |= static_cast<Address>(popStack()) << 8;
      break;
    case JMP:
      reg_PC_ = readAddress(reg_PC_);
      break;
    case JMPI: {
      Address location = readAddress(reg_PC_);
      // 6502 has a bug such that the when the vector of anindirect address
      // begins at the last byte of a page, the second byte is fetched from the
      // beginning of that page rather than the beginning of the next Recreating
      // here:
      Address Page = location & 0xff00;
      reg_PC_ = bus_.read(location) | bus_.read(Page | ((location + 1) & 0xff))
                                          << 8;
    } break;
    case PHP: {
#ifdef PSW_IN_BYTE
      // psw_ |= 0x30;
      pushStack(psw_ | (1 << 4) | (1 << 5));
      setFlag(false, StatusFlag::U);
      setFlag(false, StatusFlag::B);
#else   // PSW_IN_BYTE
      Byte flags = flag_N_ << 7 | flag_V_ << 6 |
                   1 << 5 |  // supposed to always be 1
                   1 << 4 |  // PHP pushes with the B flag as 1, no matter what
                   flag_D_ << 3 | flag_I_ << 2 | flag_Z_ << 1 | flag_C_;

      pushStack(flags);
#endif  // PSW_IN_BYTE

    } break;
    case PLP:
#ifdef PSW_IN_BYTE
      psw_ = popStack();
      setFlag(false, StatusFlag::U);
#else   // PSW_IN_BYTE
    {
      Byte flags = popStack();
      flag_N_ = flags & 0x80;
      flag_V_ = flags & 0x40;
      flag_D_ = flags & 0x8;
      flag_I_ = flags & 0x4;
      flag_Z_ = flags & 0x2;
      flag_C_ = flags & 0x1;
    }
#endif  // PSW_IN_BYTE

      break;
    case PHA:
      pushStack(reg_A_);
      break;
    case PLA:
      reg_A_ = popStack();
      setZN(reg_A_);
      break;
    case DEY:
      --reg_Y_;
      setZN(reg_Y_);
      break;
    case DEX:
      --reg_X_;
      setZN(reg_X_);
      break;
    case TAY:
      reg_Y_ = reg_A_;
      setZN(reg_Y_);
      break;
    case INY:
      ++reg_Y_;
      setZN(reg_Y_);
      break;
    case INX:
      ++reg_X_;
      setZN(reg_X_);
      break;
    case CLC:
#ifdef PSW_IN_BYTE
      setFlag(false, StatusFlag::C);
#else   // PSW_IN_BYTE
      flag_C_ = false;
#endif  // PSW_IN_BYTE
      break;
    case SEC:
#ifdef PSW_IN_BYTE
      setFlag(true, StatusFlag::C);
#else   // PSW_IN_BYTE
      flag_C_ = true;
#endif  // PSW_IN_BYTE
      break;
    case CLI:
#ifdef PSW_IN_BYTE
      setFlag(false, StatusFlag::I);
#else   // PSW_IN_BYTE
      flag_I_ = false;
#endif  // PSW_IN_BYTE
      break;
    case SEI:
#ifdef PSW_IN_BYTE
      setFlag(true, StatusFlag::I);
#else   // PSW_IN_BYTE
      flag_I_ = true;
#endif  // PSW_IN_BYTE
      break;
    case CLD:
#ifdef PSW_IN_BYTE
      setFlag(false, StatusFlag::D);
#else   // PSW_IN_BYTE
      flag_D_ = false;
#endif  // PSW_IN_BYTE
      break;
    case SED:
#ifdef PSW_IN_BYTE
      setFlag(true, StatusFlag::D);
#else   // PSW_IN_BYTE
      flag_D_ = true;
#endif  // PSW_IN_BYTE
      break;
    case TYA:
      reg_A_ = reg_Y_;
      setZN(reg_A_);
      break;
    case CLV:
#ifdef PSW_IN_BYTE
      setFlag(false, StatusFlag::V);
#else   // PSW_IN_BYTE
      flag_V_ = false;
#endif  // PSW_IN_BYTE
      break;
    case TXA:
      reg_A_ = reg_X_;
      setZN(reg_A_);
      break;
    case TXS:
      reg_SP_ = reg_X_;
      break;
    case TAX:
      reg_X_ = reg_A_;
      setZN(reg_X_);
      break;
    case TSX:
      reg_X_ = reg_SP_;
      setZN(reg_X_);
      break;
    default:
      return false;
  };
  return true;
}

bool CPU::executeBranch(Byte opcode) {
  if (OPNOTBRANCH(opcode)) {
    return false;
  }

  // branch is initialized to the condition required (for the flag specified
  // later)
  bool branch = opcode & BranchConditionMask;

  // set branch to true if the given condition is met by the given flag
  // We use xnor here, it is true if either both operands are true or false
  switch (opcode >> BranchOnFlagShift) {
    case Negative:
      // JL / JNL
#ifdef PSW_IN_BYTE
      branch = !(branch ^ getFlag(StatusFlag::N));
#else   // PSW_IN_BYTE
      branch = !(branch ^ flag_N_);
#endif  // PSW_IN_BYTE
      break;
    case Overflow:
      // JO / JNO
#ifdef PSW_IN_BYTE
      branch = !(branch ^ getFlag(StatusFlag::V));
#else   // PSW_IN_BYTE
      branch = !(branch ^ flag_V_);
#endif  // PSW_IN_BYTE
      break;
    case Carry:
      // JC / JNC
#ifdef PSW_IN_BYTE
      branch = !(branch ^ getFlag(StatusFlag::C));
#else   // PSW_IN_BYTE
      branch = !(branch ^ flag_C_);
#endif  // PSW_IN_BYTE
      break;
    case Zero:
      // JZ / JNZ
#ifdef PSW_IN_BYTE
      branch = !(branch ^ getFlag(StatusFlag::Z));
#else   // PSW_IN_BYTE
      branch = !(branch ^ flag_Z_);
#endif  // PSW_IN_BYTE
      break;
    default:
      return false;
  }

  if (branch) {
    int8_t offset = bus_.read(reg_PC_++);
    ++skipCycles_;
    auto newPC = static_cast<Address>(reg_PC_ + offset);
    setPageCrossed(reg_PC_, newPC, 2);
    reg_PC_ = newPC;
  } else {
    ++reg_PC_;
  }

  return true;
}

bool CPU::executeType1(Byte opcode) {
  if (OPNOTTYPE(opcode, 0x1)) {
    return false;
  }

  Address location = 0;  // Location of the operand, could be in RAM
  auto op = OPACTION(opcode, 1);
  switch (OPADDRMODE(opcode, 1)) {
    case IndexedIndirectX: {
      Byte zero_addr = reg_X_ + bus_.read(reg_PC_++);
      // Addresses wrap in zero page mode, thus pass through a mask
      location = bus_.read(zero_addr & 0xff) | bus_.read((zero_addr + 1) & 0xff)
                                                   << 8;
    } break;
    case ZeroPage:
      location = bus_.read(reg_PC_++);
      break;
    case Immediate:
      location = reg_PC_++;
      break;
    case Absolute:
      location = readAddress(reg_PC_);
      reg_PC_ += 2;
      break;
    case IndirectY: {
      Byte zero_addr = bus_.read(reg_PC_++);
      location = bus_.read(zero_addr & 0xff) | bus_.read((zero_addr + 1) & 0xff)
                                                   << 8;
      if (op != STA) setPageCrossed(location, location + reg_Y_);
      location += reg_Y_;
    } break;
    case IndexedX:
      // Address wraps around in the zero page
      location = (bus_.read(reg_PC_++) + reg_X_) & 0xff;
      break;
    case AbsoluteY:
      location = readAddress(reg_PC_);
      reg_PC_ += 2;
      if (op != STA) setPageCrossed(location, location + reg_Y_);
      location += reg_Y_;
      break;
    case AbsoluteX:
      location = readAddress(reg_PC_);
      reg_PC_ += 2;
      if (op != STA) setPageCrossed(location, location + reg_X_);
      location += reg_X_;
      break;
    default:
      return false;
  }

  switch (op) {
    case ORA:
      reg_A_ |= bus_.read(location);
      setZN(reg_A_);
      break;
    case AND:
      reg_A_ &= bus_.read(location);
      setZN(reg_A_);
      break;
    case EOR:
      reg_A_ ^= bus_.read(location);
      setZN(reg_A_);
      break;
    case ADC: {
      Byte operand = bus_.read(location);
      std::uint16_t sum = reg_A_ + operand;
#ifdef PSW_IN_BYTE
      sum += +getFlag(StatusFlag::C);
      // Carry forward or UNSIGNED overflow
      setFlag(sum & 0x100, StatusFlag::C);
      // SIGNED overflow, would only happen if the sign of sum is
      // different from BOTH the operands
      setFlag((reg_A_ ^ sum) & (operand ^ sum) & 0x80, StatusFlag::V);
#else   // PSW_IN_BYTE
      sum += +flag_C_;
      flag_C_ = sum & 0x100;
      flag_V_ = (reg_A_ ^ sum) & (operand ^ sum) & 0x80;
#endif  // PSW_IN_BYTE
      reg_A_ = static_cast<Byte>(sum);
      setZN(reg_A_);
    } break;
    case STA:
      bus_.write(location, reg_A_);
      break;
    case LDA:
      reg_A_ = bus_.read(location);
      setZN(reg_A_);
      break;
    case SBC: {
      // High carry means "no borrow", thus negate and subtract
      std::uint16_t subtrahend = bus_.read(location),
                    diff = reg_A_ - subtrahend;
#ifdef PSW_IN_BYTE
      diff -= +!getFlag(StatusFlag::C);
      // if the ninth bit is 1, the resulting number is negative => borrow =>
      // low carry
      setFlag(!(diff & 0x100), StatusFlag::C);
      // Same as ADC, except instead of the subtrahend,
      // substitute with it's one complement
      setFlag((reg_A_ ^ diff) & (~subtrahend ^ diff) & 0x80, StatusFlag::V);
#else   // PSW_IN_BYTE
      diff -= +!flag_C_;
      flag_C_ = !(diff & 0x100);
      flag_V_ = (reg_A_ ^ diff) & (~subtrahend ^ diff) & 0x80;
#endif  // PSW_IN_BYTE
      reg_A_ = diff;
      setZN(diff);
    } break;
    case CMP: {
      std::uint16_t diff = reg_A_ - bus_.read(location);
#ifdef PSW_IN_BYTE
      setFlag(!(diff & 0x100), StatusFlag::C);
#else   // PSW_IN_BYTE
      flag_C_ = !(diff & 0x100);
#endif  // PSW_IN_BYTE
      setZN(diff);
    } break;
    default:
      return false;
  }

  return true;
}

bool CPU::executeType2(Byte opcode) {
  if (OPNOTTYPE(opcode, 0x2)) {
    return false;
  }

  Address location = 0;
  auto op = OPACTION(opcode, 2);
  auto addr_mode = OPADDRMODE(opcode, 2);
  switch (addr_mode) {
    case Immediate_:
      location = reg_PC_++;
      break;
    case ZeroPage_:
      location = bus_.read(reg_PC_++);
      break;
    case Accumulator:
      break;
    case Absolute_:
      location = readAddress(reg_PC_);
      reg_PC_ += 2;
      break;
    case Indexed: {
      location = bus_.read(reg_PC_++);
      Byte index;
      if (op == LDX || op == STX) {
        index = reg_Y_;
      } else {
        index = reg_X_;
      }
      // The mask wraps address around zero page
      location = (location + index) & 0xff;
    } break;
    case AbsoluteIndexed: {
      location = readAddress(reg_PC_);
      reg_PC_ += 2;
      Byte index;
      if (op == LDX || op == STX) {
        index = reg_Y_;
      } else {
        index = reg_X_;
      }
      setPageCrossed(location, location + index);
      location += index;
    } break;
    default:
      return false;
  }

  std::uint16_t operand = 0;
  switch (op) {
    case ASL:
    case ROL:
      if (addr_mode == Accumulator) {
        bool prev_C =
#ifdef PSW_IN_BYTE
            getFlag(StatusFlag::C);
        setFlag(reg_A_ & 0x80, StatusFlag::C);
#else   // PSW_IN_BYTE
            flag_C_;
        flag_C_ = reg_A_ & 0x80;
#endif  // PSW_IN_BYTE
        reg_A_ <<= 1;
        // If Rotating, set the bit-0 to the the previous carry
        reg_A_ = reg_A_ | (prev_C && (op == ROL));
        setZN(reg_A_);
      } else {
        operand = bus_.read(location);
        bool prev_C =
#ifdef PSW_IN_BYTE
            getFlag(StatusFlag::C);
        setFlag(operand & 0x80, StatusFlag::C);
#else   // PSW_IN_BYTE
            flag_C_;
        flag_C_ = operand & 0x80;
#endif  // PSW_IN_BYTE
        operand = operand << 1 | (prev_C && (op == ROL));
        setZN(operand);
        bus_.write(location, operand);
      }
      break;
    case LSR:
    case ROR:
      if (addr_mode == Accumulator) {
        bool prev_C =
#ifdef PSW_IN_BYTE
            getFlag(StatusFlag::C);
        setFlag(reg_A_ & 1, StatusFlag::C);
#else   // PSW_IN_BYTE
            flag_C_;
        flag_C_ = reg_A_ & 1;
#endif  // PSW_IN_BYTE
        reg_A_ >>= 1;
        // If Rotating, set the bit-7 to the previous carry
        reg_A_ = reg_A_ | (prev_C && (op == ROR)) << 7;
        setZN(reg_A_);
      } else {
        operand = bus_.read(location);
        bool prev_C =
#ifdef PSW_IN_BYTE
            getFlag(StatusFlag::C);
        setFlag(operand & 1, StatusFlag::C);
#else   // PSW_IN_BYTE
            flag_C_;
        flag_C_ = operand & 1;
#endif  // PSW_IN_BYTE
        operand = operand >> 1 | ((prev_C && (op == ROR)) << 7);
        setZN(operand);
        bus_.write(location, operand);
      }
      break;
    case STX:
      bus_.write(location, reg_X_);
      break;
    case LDX:
      reg_X_ = bus_.read(location);
      setZN(reg_X_);
      break;
    case DEC: {
      auto tmp = bus_.read(location) - 1;
      setZN(tmp);
      bus_.write(location, tmp);
    } break;
    case INC: {
      auto tmp = bus_.read(location) + 1;
      setZN(tmp);
      bus_.write(location, tmp);
    } break;
    default:
      LOG(ERROR) << "Unrecognized opcode:" << std::hex << int(op)
                 << " @pc: " << reg_PC_;
      return false;
  }
  return true;
}

bool CPU::executeType0(Byte opcode) {
  if (OPNOTTYPE(opcode, 0x0)) {
    return false;
  }

  Address location = 0;
  switch (OPADDRMODE(opcode, 2)) {
    case Immediate_:
      location = reg_PC_++;
      break;
    case ZeroPage_:
      location = bus_.read(reg_PC_++);
      break;
    case Absolute_:
      location = readAddress(reg_PC_);
      reg_PC_ += 2;
      break;
    case Indexed:
      // Address wraps around in the zero page
      location = (bus_.read(reg_PC_++) + reg_X_) & 0xff;
      break;
    case AbsoluteIndexed:
      location = readAddress(reg_PC_);
      reg_PC_ += 2;
      setPageCrossed(location, location + reg_X_);
      location += reg_X_;
      break;
    default:
      return false;
  }
  std::uint16_t operand = 0;
  switch (OPACTION(opcode, 0)) {
    case BIT:
      operand = bus_.read(location);
#ifdef PSW_IN_BYTE
      setFlag(!(reg_A_ & operand), StatusFlag::Z);
      setFlag(operand & 0x40, StatusFlag::V);
      setFlag(operand & 0x80, StatusFlag::N);
#else   // PSW_IN_BYTE
      flag_Z_ = !(reg_A_ & operand);
      flag_V_ = operand & 0x40;
      flag_N_ = operand & 0x80;
#endif  // PSW_IN_BYTE
      break;
    case STY:
      bus_.write(location, reg_Y_);
      break;
    case LDY:
      reg_Y_ = bus_.read(location);
      setZN(reg_Y_);
      break;
    case CPY: {
      std::uint16_t diff = reg_Y_ - bus_.read(location);
#ifdef PSW_IN_BYTE
      setFlag(!(diff & 0x100), StatusFlag::C);
#else   // PSW_IN_BYTE
      flag_C_ = !(diff & 0x100);
#endif  // PSW_IN_BYTE
      setZN(diff);
    } break;
    case CPX: {
      std::uint16_t diff = reg_X_ - bus_.read(location);
#ifdef PSW_IN_BYTE
      setFlag(!(diff & 0x100), StatusFlag::C);
#else   // PSW_IN_BYTE
      flag_C_ = !(diff & 0x100);
#endif  // PSW_IN_BYTE
      setZN(diff);
    } break;
    default:
      return false;
  }

  return true;
}

Address CPU::readAddress(Address addr) { return bus_.readAddress(addr); }

void CPU::DebugDump() {
  int psw =
#ifdef PSW_IN_BYTE
      psw_;
#else   // PSW_IN_BYTE
      flag_N_ << 7 | flag_V_ << 6 | 1 << 5 | flag_D_ << 3 | flag_I_ << 2 |
      flag_Z_ << 1 | flag_C_;
#endif  // PSW_IN_BYTE
  LOG(INFO) << std::hex << std::setfill('0') << std::setw(4)
            << "%PC=" << +reg_PC_ << " op=" << std::setw(2)
            << +bus_.read(reg_PC_) << "  "
            << "%A=" << std::setw(2) << +reg_A_ << " "
            << "%X=" << std::setw(2) << +reg_X_ << " "
            << "%Y=" << std::setw(2) << +reg_Y_ << " "
            << "%PSW=" << std::setw(2) << psw << " "
            << "%SP:" << std::setw(2) << +reg_SP_ << " "
            << "CYC:" << std::setw(3) << std::setfill(' ') << std::dec
            << ((cycles_ - 1) * 3) % 341 << " cyc:" << cycles_ << std::endl;
  Disassembler disass(bus_);
  Address addr = old_PC_ & 0xff00;
  for (Address cnt = 0; addr <= ((old_PC_ & 0xff00) | 0xff) && cnt < 256;
       cnt++) {
    disass.OneInstr(addr, addr == old_PC_);
    addr = disass.pc();
  }

  addr = reg_PC_ & 0xff00;
  LOG(INFO) << "current: " << std::hex << addr;
  for (Address cnt = 0; addr <= ((reg_PC_ & 0xff00) | 0xff) && cnt < 256;
       cnt++) {
    disass.OneInstr(addr, addr == reg_PC_);
    addr = disass.pc();
  }

  addr = readAddress(IRQVector);
  LOG(INFO) << "IRQ: " << std::hex << addr;
  for (Address cnt = 0; cnt < 64; cnt++) {
    disass.OneInstr(addr, addr == old_PC_);
    addr = disass.pc();
  }

  addr = readAddress(NMIVector);
  LOG(INFO) << "NMI: " << std::hex << addr;
  for (Address cnt = 0; cnt < 64; cnt++) {
    disass.OneInstr(addr, addr == old_PC_);
    addr = disass.pc();
  }

  LOG(INFO) << "Stack:";
  for (Address sp = 0x1fd; sp > (0x100 + reg_SP_); sp--) {
    LOG(INFO) << std::hex << std::setfill('0') << std::setw(4) << sp << "["
              << std::setw(2) << (0x1fd - sp) << "] " << std::setw(2)
              << +bus_.read(sp);
  }
}

};  // namespace hn
