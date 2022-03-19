#include "Disassembler.h"

#include <iomanip>

#include "glog/logging.h"

namespace hn {

Disassembler::Disassembler(MainBus& bus) : bus_(bus) {}

void Disassembler::Step() {
  char buffer[1024];
  disassemble(false, buffer);
}

bool Disassembler::OneInstr(Address pc, bool star) {
  setPC(pc);
  char buffer[1024];

  if (disassemble(star, buffer)) {
    LOG(INFO) << buffer;
    return true;
  }

  return false;
}

void Disassembler::DisassembleOnePage(Address addr, Address pc, int limit) {
  char buffer[1024];
  PC_ = addr;
  for (Address end = (PC_ & 0xff00) + 0x100;
       (PC_ < end || (!end && PC_ > 0x100)) && limit > 0; limit--) {
    disassemble(PC_ == pc, buffer);
    LOG(INFO) << buffer;
  }
}

bool Disassembler::disassemble(bool star, char* buffer) {
  Address pc = PC_;
  char* buf = buffer +
              sprintf(buffer, "%c 0x%04x            ", (star ? '*' : ' '), PC_);
  Byte opcode = bus_.read(PC_++);
  // Using short-circuit evaluation, call the other function only if the first
  // failed ExecuteImplied must be called first and ExecuteBranch must be before
  // ExecuteType0
  if ((explainImplied(opcode, buf) || explainBranch(opcode, buf) ||
       explainType1(opcode, buf) || explainType2(opcode, buf) ||
       explainType0(opcode, buf))) {
    // Output the raw byte of the opcode
    buf = &buffer[10];
    for (int n = 0; pc < PC_ && n < 3; pc++) {
      buf += sprintf(buf, "%02x ", bus_.read(pc));
    }
    *buf = ' ';

    return true;
  }

  sprintf(buf, "Unknown Code");
  sprintf(&buffer[10], "%02x", opcode);
  buffer[12] = ' ';

  return false;
}

bool Disassembler::explainImplied(Byte opcode, char* buffer) {
#define CASE(op)                    \
  case op:                          \
    buffer += sprintf(buffer, #op); \
    break

  switch (static_cast<OperationImplied>(opcode)) {
    case JMP:
      buffer += sprintf(buffer, "JMP 0x%04x", bus_.readAddress(PC_));
      PC_ += 2;
      break;
    case JMPI:
      buffer += sprintf(buffer, "JMPI [0x%04x]", bus_.readAddress(PC_));
      PC_ += 2;
      break;
    case JSR:
      buffer += sprintf(buffer, "JSR 0x%04x", bus_.readAddress(PC_));
      PC_ += 2;
      break;

      CASE(BRK);
      CASE(CLC);
      CASE(CLD);
      CASE(CLI);
      CASE(CLV);
      CASE(DEX);
      CASE(DEY);
      CASE(INX);
      CASE(INY);
      CASE(NOP);
      CASE(PHA);
      CASE(PHP);
      CASE(PLA);
      CASE(PLP);
      CASE(RTI);
      CASE(RTS);
      CASE(SEC);
      CASE(SED);
      CASE(SEI);
      CASE(TAX);
      CASE(TAY);
      CASE(TSX);
      CASE(TXA);
      CASE(TXS);
      CASE(TYA);
    default:
      return false;
  }
#undef CASE
  return true;
}

bool Disassembler::explainBranch(Byte opcode, char* buffer) {
  if (OPNOTBRANCH(opcode)) {
    return false;
  }

  *buffer++ = 'J';
  // branch is initialized to the condition required (for the flag specified
  // later)
  if (opcode & BranchConditionMask) {
    *buffer++ = 'N';
  }

  // set branch to true if the given condition is met by the given flag
  // We use xnor here, it is true if either both operands are true or false
  *buffer++ = "LOCZ"[opcode >> BranchOnFlagShift];
  *buffer++ = ' ';

  int8_t offset = bus_.read(PC_++);
  sprintf(buffer, "0x%04x", PC_ + offset);

  return true;
}

bool Disassembler::explainType1(Byte opcode, char* buffer) {
  if (OPNOTTYPE(opcode, 0x1)) {
    return false;
  }

#define CASE(op)                           \
  case op:                                 \
    buffer += sprintf(buffer, "%s ", #op); \
    break
  auto op = OPACTION(opcode, 1);
  switch (op) {
    CASE(ADC);
    CASE(AND);
    CASE(CMP);
    CASE(EOR);
    CASE(LDA);
    CASE(ORA);
    CASE(SBC);
    CASE(STA);
    default:
      return false;
  }
#undef CASE

  Address location = 0;  // Location of the operand, could be in RAM
  switch (OPADDRMODE(opcode, 1)) {
    case IndexedIndirectX:
      sprintf(buffer, "[ZeroPage[%%X+%02x]]", bus_.read(PC_++));
      break;
    case ZeroPage:
      sprintf(buffer, "ZeroPage[%02x]", bus_.read(PC_++));
      break;
    case Immediate:
      sprintf(buffer, "0x%02x", bus_.read(PC_++));
      break;
    case Absolute:
      sprintf(buffer, "[0x%04x]", bus_.readAddress(PC_));
      PC_ += 2;
      break;
    case IndirectY:
      sprintf(buffer, "[%%Y+[0x%02x]]", bus_.read(PC_++));
      break;
    case IndexedX:
      // Address wraps around in the zero page
      sprintf(buffer, "[%%X+0x%02x]", bus_.read(PC_++));
      break;
    case AbsoluteY:
      sprintf(buffer, "[%%Y+0x%04x]", bus_.readAddress(PC_));
      PC_ += 2;
      break;
    case AbsoluteX:
      sprintf(buffer, "[%%X+0x%04x]", bus_.readAddress(PC_));
      PC_ += 2;
      break;
    default:
      return false;
  }

  return true;
}

bool Disassembler::explainType2(Byte opcode, char* buffer) {
  if (OPNOTTYPE(opcode, 0x2)) {
    return false;
  }

  Address location = 0;
  auto op = OPACTION(opcode, 2);
#define CASE(op)                           \
  case op:                                 \
    buffer += sprintf(buffer, "%s ", #op); \
    break
  std::uint16_t operand = 0;
  switch (op) {
    CASE(ASL);
    CASE(DEC);
    CASE(INC);
    CASE(LDX);
    CASE(LSR);
    CASE(ROL);
    CASE(ROR);
    CASE(STX);
    default:
      LOG(ERROR) << "Unrecognized opcode:" << int(op);
      return false;
  }
#undef CASE

  auto addr_mode = OPADDRMODE(opcode, 2);
  switch (addr_mode) {
    case Immediate_:
      sprintf(buffer, "0x%02x", bus_.read(PC_++));
      break;
    case ZeroPage_:
      sprintf(buffer, "[0x%02x]", bus_.read(PC_++));
      break;
    case Accumulator:
      sprintf(buffer, "%%A");
      break;
    case Absolute_:
      sprintf(buffer, "[0x%04x]", bus_.readAddress(PC_));
      PC_ += 2;
      break;
    case Indexed:
      sprintf(buffer, "[0x%02x+%%%c]", bus_.read(PC_++),
              (op == LDX || op == STX ? 'Y' : 'X'));
      break;
    case AbsoluteIndexed:
      sprintf(buffer, "[0x%02x+%%%c]", bus_.readAddress(PC_),
              (op == LDX || op == STX ? 'Y' : 'X'));
      PC_ += 2;
      break;
    default:
      return false;
  }

  return true;
}

bool Disassembler::explainType0(Byte opcode, char* buffer) {
  if (OPNOTTYPE(opcode, 0x0)) {
    return false;
  }
#define CASE(op)                           \
  case op:                                 \
    buffer += sprintf(buffer, "%s ", #op); \
    break
  std::uint16_t operand = 0;
  switch (OPACTION(opcode, 0)) {
    CASE(BIT);
    CASE(CPX);
    CASE(CPY);
    CASE(LDY);
    CASE(STY);
    default:
      return false;
  }
#undef CASE

  Address location = 0;
  switch (OPADDRMODE(opcode, 2)) {
    case Immediate_:
      // Imm
      sprintf(buffer, "0x%02x", bus_.read(PC_++));
      // location = PC_++;
      break;
    case ZeroPage_:
      // [PC]
      sprintf(buffer, "[0x%02x]", bus_.read(PC_++));
      // location = bus_.read(PC_++);
      break;
    case Absolute_:
      // [:PC]
      sprintf(buffer, "[0x%04x]", bus_.readAddress(PC_));
      PC_ += 2;
      break;
    case Indexed:
      // Address wraps around in the zero page
      sprintf(buffer, "[0x%02x+X]", bus_.read(PC_++));
      break;
    case AbsoluteIndexed:
      sprintf(buffer, "[0x%04x+X]", bus_.readAddress(PC_));
      PC_ += 2;
      break;
    default:
      return false;
  }

  return true;
}

};  // namespace hn
