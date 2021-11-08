#pragma once

#include "CPUOpcodes.h"
#include "MainBus.h"
#include "common.h"

namespace hn {

class Disassembler {
 public:
  Disassembler(MainBus& bus);

  void Step();
  bool OneInstr(Address pc, bool star = false);
  void setPC(Address pc) { PC_ = pc; }
  Address pc() const { return PC_; }

 protected:
  bool explainImplied(Byte opcode, char* buffer);
  bool explainType0(Byte opcode, char* buffer);
  bool explainType1(Byte opcode, char* buffer);
  bool explainType2(Byte opcode, char* buffer);
  bool explainBranch(Byte opcode, char* buffer);

 private:
  Address PC_;
  MainBus& bus_;
};

}  // namespace hn
