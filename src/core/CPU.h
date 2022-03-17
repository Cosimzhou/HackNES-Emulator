#pragma once
#include "CPUOpcodes.h"
#include "MainBus.h"

#define PSW_IN_BYTE

namespace hn {

class CPU : public Serialize {
 public:
  CPU(MainBus& mem);

  void Step();
  void Reset();
  void Reset(Address start_addr);

  void skipDMACycles();

  void ClearIRQ() { CLR_BIT(irq_flag_, IT_IRQ); }
  void TryIRQ() { SET_BIT(irq_flag_, IT_IRQ); }
  void TryNMI() { SET_BIT(irq_flag_, IT_NMI); }

  size_t clock_cycles() const { return cycles_; }

  void DebugDump();

  virtual void Save(std::ostream& os) override;
  virtual void Restore(std::istream& is) override;

 private:
  // Assuming sequential execution, for asynchronously calling this with
  // Execute, further work needed
  void interrupt(InterruptType type);

  // Instructions are split into five sets to make decoding easier.
  // These functions return true if they succeed
  bool executeImplied(Byte opcode);
  bool executeBranch(Byte opcode);
  bool executeType0(Byte opcode);
  bool executeType1(Byte opcode);
  bool executeType2(Byte opcode);

  Address readAddress(Address addr);

  void pushStack(Byte value);
  Byte popStack();

  // If a and b are in different pages, increases the SkipCycles_ by inc
  void setPageCrossed(Address a, Address b, int inc = 1);
  void setZN(Byte value);

#ifdef PSW_IN_BYTE
  inline void setFlag(bool val, StatusFlag flag) {
    Byte bflag = static_cast<Byte>(flag);
    if (val) {
      SET_BIT(psw_, bflag);
    } else {
      CLR_BIT(psw_, bflag);
    }
  }

  inline bool getFlag(StatusFlag flag) const {
    return (psw_ & static_cast<Byte>(flag)) != 0;
  }
#endif  // PSW_IN_BYTE

  int skipCycles_;
  int cycles_;

  // Registers
  Address reg_PC_;
  Address old_PC_;
  Byte reg_SP_;
  Byte reg_A_;
  Byte reg_X_;
  Byte reg_Y_;

  // Status flags.
#ifdef PSW_IN_BYTE
  Byte psw_;
#else   // PSW_IN_BYTE
  // Is storing them in one byte better?
  bool flag_C_;
  bool flag_Z_;
  bool flag_I_;
  // bool flag_B_;
  bool flag_D_;
  bool flag_V_;
  bool flag_N_;
#endif  // PSW_IN_BYTE

  Byte irq_flag_;

  MainBus& bus_;
};

}  // namespace hn
