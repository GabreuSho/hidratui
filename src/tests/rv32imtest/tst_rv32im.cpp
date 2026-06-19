// RV32IM Machine Tests
#include <QCoreApplication>
#include <QString>
#include <iostream>
#include <iomanip>
#include "machines/rv32immachine.h"
#include "tui/machine_config.h"

static void runSteps(RV32IMMachine &machine, int maxSteps = 30) {
    machine.setRunning(true);
    for (int i = 0; i < maxSteps && machine.isRunning(); i++)
        machine.step();
}

static void printInstrs(RV32IMMachine &machine, int count, int start = RV32IMMachine::TEXT_BASE) {
    for (int i = 0; i < count; i++) {
        int addr = start + i * 4;
        QString instr = machine.getInstructionString(addr);
        if (!instr.isEmpty())
            std::cout << "  0x" << std::hex << addr << std::dec << ": " << instr.toStdString() << std::endl;
    }
}

//////////////////////////////////////////////////
// Test 1: Basic — li + ecall
//////////////////////////////////////////////////
static bool test_basic() {
    std::cout << "\n=== Test 1: Basic (li + ecall) ===" << std::endl;
    RV32IMMachine m;
    m.assemble("li t0, 42\necall\n");
    if (!m.getBuildSuccessful()) { std::cout << " FAIL: build\n"; return false; }
    m.updateInstructionStrings(); printInstrs(m, 4);
    runSteps(m);
    int v = m.getRegisterValueByName("t0");
    if (v != 42) { std::cout << " FAIL: t0=" << v << " expected 42\n"; return false; }
    std::cout << " PASS" << std::endl; return true;
}

//////////////////////////////////////////////////
// Test 2: Store/Load
//////////////////////////////////////////////////
static bool test_store_load() {
    std::cout << "\n=== Test 2: Store/Load (sw, lw) ===" << std::endl;
    RV32IMMachine m;
    m.assemble("li t0, 100\naddi sp, x0, 256\nsw t0, 0(sp)\nlw t1, 0(sp)\necall\n");
    if (!m.getBuildSuccessful()) { std::cout << " FAIL: build\n"; return false; }
    m.updateInstructionStrings(); printInstrs(m, 6);
    runSteps(m);
    int v = m.getRegisterValueByName("t1");
    if (v != 100) { std::cout << " FAIL: t1=" << v << " expected 100\n"; return false; }
    std::cout << " PASS" << std::endl; return true;
}

//////////////////////////////////////////////////
// Test 3: Branch (beq)
//////////////////////////////////////////////////
static bool test_branch() {
    std::cout << "\n=== Test 3: Branch (beq) ===" << std::endl;
    RV32IMMachine m;
    QString code = R"(
li t0, 5
li t1, 5
beq t0, t1, equal
li t2, 0
j end
equal:
li t2, 1
end:
ecall
)";
    m.assemble(code);
    if (!m.getBuildSuccessful()) { std::cout << " FAIL: build\n"; return false; }
    m.updateInstructionStrings(); printInstrs(m, 8);
    runSteps(m);
    int v = m.getRegisterValueByName("t2");
    if (v != 1) { std::cout << " FAIL: t2=" << v << " expected 1\n"; return false; }
    std::cout << " PASS" << std::endl; return true;
}

//////////////////////////////////////////////////
// Test 4: Arithmetic (add, sub, mul)
//////////////////////////////////////////////////
static bool test_arithmetic() {
    std::cout << "\n=== Test 4: Arithmetic ===" << std::endl;
    RV32IMMachine m;
    QString code = R"(
li t0, 10
li t1, 3
add t2, t0, t1
sub t3, t0, t1
mul t4, t0, t1
ecall
)";
    m.assemble(code);
    if (!m.getBuildSuccessful()) { std::cout << " FAIL: build\n"; return false; }
    runSteps(m);
    if (m.getRegisterValueByName("t2") != 13) { std::cout << " FAIL: add\n"; return false; }
    if (m.getRegisterValueByName("t3") != 7) { std::cout << " FAIL: sub\n"; return false; }
    if (m.getRegisterValueByName("t4") != 30) { std::cout << " FAIL: mul\n"; return false; }
    std::cout << " PASS" << std::endl; return true;
}

//////////////////////////////////////////////////
// Test 5: JAL/JALR
//////////////////////////////////////////////////
static bool test_jal_jalr() {
    std::cout << "\n=== Test 5: JAL/JALR ===" << std::endl;
    RV32IMMachine m;
    QString code = R"(
jal ra, subfunc
ecall
subfunc:
li t0, 99
jalr x0, 0(ra)
)";
    m.assemble(code);
    if (!m.getBuildSuccessful()) { std::cout << " FAIL: build\n"; return false; }
    runSteps(m);
    if (m.getRegisterValueByName("t0") != 99) { std::cout << " FAIL: t0=" << m.getRegisterValueByName("t0") << "\n"; return false; }
    std::cout << " PASS" << std::endl; return true;
}

//////////////////////////////////////////////////
// Test 6: LUI + ADDI (large immediate)
//////////////////////////////////////////////////
static bool test_lui_addi() {
    std::cout << "\n=== Test 6: LUI+ADDI (large imm) ===" << std::endl;
    RV32IMMachine m;
    m.assemble("li t0, 0x12345\necall\n");
    if (!m.getBuildSuccessful()) { std::cout << " FAIL: build\n"; return false; }
    runSteps(m);
    if (m.getRegisterValueByName("t0") != 0x12345) { std::cout << " FAIL: t0=0x" << std::hex << m.getRegisterValueByName("t0") << std::dec << "\n"; return false; }
    std::cout << " PASS" << std::endl; return true;
}

//////////////////////////////////////////////////
// Test 7: Sparse memory
//////////////////////////////////////////////////
static bool test_sparse_memory() {
    std::cout << "\n=== Test 7: Sparse Memory ===" << std::endl;
    RV32IMMachine m;
    m.setMemoryValue(0x10000, 0xAB);
    m.setMemoryValue(0x20000, 0xCD);
    m.setMemoryValue(0x0000, 0x01);
    if (m.getMemoryValue(0x10000) != 0xAB) { std::cout << " FAIL: high addr\n"; return false; }
    if (m.getMemoryValue(0x30000) != 0) { std::cout << " FAIL: uninit\n"; return false; }
    m.setMemoryValue(0x10000, 0);
    if (m.getMemoryValue(0x10000) != 0) { std::cout << " FAIL: zeroed\n"; return false; }
    std::cout << " PASS" << std::endl; return true;
}

//////////////////////////////////////////////////
// Test 8: Register names (ABI + numeric)
//////////////////////////////////////////////////
static bool test_register_names() {
    std::cout << "\n=== Test 8: Register Names ===" << std::endl;
    RV32IMMachine m;
    m.setRegisterValue(QString("sp"), 0x100);
    if (m.getRegisterValue(QString("x2")) != 0x100) { std::cout << " FAIL: x2!=sp\n"; return false; }
    m.setRegisterValue(QString("x0"), 123);
    if (m.getRegisterValue(QString("zero")) != 0) { std::cout << " FAIL: x0!=0\n"; return false; }
    std::cout << " PASS" << std::endl; return true;
}

//////////////////////////////////////////////////
// Test 9: Pseudo-instructions
//////////////////////////////////////////////////
static bool test_pseudo() {
    std::cout << "\n=== Test 9: Pseudo-instructions ===" << std::endl;
    RV32IMMachine m;
    QString code = R"(
li t0, 42
mv t1, t0
nop
not t2, t0
neg t3, t0
j end
li t0, 999
end:
ecall
)";
    m.assemble(code);
    if (!m.getBuildSuccessful()) { std::cout << " FAIL: build\n"; return false; }
    runSteps(m);
    if (m.getRegisterValueByName("t1") != 42) { std::cout << " FAIL: mv\n"; return false; }
    if (m.getRegisterValueByName("t2") != ~42) { std::cout << " FAIL: not\n"; return false; }
    if (m.getRegisterValueByName("t3") != -42) { std::cout << " FAIL: neg\n"; return false; }
    if (m.getRegisterValueByName("t0") == 999) { std::cout << " FAIL: j not taken\n"; return false; }
    std::cout << " PASS" << std::endl; return true;
}

//////////////////////////////////////////////////
// Test 10: Shifts (SLLI, SRLI, SRAI)
//////////////////////////////////////////////////
static bool test_shifts() {
    std::cout << "\n=== Test 10: Shifts ===" << std::endl;
    RV32IMMachine m;
    QString code = R"(
li t0, 8
slli t1, t0, 2
srli t2, t1, 1
addi t3, x0, -8
srai t4, t3, 2
ecall
)";
    m.assemble(code);
    if (!m.getBuildSuccessful()) { std::cout << " FAIL: build\n"; return false; }
    runSteps(m);
    if (m.getRegisterValueByName("t1") != 32) { std::cout << " FAIL: slli=" << m.getRegisterValueByName("t1") << "\n"; return false; }
    if (m.getRegisterValueByName("t2") != 16) { std::cout << " FAIL: srli=" << m.getRegisterValueByName("t2") << "\n"; return false; }
    int t4 = m.getRegisterValueByName("t4");
    // SRAI of -8 >> 2 should be -2
    if (t4 != -2) { std::cout << " FAIL: srai=" << t4 << " expected -2\n"; return false; }
    std::cout << " PASS" << std::endl; return true;
}

//////////////////////////////////////////////////
// Test 11: Branch with large li (8-byte pseudo)
//////////////////////////////////////////////////
static bool test_branch_large_li() {
    std::cout << "\n=== Test 11: Branch with large li ===" << std::endl;
    RV32IMMachine m;
    QString code = R"(
li t0, 100000
li t1, 5
beq t0, t1, equal
li t2, 0
j end
equal:
li t2, 1
end:
ecall
)";
    m.assemble(code);
    if (!m.getBuildSuccessful()) { std::cout << " FAIL: build\n"; return false; }
    m.updateInstructionStrings(); printInstrs(m, 12);
    runSteps(m);
    int v = m.getRegisterValueByName("t2");
    // t0=100000 != t1=5, so branch NOT taken, t2=0
    if (v != 0) { std::cout << " FAIL: t2=" << v << " expected 0 (branch not taken)\n"; return false; }
    std::cout << " PASS" << std::endl; return true;
}

//////////////////////////////////////////////////
// Test 12: DIV/REM
//////////////////////////////////////////////////
static bool test_div_rem() {
    std::cout << "\n=== Test 12: DIV/REM ===" << std::endl;
    RV32IMMachine m;
    QString code = R"(
li t0, 7
li t1, 3
div t2, t0, t1
rem t3, t0, t1
ecall
)";
    m.assemble(code);
    if (!m.getBuildSuccessful()) { std::cout << " FAIL: build\n"; return false; }
    runSteps(m);
    if (m.getRegisterValueByName("t2") != 2) { std::cout << " FAIL: div\n"; return false; }
    if (m.getRegisterValueByName("t3") != 1) { std::cout << " FAIL: rem\n"; return false; }
    std::cout << " PASS" << std::endl; return true;
}

//////////////////////////////////////////////////
// Test 13: Virtual dispatch via Machine* — simulate registers_panel behavior
//////////////////////////////////////////////////
static bool test_virtual_dispatch_via_base() {
    std::cout << "\n=== Test 13: Virtual dispatch (registers_panel simulation) ===" << std::endl;
    RV32IMMachine rv;
    Machine* m = &rv;  // TUI uses Machine*

    // Simulate exactly what RegistersPanel::render_all_registers() does
    // It uses MachineConfigProvider::getConfig("rv32im")
    // But we'll test the critical path: getRegisterValue for each name
    
    std::vector<std::string> reg_names = {
        "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0/fp", "s1",
        "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
        "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11",
        "t3", "t4", "t5", "t6", "PC"
    };
    
    int shown = 0;
    int exceptions = 0;
    for (const auto& name : reg_names) {
        try {
            int val = m->getRegisterValue(QString::fromStdString(name));
            (void)val;
            shown++;
        } catch (...) {
            std::cout << "  EXCEPTION for: " << name << std::endl;
            exceptions++;
        }
    }
    
    std::cout << "  shown=" << shown << " exceptions=" << exceptions << " total=" << reg_names.size() << std::endl;
    
    if (exceptions > 0) {
        std::cout << " FAIL: " << exceptions << " register names threw exceptions" << std::endl;
        return false;
    }
    if (shown != (int)reg_names.size()) {
        std::cout << " FAIL: only " << shown << "/" << reg_names.size() << " shown" << std::endl;
        return false;
    }
    
    // Also test hasRegister for common 8-bit names (fallback path)
    if (m->hasRegister("ACC")) { std::cout << " FAIL: hasRegister(ACC)=true\n"; return false; }
    if (!m->hasRegister("PC")) { std::cout << " FAIL: hasRegister(PC)=false\n"; return false; }
    if (!m->hasRegister("SP")) { std::cout << " FAIL: hasRegister(SP)=false\n"; return false; }
    
    // Verify step() dispatch (PC increments by 4)
    m->assemble("li t0, 42\necall\n");
    m->updateInstructionStrings();
    m->setRunning(true);
    m->step();
    int pc = m->getPCValue();
        if (pc != RV32IMMachine::TEXT_BASE + 4) {
    std::cout << " FAIL: PC=" << pc << " expected TEXT_BASE+4 after step" << std::endl;
    return false;
  }
  std::cout << " PASS" << std::endl;
  return true;
}

//////////////////////////////////////////////////
// Test 14: MachineConfig for RV32IM
//////////////////////////////////////////////////
static bool test_machine_config_rv32im() {
    std::cout << "\n=== Test 14: MachineConfig RV32IM ===" << std::endl;
    
    // Test exact same call as RegistersPanel::get_machine_config()
    hidra::tui::MachineConfig config = hidra::tui::MachineConfigProvider::getConfig("rv32im");
    
    std::cout << "  config.name=" << config.name << std::endl;
    std::cout << "  config.registers.size()=" << config.registers.size() << std::endl;
    std::cout << "  config.flags.size()=" << config.flags.size() << std::endl;
    
    if (config.registers.empty()) {
        std::cout << " FAIL: registers is EMPTY — this causes TUI to fall back to 8-bit common_regs!" << std::endl;
        return false;
    }
    
    if (config.registers.size() != 33) {
        std::cout << " FAIL: expected 33 registers, got " << config.registers.size() << std::endl;
        return false;
    }
    
    if (config.name != "RV32IM") {
        std::cout << " FAIL: name=" << config.name << " expected RV32IM" << std::endl;
        return false;
    }
    
    // Also test "rvm" alias
    hidra::tui::MachineConfig config2 = hidra::tui::MachineConfigProvider::getConfig("rvm");
    if (config2.registers.size() != 33) {
        std::cout << " FAIL: 'rvm' alias gives " << config2.registers.size() << " registers" << std::endl;
        return false;
    }
    
    std::cout << " PASS" << std::endl;
    return true;
}

//////////////////////////////////////////////////
// Test 15: .text / .data sections
//////////////////////////////////////////////////
static bool test_text_data_sections()
{
  std::cout << "\n=== Test 15: .text / .data sections ===" << std::endl;

  // Code without explicit .text should still go to TEXT_BASE
  {
    RV32IMMachine m;
    m.assemble("li t0, 42\necall\n");
    int pc = m.getPCValue();
    if (pc != RV32IMMachine::TEXT_BASE) {
      std::cout << " FAIL: PC=0x" << std::hex << pc << " expected 0x" << RV32IMMachine::TEXT_BASE << std::dec << std::endl;
      return false;
    }
    // addi t0, x0, 42 -> byte0 = rd[1:0]|opcode = 0x93
    int b0 = m.getMemoryValue(RV32IMMachine::TEXT_BASE);
    if (b0 != 0x93) {
      std::cout << " FAIL: byte at TEXT_BASE=0x" << std::hex << b0 << " expected 0x93" << std::dec << std::endl;
      return false;
    }
  }

  // .text and .data with explicit directives
  {
    RV32IMMachine m;
    m.assemble(".text\nli t0, 42\necall\n.data\n.word 0xDEADBEEF");
    if (!m.getBuildSuccessful()) {
      std::cout << " FAIL: build unsuccessful" << std::endl;
      return false;
    }
    int pc = m.getPCValue();
    if (pc != RV32IMMachine::TEXT_BASE) {
      std::cout << " FAIL: PC=0x" << std::hex << pc << " expected 0x" << RV32IMMachine::TEXT_BASE << std::dec << std::endl;
      return false;
    }
    int b0 = m.getMemoryValue(RV32IMMachine::TEXT_BASE);
    if (b0 != 0x93) {
      std::cout << " FAIL: byte at TEXT_BASE=0x" << std::hex << b0 << " expected 0x93" << std::dec << std::endl;
      return false;
    }
    // Data at DATA_BASE should be 0xEF (little-endian of 0xDEADBEEF)
    int db0 = m.getMemoryValue(RV32IMMachine::DATA_BASE);
    if (db0 != 0xEF) {
      std::cout << " FAIL: byte at DATA_BASE=0x" << std::hex << db0 << " expected 0xEF" << std::dec << std::endl;
      return false;
    }
  }

  // Switch between .text and .data multiple times
  {
    RV32IMMachine m;
    m.assemble(".text\nli t0, 1\n.data\n.word 100\n.text\nli t1, 2");
    if (!m.getBuildSuccessful()) {
      std::cout << " FAIL: build unsuccessful (multi-section)" << std::endl;
      return false;
    }
    int b0_second = m.getMemoryValue(RV32IMMachine::TEXT_BASE + 4);
    // li t1, 2 -> addi t1, x0, 2 -> byte0 = (6<<7)|0x13
    int expected = 0x13;  // byte0 of addi t1,x0,2: rd[0]=0, opcode=0x13
    if (b0_second != expected) {
      std::cout << " FAIL: second .text byte=0x" << std::hex << b0_second << " expected 0x" << expected << std::dec << std::endl;
      return false;
    }
  }

  std::cout << " PASS" << std::endl;
  return true;
}


//////////////////////////////////////////////////
// Test 16: SP and GP initialization
//////////////////////////////////////////////////
static bool test_sp_gp_init() {
    std::cout << "\n=== Test 16: SP/GP initialization ===" << std::endl;

    RV32IMMachine m;
    m.assemble("li t0, 42\necall\n");

    int sp = m.getRegisterValueByName("sp");
    int gp = m.getRegisterValueByName("gp");

    if (sp != RV32IMMachine::SP_INIT) {
        std::cout << " FAIL: SP=0x" << std::hex << sp << " expected 0x"
                  << RV32IMMachine::SP_INIT << std::dec << std::endl;
        return false;
    }
    if (gp != RV32IMMachine::DATA_BASE) {
        std::cout << " FAIL: GP=0x" << std::hex << gp << " expected 0x"
                  << RV32IMMachine::DATA_BASE << std::dec << std::endl;
        return false;
    }

    std::cout << " PASS" << std::endl;
    return true;
}

//////////////////////////////////////////////////
// Test 17: SP/GP/PC preserved through clearAfterBuild
//////////////////////////////////////////////////
static bool test_clear_after_build_preserves_init() {
    std::cout << "\n=== Test 17: clearAfterBuild preserves SP/GP/PC ===" << std::endl;
    RV32IMMachine m;
    m.assemble("li t0, 42\necall\n");
    if (!m.getBuildSuccessful()) {
        std::cout << " FAIL: build unsuccessful" << std::endl;
        return false;
    }
    // After assemble, SP/GP/PC should be RARS defaults
    if (m.getPCValue() != RV32IMMachine::TEXT_BASE) {
        std::cout << " FAIL: PC=0x" << std::hex << m.getPCValue()
                  << " expected 0x" << RV32IMMachine::TEXT_BASE << std::dec << std::endl;
        return false;
    }
    if (m.getRegisterValueByName("sp") != RV32IMMachine::SP_INIT) {
        std::cout << " FAIL: SP=0x" << std::hex << m.getRegisterValueByName("sp")
                  << " expected 0x" << RV32IMMachine::SP_INIT << std::dec << std::endl;
        return false;
    }
    if (m.getRegisterValueByName("gp") != RV32IMMachine::DATA_BASE) {
        std::cout << " FAIL: GP=0x" << std::hex << m.getRegisterValueByName("gp")
                  << " expected 0x" << RV32IMMachine::DATA_BASE << std::dec << std::endl;
        return false;
    }
    // clear() resets to RARS defaults (not zero, since RV32IM should never
    // have PC/SP at zero — even after a failed assemble)
    m.clear();
    if (m.getPCValue() != RV32IMMachine::TEXT_BASE) {
        std::cout << " FAIL: PC after clear=0x" << std::hex << m.getPCValue()
                  << " expected 0x" << RV32IMMachine::TEXT_BASE << std::dec << std::endl;
        return false;
    }
    if (m.getRegisterValueByName("sp") != RV32IMMachine::SP_INIT) {
        std::cout << " FAIL: SP after clear=0x" << std::hex << m.getRegisterValueByName("sp")
                  << " expected 0x" << RV32IMMachine::SP_INIT << std::dec << std::endl;
        return false;
    }
    m.assemble("li t1, 7\necall\n");
    if (m.getPCValue() != RV32IMMachine::TEXT_BASE) {
        std::cout << " FAIL: PC after re-assemble=0x" << std::hex << m.getPCValue()
                  << " expected 0x" << RV32IMMachine::TEXT_BASE << std::dec << std::endl;
        return false;
    }
    if (m.getRegisterValueByName("sp") != RV32IMMachine::SP_INIT) {
        std::cout << " FAIL: SP after re-assemble=0x" << std::hex << m.getRegisterValueByName("sp")
                  << " expected 0x" << RV32IMMachine::SP_INIT << std::dec << std::endl;
        return false;
    }
    std::cout << " PASS" << std::endl;
    return true;
}

//////////////////////////////////////////////////
// Test 18: Failed assemble preserves SP/GP/PC
//////////////////////////////////////////////////
static bool test_failed_assemble_preserves_init() {
    std::cout << "\n=== Test 18: Failed assemble preserves SP/GP/PC ===" << std::endl;
    RV32IMMachine m;
    // .xyz is not a supported directive — assemble will fail
    m.assemble(".xyz invalid\n.text\nli t0, 42\necall\n");
    if (m.getBuildSuccessful()) {
        std::cout << " FAIL: build should have failed for .xyz" << std::endl;
        return false;
    }
    // Even after failed assemble, PC/SP/GP must be RARS defaults
    if (m.getPCValue() != RV32IMMachine::TEXT_BASE) {
        std::cout << " FAIL: PC after failed assemble=0x" << std::hex << m.getPCValue()
                  << " expected 0x" << RV32IMMachine::TEXT_BASE << std::dec << std::endl;
        return false;
    }
    if (m.getRegisterValueByName("sp") != RV32IMMachine::SP_INIT) {
        std::cout << " FAIL: SP after failed assemble=0x" << std::hex << m.getRegisterValueByName("sp")
                  << " expected 0x" << RV32IMMachine::SP_INIT << std::dec << std::endl;
        return false;
    }
    if (m.getRegisterValueByName("gp") != RV32IMMachine::DATA_BASE) {
        std::cout << " FAIL: GP after failed assemble=0x" << std::hex << m.getRegisterValueByName("gp")
                  << " expected 0x" << RV32IMMachine::DATA_BASE << std::dec << std::endl;
        return false;
    }
    std::cout << " PASS" << std::endl;
    return true;
}

//////////////////////////////////////////////////
// Test 19: Space-separated args (RARS syntax)
//////////////////////////////////////////////////
static bool test_space_separated_args() {
    std::cout << "\n=== Test 19: Space-separated args ===" << std::endl;
    RV32IMMachine m;
    // Without commas — RARS-compatible
    m.assemble("li t0 42\nadd t1 t0 t2\necall\n");
    if (!m.getBuildSuccessful()) {
        std::cout << " FAIL: build unsuccessful for space-separated args" << std::endl;
        return false;
    }
    // With commas — standard
    RV32IMMachine m2;
    m2.assemble("li t0, 42\nadd t1, t0, t2\necall\n");
    if (!m2.getBuildSuccessful()) {
        std::cout << " FAIL: build unsuccessful for comma-separated args" << std::endl;
        return false;
    }
    // Mixed: "addi t1,t0 3"
    RV32IMMachine m3;
    m3.assemble("addi t1,t0 3\necall\n");
    if (!m3.getBuildSuccessful()) {
        std::cout << " FAIL: build unsuccessful for mixed separators" << std::endl;
        return false;
    }
    std::cout << " PASS" << std::endl;
    return true;
}

//////////////////////////////////////////////////
// Test 20: Labels in .data section
//////////////////////////////////////////////////
static bool test_data_labels() {
    std::cout << "\n=== Test 20: Labels in .data section ===" << std::endl;
    RV32IMMachine m;
    m.assemble(".text\nlw t0, mydata\necall\n.data\nmydata: .word 42\n");
    if (!m.getBuildSuccessful()) {
        std::cout << " FAIL: build unsuccessful" << std::endl;
        return false;
    }
    // mydata should be at DATA_BASE
    int val = m.memoryReadWord(RV32IMMachine::DATA_BASE);
    if (val != 42) {
        std::cout << " FAIL: mydata value=" << val << " expected 42" << std::endl;
        return false;
    }
    std::cout << " PASS" << std::endl;
    return true;
}

//////////////////////////////////////////////////
// Test 21: sw/lw with 3 args (RARS syntax)
//////////////////////////////////////////////////
static bool test_sw_lw_3args() {
    std::cout << "\n=== Test 21: sw/lw with 3 args ===" << std::endl;
    RV32IMMachine m;
    m.assemble(".text\nli t0, 99\nsw t0 result t1\nebreak\n.data\nresult: .word 0\n");
    if (!m.getBuildSuccessful()) {
        std::cout << " FAIL: build unsuccessful" << std::endl;
        return false;
    }
    std::cout << " PASS" << std::endl;
    return true;
}

//////////////////////////////////////////////////
// Test 22: teste_case2.s — full execution with .data label
//////////////////////////////////////////////////
static bool test_case2_execution() {
    std::cout << "\n=== Test 22: teste_case2.s full execution ===" << std::endl;
    RV32IMMachine m;
    QString code = R"(
.text
li x4 0
li x6 1
lw x5 end_data
j entry
loop: beqz x5 fim
add x6 x6 x6
entry: and x7 x5 x6
beqz x7 loop
j ehum
ehum: xor x5 x5 x6
addi x4 x4 1
j loop
fim: sw x4 end_res x8
ebreak
.data
end_data: .word 0x0000ffff
end_res: .word 0
)";
    m.assemble(code);
    if (!m.getBuildSuccessful()) {
        std::cout << " FAIL: build unsuccessful" << std::endl;
        return false;
    }
    runSteps(m, 200);
    int x4 = m.getRegisterValueByName("x4");
    int x5 = m.getRegisterValueByName("x5");
    int end_res = m.memoryReadWord(RV32IMMachine::DATA_BASE + 4);
    if (x4 != 16) {
        std::cout << " FAIL: x4=" << x4 << " expected 16" << std::endl;
        return false;
    }
    if (x5 != 0) {
        std::cout << " FAIL: x5=0x" << std::hex << x5 << " expected 0" << std::dec << std::endl;
        return false;
    }
    if (end_res != 16) {
        std::cout << " FAIL: end_res=" << end_res << " expected 16" << std::endl;
        return false;
    }
    if (m.isRunning()) {
        std::cout << " FAIL: still running" << std::endl;
        return false;
    }
    std::cout << " PASS" << std::endl;
    return true;
}

//////////////////////////////////////////////////
// Test 23: lw/sw with label — auipc expansion
//////////////////////////////////////////////////
static bool test_lw_sw_label_auipc() {
    std::cout << "\n=== Test 23: lw/sw with label (auipc expansion) ===" << std::endl;
    RV32IMMachine m;
    m.assemble(".text\nlw t0, mydata\necall\n.data\nmydata: .word 42\n");
    if (!m.getBuildSuccessful()) {
        std::cout << " FAIL: build unsuccessful" << std::endl;
        return false;
    }
    // lw t0, mydata should expand to auipc+lw (8 bytes)
    // First instruction at TEXT_BASE should be AUIPC (opcode 0x17)
    uint32_t first = (uint32_t)m.memoryReadWord(RV32IMMachine::TEXT_BASE);
    int opcode = first & 0x7F;
    if (opcode != 0x17) {
        std::cout << " FAIL: first instr opcode=0x" << std::hex << opcode << " expected 0x17 (AUIPC)" << std::dec << std::endl;
        return false;
    }
    // Second instruction should be LW (opcode 0x03)
    uint32_t second = (uint32_t)m.memoryReadWord(RV32IMMachine::TEXT_BASE + 4);
    opcode = second & 0x7F;
    if (opcode != 0x03) {
        std::cout << " FAIL: second instr opcode=0x" << std::hex << opcode << " expected 0x03 (LW)" << std::dec << std::endl;
        return false;
    }
    // Run and verify t0=42
    runSteps(m, 10);
    if (m.getRegisterValueByName("t0") != 42) {
        std::cout << " FAIL: t0=" << m.getRegisterValueByName("t0") << " expected 42" << std::endl;
        return false;
    }
    std::cout << " PASS" << std::endl;
    return true;
}

//////////////////////////////////////////////////
// Test 24: sw with label — store result in .data
//////////////////////////////////////////////////
static bool test_sw_label_store() {
    std::cout << "\n=== Test 24: sw with label (store to .data) ===" << std::endl;
    RV32IMMachine m;
    m.assemble(".text\nli t0, 777\nsw t0 result t1\nebreak\n.data\nresult: .word 0\n");
    if (!m.getBuildSuccessful()) {
        std::cout << " FAIL: build unsuccessful" << std::endl;
        return false;
    }
    runSteps(m, 20);
    int val = m.memoryReadWord(RV32IMMachine::DATA_BASE);
    if (val != 777) {
        std::cout << " FAIL: result=" << val << " expected 777" << std::endl;
        return false;
    }
    if (m.isRunning()) {
        std::cout << " FAIL: still running" << std::endl;
        return false;
    }
    std::cout << " PASS" << std::endl;
    return true;
}

//////////////////////////////////////////////////
// Test 25: All branch types (step-by-step)
//////////////////////////////////////////////////
static bool test_all_branches() {
    std::cout << "\n=== Test 25: All branch types ===" << std::endl;
    RV32IMMachine m;
    QString code = R"(
        li t0, 5
        li t1, 5
        li t2, 10
        li t3, 3
        beq t0, t1, b1
        li t4, 99
        b1: bne t0, t3, b2
        li t4, 99
        b2: blt t3, t0, b3
        li t4, 99
        b3: bge t2, t0, b4
        li t4, 99
        b4: bltu t3, t2, b5
        li t4, 99
        b5: bgeu t2, t3, b6
        li t4, 99
        b6: ecall
    )";
    m.assemble(code);
    if (!m.getBuildSuccessful()) {
        std::cout << " FAIL: build" << std::endl;
        return false;
    }
    runSteps(m, 40);
    // t4 should never be set to 99 (all branches should be taken)
    int t4 = m.getRegisterValueByName("t4");
    if (t4 == 99) {
        std::cout << " FAIL: a branch was not taken, t4=" << t4 << std::endl;
        return false;
    }
    std::cout << " PASS" << std::endl;
    return true;
}

//////////////////////////////////////////////////
// Test 26: LUI and AUIPC (step-by-step)
//////////////////////////////////////////////////
static bool test_lui_auipc() {
    std::cout << "\n=== Test 26: LUI and AUIPC ===" << std::endl;
    RV32IMMachine m;
    m.assemble("lui t0, 0x12345\nauipc t1, 0x1000\necall\n");
    if (!m.getBuildSuccessful()) {
        std::cout << " FAIL: build" << std::endl;
        return false;
    }
    // Debug: check instruction encoding
    uint32_t instr0 = m.memoryReadWord(RV32IMMachine::TEXT_BASE);
    uint32_t instr1 = m.memoryReadWord(RV32IMMachine::TEXT_BASE + 4);
    std::cout << "  DEBUG: instr@0x" << std::hex << RV32IMMachine::TEXT_BASE
              << " = 0x" << instr0 << " opcode=" << (instr0 & 0x7F) << std::dec << std::endl;
    std::cout << "  DEBUG: instr@0x" << std::hex << (RV32IMMachine::TEXT_BASE + 4)
              << " = 0x" << instr1 << " opcode=" << (instr1 & 0x7F) << std::dec << std::endl;
    runSteps(m, 10);
    int t0 = m.getRegisterValueByName("t0");
    if (t0 != 0x12345000) {
        std::cout << " FAIL: lui t0=0x" << std::hex << t0 << " expected 0x12345000" << std::dec << std::endl;
        return false;
    }
    int t1 = m.getRegisterValueByName("t1");
    // auipc t1, 0x1000: PC + (0x1000 << 12) = address_of_auipc + 0x1000000
    // AUIPC is at TEXT_BASE + 4 (after LUI)
    int expected_t1 = RV32IMMachine::TEXT_BASE + 4 + 0x1000000;
    std::cout << "  DEBUG: t1=0x" << std::hex << t1 << " expected=0x" << expected_t1 << std::dec << std::endl;
    if (t1 != expected_t1) {
        std::cout << " FAIL: auipc t1=0x" << std::hex << t1 << " expected 0x" << expected_t1 << std::dec << std::endl;
        return false;
    }
    std::cout << " PASS" << std::endl;
    return true;
}

//////////////////////////////////////////////////
// Test 27: M extension (mulh, mulhsu, mulhu, divu, remu)
//////////////////////////////////////////////////
static bool test_m_extension_full() {
    std::cout << "\n=== Test 27: M extension full ===" << std::endl;
    RV32IMMachine m;
    QString code = R"(
        li t0, 7
        li t1, 3
        mul t2, t0, t1
        div t3, t0, t1
        rem t4, t0, t1
        divu t5, t0, t1
        remu t6, t0, t1
        ecall
    )";
    m.assemble(code);
    if (!m.getBuildSuccessful()) {
        std::cout << " FAIL: build" << std::endl;
        return false;
    }
    runSteps(m, 30);
    if (m.getRegisterValueByName("t2") != 21) { std::cout << " FAIL: mul" << std::endl; return false; }
    if (m.getRegisterValueByName("t3") != 2) { std::cout << " FAIL: div" << std::endl; return false; }
    if (m.getRegisterValueByName("t4") != 1) { std::cout << " FAIL: rem" << std::endl; return false; }
    if (m.getRegisterValueByName("t5") != 2) { std::cout << " FAIL: divu" << std::endl; return false; }
    if (m.getRegisterValueByName("t6") != 1) { std::cout << " FAIL: remu" << std::endl; return false; }
    std::cout << " PASS" << std::endl;
    return true;
}

//////////////////////////////////////////////////
// Test 28: Logical and shift instructions (step-by-step)
//////////////////////////////////////////////////
static bool test_logical_shift() {
    std::cout << "\n=== Test 28: Logical and shift ===" << std::endl;
    RV32IMMachine m;
    QString code = R"(
        li t0, 0xFF
        li t1, 0x0F
        and t2, t0, t1
        or t3, t0, t1
        xor t4, t0, t1
        sll t5, t1, t1
        srl t6, t0, t1
        ecall
    )";
    m.assemble(code);
    if (!m.getBuildSuccessful()) {
        std::cout << " FAIL: build" << std::endl;
        return false;
    }
    runSteps(m, 30);
    if (m.getRegisterValueByName("t2") != 0x0F) { std::cout << " FAIL: and=" << m.getRegisterValueByName("t2") << std::endl; return false; }
    if (m.getRegisterValueByName("t3") != 0xFF) { std::cout << " FAIL: or=" << m.getRegisterValueByName("t3") << std::endl; return false; }
    if (m.getRegisterValueByName("t4") != 0xF0) { std::cout << " FAIL: xor=" << m.getRegisterValueByName("t4") << std::endl; return false; }
    if (m.getRegisterValueByName("t5") != (0x0F << 0x0F)) { std::cout << " FAIL: sll" << std::endl; return false; }
    if ((uint32_t)m.getRegisterValueByName("t6") != (0xFFu >> 0x0F)) { std::cout << " FAIL: srl" << std::endl; return false; }
    std::cout << " PASS" << std::endl;
    return true;
}

//////////////////////////////////////////////////
// Test 29: SLT/SLTU/SLTI/SLTIU
//////////////////////////////////////////////////
static bool test_set_instructions() {
    std::cout << "\n=== Test 29: SLT/SLTU/SLTI/SLTIU ===" << std::endl;
    RV32IMMachine m;
    QString code = R"(
        li t0, -1
        li t1, 1
        slt t2, t0, t1
        sltu t3, t0, t1
        slti t4, t0, 0
        sltiu t5, t0, 1
        ecall
    )";
    m.assemble(code);
    if (!m.getBuildSuccessful()) {
        std::cout << " FAIL: build" << std::endl;
        return false;
    }
    runSteps(m, 30);
    // -1 < 1 (signed) → t2=1
    if (m.getRegisterValueByName("t2") != 1) { std::cout << " FAIL: slt=" << m.getRegisterValueByName("t2") << std::endl; return false; }
    // 0xFFFFFFFF > 1 (unsigned) → t3=0
    if (m.getRegisterValueByName("t3") != 0) { std::cout << " FAIL: sltu=" << m.getRegisterValueByName("t3") << std::endl; return false; }
    // -1 < 0 (signed) → t4=1
    if (m.getRegisterValueByName("t4") != 1) { std::cout << " FAIL: slti=" << m.getRegisterValueByName("t4") << std::endl; return false; }
    // -1 as unsigned < 1 as unsigned? 0xFFFFFFFF < 1 → false → t5=0
    if (m.getRegisterValueByName("t5") != 0) { std::cout << " FAIL: sltiu=" << m.getRegisterValueByName("t5") << std::endl; return false; }
    std::cout << " PASS" << std::endl;
    return true;
}

//////////////////////////////////////////////////
// Test 30: Load/store byte and half
//////////////////////////////////////////////////
static bool test_load_store_byte_half() {
    std::cout << "\n=== Test 30: Load/store byte and half ===" << std::endl;
    RV32IMMachine m;
    QString code = R"(
        li t0, 0x41
        li t1, 0x1234
        addi sp, x0, 256
        sb t0, 0(sp)
        sh t1, 2(sp)
        lb t2, 0(sp)
        lh t3, 2(sp)
        lbu t4, 0(sp)
        lhu t5, 2(sp)
        ecall
    )";
    m.assemble(code);
    if (!m.getBuildSuccessful()) {
        std::cout << " FAIL: build" << std::endl;
        return false;
    }
    runSteps(m, 30);
    if (m.getRegisterValueByName("t2") != 0x41) { std::cout << " FAIL: lb=" << m.getRegisterValueByName("t2") << std::endl; return false; }
    if (m.getRegisterValueByName("t3") != 0x1234) { std::cout << " FAIL: lh=" << m.getRegisterValueByName("t3") << std::endl; return false; }
    if (m.getRegisterValueByName("t4") != 0x41) { std::cout << " FAIL: lbu=" << m.getRegisterValueByName("t4") << std::endl; return false; }
    if (m.getRegisterValueByName("t5") != 0x1234) { std::cout << " FAIL: lhu=" << m.getRegisterValueByName("t5") << std::endl; return false; }
    std::cout << " PASS" << std::endl;
    return true;
}

//////////////////////////////////////////////////
// Test 31: .eqv (equate) directive
//////////////////////////////////////////////////
static bool test_eqv_equate() {
    std::cout << "\n=== Test 31: .eqv (equate) ===" << std::endl;
    RV32IMMachine m;

    QString code = R"(
.eqv CONST 42
.eqv HEXVAL 0x100
.text
li t0, CONST
li t1, HEXVAL
li t2, 10
add t3, t0, t1
add t3, t3, t2
ecall
)";
    m.assemble(code);
    if (!m.getBuildSuccessful()) { std::cout << " FAIL: build\n"; return false; }

    runSteps(m);
    // t0=42, t1=256, t2=10, t3=42+256+10=308
    if (m.getRegisterValueByName("t0") != 42) { std::cout << " FAIL: t0=" << m.getRegisterValueByName("t0") << " expected 42\n"; return false; }
    if (m.getRegisterValueByName("t1") != 256) { std::cout << " FAIL: t1=" << m.getRegisterValueByName("t1") << " expected 256\n"; return false; }
    if (m.getRegisterValueByName("t3") != 308) { std::cout << " FAIL: t3=" << m.getRegisterValueByName("t3") << " expected 308\n"; return false; }

    std::cout << " PASS" << std::endl;
    return true;
}

//////////////////////////////////////////////////
// Test 32: .space directive
//////////////////////////////////////////////////
static bool test_space_directive() {
    std::cout << "\n=== Test 32: .space directive ===" << std::endl;
    RV32IMMachine m;

    // .space reserves N bytes initialized to zero.
    // We verify that subsequent labels land at correct addresses.
    QString code = R"(
.text
li t0, 1
ecall
.data
buffer: .space 16
value: .word 0xdeadbeef
)";
    m.assemble(code);
    if (!m.getBuildSuccessful()) { std::cout << " FAIL: build\n"; return false; }

    // buffer should be at DATA_BASE (0x10040000)
    // value should be at DATA_BASE + 16 (0x10040010)
    int bufferAddr = m.getLabelAddress("buffer");
    int valueAddr = m.getLabelAddress("value");
    if (bufferAddr != RV32IMMachine::DATA_BASE) {
        std::cout << " FAIL: buffer addr=0x" << std::hex << bufferAddr
                  << " expected 0x" << RV32IMMachine::DATA_BASE << std::dec << std::endl;
        return false;
    }
    if (valueAddr != RV32IMMachine::DATA_BASE + 16) {
        std::cout << " FAIL: value addr=0x" << std::hex << valueAddr
                  << " expected 0x" << (RV32IMMachine::DATA_BASE + 16) << std::dec << std::endl;
        return false;
    }
    // Verify .word after .space is correct
    uint32_t wordVal = static_cast<uint32_t>(m.getMemoryValue(valueAddr)) |
                       (static_cast<uint32_t>(m.getMemoryValue(valueAddr + 1)) << 8) |
                       (static_cast<uint32_t>(m.getMemoryValue(valueAddr + 2)) << 16) |
                       (static_cast<uint32_t>(m.getMemoryValue(valueAddr + 3)) << 24);
    if (wordVal != 0xdeadbeef) {
        std::cout << " FAIL: value word=0x" << std::hex << wordVal
                  << " expected 0xdeadbeef" << std::dec << std::endl;
        return false;
    }

    std::cout << " PASS" << std::endl;
    return true;
}

//////////////////////////////////////////////////
// Test 34: .word with multiple values
//////////////////////////////////////////////////
static bool test_word_multiple_values() {
    std::cout << "\n=== Test 34: .word with multiple values ===" << std::endl;
    RV32IMMachine m;

    QString code = R"(
.data
vec: .word 2, 4, 4, 3, 6, 1, 3, 4, 64
)";
    m.assemble(code);
    if (!m.getBuildSuccessful()) { std::cout << " FAIL: build\n"; return false; }

    int vecAddr = m.getLabelAddress("vec");
    if (vecAddr != RV32IMMachine::DATA_BASE) {
        std::cout << " FAIL: vec addr=0x" << std::hex << vecAddr
                  << " expected 0x" << RV32IMMachine::DATA_BASE << std::dec << std::endl;
        return false;
    }

    uint32_t expected[] = {2, 4, 4, 3, 6, 1, 3, 4, 64};
    for (int i = 0; i < 9; i++) {
        uint32_t val = static_cast<uint32_t>(m.getMemoryValue(vecAddr + i * 4)) |
                       (static_cast<uint32_t>(m.getMemoryValue(vecAddr + i * 4 + 1)) << 8) |
                       (static_cast<uint32_t>(m.getMemoryValue(vecAddr + i * 4 + 2)) << 16) |
                       (static_cast<uint32_t>(m.getMemoryValue(vecAddr + i * 4 + 3)) << 24);
        if (val != expected[i]) {
            std::cout << " FAIL: vec[" << i << "]=0x" << std::hex << val
                      << " expected 0x" << expected[i] << std::dec << std::endl;
            return false;
        }
    }

    std::cout << " PASS" << std::endl;
    return true;
}

//////////////////////////////////////////////////
// Test 35: .half with multiple values
//////////////////////////////////////////////////
static bool test_half_multiple_values() {
    std::cout << "\n=== Test 35: .half with multiple values ===" << std::endl;
    RV32IMMachine m;

    QString code = R"(
.data
vec: .half 1, 2, 3, 4, 5
)";
    m.assemble(code);
    if (!m.getBuildSuccessful()) { std::cout << " FAIL: build\n"; return false; }

    int vecAddr = m.getLabelAddress("vec");
    if (vecAddr != RV32IMMachine::DATA_BASE) {
        std::cout << " FAIL: vec addr=0x" << std::hex << vecAddr
                  << " expected 0x" << RV32IMMachine::DATA_BASE << std::dec << std::endl;
        return false;
    }

    for (int i = 0; i < 5; i++) {
        uint16_t val = static_cast<uint16_t>(m.getMemoryValue(vecAddr + i * 2)) |
                       (static_cast<uint16_t>(m.getMemoryValue(vecAddr + i * 2 + 1)) << 8);
        if (val != i + 1) {
            std::cout << " FAIL: vec[" << i << "]=0x" << std::hex << val
                      << " expected 0x" << (i + 1) << std::dec << std::endl;
            return false;
        }
    }

    std::cout << " PASS" << std::endl;
    return true;
}

//////////////////////////////////////////////////
// Test 36: sw/lw with base register (integration test)
//////////////////////////////////////////////////
static bool test_sw_lw_integration() {
    std::cout << "\n=== Test 36: sw/lw with base register ===" << std::endl;

    // Test 1: Basic lw with label and base register
    RV32IMMachine m1;
    QString code1 = R"(
.data
valor: .word 42
.text
la t1 valor
lw t0 0(t1)
ebreak
)";
    m1.assemble(code1);
    if (!m1.getBuildSuccessful()) {
        std::cout << " FAIL: build test 1\n";
        return false;
    }
    m1.setRunning(true);
    while (m1.isRunning()) m1.step();
    int t0_val = m1.getRegisterValueByName("t0");
    std::cout << "Test 1 - la t1 valor; lw t0 0(t1): t0=" << t0_val << std::endl;
    if (t0_val != 42) {
        std::cout << " FAIL: expected 42, got " << t0_val << std::endl;
        return false;
    }

    // Test 2: Basic sw with label and base register
    RV32IMMachine m2;
    QString code2 = R"(
.data
resultado: .word 0
.text
la t1 resultado
li t0 123
sw t0 0(t1)
ebreak
)";
    m2.assemble(code2);
    if (!m2.getBuildSuccessful()) {
        std::cout << " FAIL: build test 2\n";
        return false;
    }
    m2.setRunning(true);
    while (m2.isRunning()) m2.step();
    uint32_t resultado = m2.memoryReadWord(m2.getLabelAddress("resultado"));
    std::cout << "Test 2 - la t1 resultado; li t0 123; sw t0 0(t1): resultado=" << resultado << std::endl;
    if (resultado != 123) {
        std::cout << " FAIL: expected 123, got " << resultado << std::endl;
        return false;
    }

    // Test 3: Array processing with lw/sw using base register
    RV32IMMachine m3;
    QString code3 = R"(
.data
valores: .word 10, 20, 30, 40, 50

.text
la t0 valores    # t0 = base of array
lw t1 0(t0)      # t1 = first value (10)
addi t0 t0 4     # t0 = next element
lw t2 0(t0)      # t2 = second value (20)
add t1 t1 t2     # t1 = 30
sw t1 -4(t0)     # Store 30 at first position
ebreak
)";
    m3.assemble(code3);
    if (!m3.getBuildSuccessful()) {
        std::cout << " FAIL: build test 3\n";
        return false;
    }
    m3.setRunning(true);
    while (m3.isRunning()) m3.step();
    int t1_after = m3.getRegisterValueByName("t1");
    int t2_after = m3.getRegisterValueByName("t2");
    uint32_t valores0 = m3.memoryReadWord(m3.getLabelAddress("valores"));
    std::cout << "Test 3 - lw/sw with base: t1=" << t1_after << " t2=" << t2_after
              << " valores[0]=" << valores0 << std::endl;
    if (t1_after != 30) {
        std::cout << " FAIL: expected t1=30, got " << t1_after << std::endl;
        return false;
    }
    if (valores0 != 30) {
        std::cout << " FAIL: expected valores[0]=30, got " << valores0 << std::endl;
        return false;
    }

    std::cout << " PASS" << std::endl;
    return true;
}

//////////////////////////////////////////////////
// Test 37: .byte with multiple values
//////////////////////////////////////////////////
static bool test_byte_multiple_values() {
    std::cout << "\n=== Test 36: .byte with multiple values ===" << std::endl;
    RV32IMMachine m;

    QString code = R"(
.data
vec: .byte 10, 20, 30, 40, 50
)";
    m.assemble(code);
    if (!m.getBuildSuccessful()) { std::cout << " FAIL: build\n"; return false; }

    int vecAddr = m.getLabelAddress("vec");
    if (vecAddr != RV32IMMachine::DATA_BASE) {
        std::cout << " FAIL: vec addr=0x" << std::hex << vecAddr
                  << " expected 0x" << RV32IMMachine::DATA_BASE << std::dec << std::endl;
        return false;
    }

    uint8_t expected[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        uint8_t val = static_cast<uint8_t>(m.getMemoryValue(vecAddr + i));
        if (val != expected[i]) {
            std::cout << " FAIL: vec[" << i << "]=" << (int)val
                      << " expected " << (int)expected[i] << std::endl;
            return false;
        }
    }

    std::cout << " PASS" << std::endl;
    return true;
}

//////////////////////////////////////////////////
// Test 33: All comment styles
//////////////////////////////////////////////////
static bool test_comment_styles() {
    std::cout << "\n=== Test 33: Comment styles ===" << std::endl;
    RV32IMMachine m;

    // Test all supported comment styles: # // ; %
    QString code = R"(
# Comment with hash
li t0, 42 // Comment with double slash
ecall; Comment with semicolon
% Comment with percent
)";
    m.assemble(code);
    if (!m.getBuildSuccessful()) { std::cout << " FAIL: build\n"; return false; }

    // After assembling only "li t0, 42" and "ecall", t0 should be 42
    m.updateInstructionStrings();
    runSteps(m);
    if (m.getRegisterValueByName("t0") != 42) {
        std::cout << " FAIL: t0=" << m.getRegisterValueByName("t0") << " expected 42\n";
        return false;
    }

    std::cout << " PASS" << std::endl;
    return true;
}

//////////////////////////////////////////////////
// Test 38: Negative hex values with li
//////////////////////////////////////////////////
static bool test_negative_hex_li() {
    std::cout << "\n=== Test 38: Negative hex values (li) ===" << std::endl;
    RV32IMMachine m;

    QString code = R"(
li t0, -0x10
li t1, -0xFF
li t2, -0x100
ecall
)";
    m.assemble(code);
    if (!m.getBuildSuccessful()) { std::cout << " FAIL: build\n"; return false; }

    m.updateInstructionStrings();
    runSteps(m);

    if (m.getRegisterValueByName("t0") != -16) {
        std::cout << " FAIL: t0=" << m.getRegisterValueByName("t0") << " expected -16\n";
        return false;
    }
    if (m.getRegisterValueByName("t1") != -255) {
        std::cout << " FAIL: t1=" << m.getRegisterValueByName("t1") << " expected -255\n";
        return false;
    }
    if (m.getRegisterValueByName("t2") != -256) {
        std::cout << " FAIL: t2=" << m.getRegisterValueByName("t2") << " expected -256\n";
        return false;
    }

    std::cout << " PASS" << std::endl;
    return true;
}

//////////////////////////////////////////////////
// Test 39: Negative hex in arithmetic
//////////////////////////////////////////////////
static bool test_negative_hex_arithmetic() {
    std::cout << "\n=== Test 39: Negative hex arithmetic ===" << std::endl;
    RV32IMMachine m;

    QString code = R"(
li t0, -0x10
li t1, 0x10
add t2, t0, t1
sub t3, t0, t1
ecall
)";
    m.assemble(code);
    if (!m.getBuildSuccessful()) { std::cout << " FAIL: build\n"; return false; }

    m.updateInstructionStrings();
    runSteps(m);

    if (m.getRegisterValueByName("t2") != 0) {
        std::cout << " FAIL: t2=" << m.getRegisterValueByName("t2") << " expected 0\n";
        return false;
    }
    if (m.getRegisterValueByName("t3") != -32) {
        std::cout << " FAIL: t3=" << m.getRegisterValueByName("t3") << " expected -32\n";
        return false;
    }

    std::cout << " PASS" << std::endl;
    return true;
}

//////////////////////////////////////////////////
// Test 40: Negative hex with .eqv equate
//////////////////////////////////////////////////
static bool test_negative_hex_eqv() {
    std::cout << "\n=== Test 40: Negative hex in .eqv ===" << std::endl;
    RV32IMMachine m;

    QString code = R"(
.eqv MASK -0xFF
li t0, MASK
li t1, -0x100
add t2, t0, t1
ecall
)";
    m.assemble(code);
    if (!m.getBuildSuccessful()) { std::cout << " FAIL: build\n"; return false; }

    m.updateInstructionStrings();
    runSteps(m);

    if (m.getRegisterValueByName("t0") != -255) {
        std::cout << " FAIL: t0=" << m.getRegisterValueByName("t0") << " expected -255\n";
        return false;
    }
    if (m.getRegisterValueByName("t1") != -256) {
        std::cout << " FAIL: t1=" << m.getRegisterValueByName("t1") << " expected -256\n";
        return false;
    }
    if (m.getRegisterValueByName("t2") != -511) {
        std::cout << " FAIL: t2=" << m.getRegisterValueByName("t2") << " expected -511\n";
        return false;
    }

    std::cout << " PASS" << std::endl;
    return true;
}

//////////////////////////////////////////////////
// Test 41: Mixed positive and negative hex
//////////////////////////////////////////////////
static bool test_mixed_hex() {
    std::cout << "\n=== Test 41: Mixed hex values ===" << std::endl;
    RV32IMMachine m;

    QString code = R"(
li t0, 0x100
li t1, -0x100
li t2, 0xFF
li t3, -0xFF
li t4, 0x1000
li t5, -0x1000
ecall
)";
    m.assemble(code);
    if (!m.getBuildSuccessful()) { std::cout << " FAIL: build\n"; return false; }

    m.updateInstructionStrings();
    runSteps(m);

    if (m.getRegisterValueByName("t0") != 256) {
        std::cout << " FAIL: t0=" << m.getRegisterValueByName("t0") << " expected 256\n";
        return false;
    }
    if (m.getRegisterValueByName("t1") != -256) {
        std::cout << " FAIL: t1=" << m.getRegisterValueByName("t1") << " expected -256\n";
        return false;
    }
    if (m.getRegisterValueByName("t2") != 255) {
        std::cout << " FAIL: t2=" << m.getRegisterValueByName("t2") << " expected 255\n";
        return false;
    }
    if (m.getRegisterValueByName("t3") != -255) {
        std::cout << " FAIL: t3=" << m.getRegisterValueByName("t3") << " expected -255\n";
        return false;
    }
    if (m.getRegisterValueByName("t4") != 4096) {
        std::cout << " FAIL: t4=" << m.getRegisterValueByName("t4") << " expected 4096\n";
        return false;
    }
    if (m.getRegisterValueByName("t5") != -4096) {
        std::cout << " FAIL: t5=" << m.getRegisterValueByName("t5") << " expected -4096\n";
        return false;
    }

    std::cout << " PASS" << std::endl;
    return true;
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    std::cout << "\n=== RV32IM Test Suite ===\n" << std::endl;

    int passed = 0, failed = 0;
    auto run = [&](bool (*fn)()) { if (fn()) passed++; else failed++; };

    run(test_basic);
    run(test_store_load);
    run(test_branch);
    run(test_arithmetic);
    run(test_jal_jalr);
    run(test_lui_addi);
    run(test_sparse_memory);
    run(test_register_names);
    run(test_pseudo);
    run(test_shifts);
    run(test_branch_large_li);
    run(test_div_rem);
    run(test_virtual_dispatch_via_base);
    run(test_machine_config_rv32im);
    run(test_text_data_sections);
    run(test_sp_gp_init);
    run(test_clear_after_build_preserves_init);
    run(test_failed_assemble_preserves_init);
    run(test_space_separated_args);
    run(test_data_labels);
    run(test_sw_lw_3args);
    run(test_case2_execution);
    run(test_lw_sw_label_auipc);
    run(test_sw_label_store);
    run(test_all_branches);
    run(test_lui_auipc);
    run(test_m_extension_full);
    run(test_logical_shift);
    run(test_set_instructions);
    run(test_load_store_byte_half);
    run(test_eqv_equate);
    run(test_space_directive);
    run(test_comment_styles);
    run(test_word_multiple_values);
    run(test_half_multiple_values);
    run(test_sw_lw_integration);
    run(test_byte_multiple_values);
    run(test_negative_hex_li);
    run(test_negative_hex_arithmetic);
    run(test_negative_hex_eqv);
    run(test_mixed_hex);

    std::cout << "\n=== Results: " << passed << " passed, " << failed << " failed ===\n" << std::endl;
    return failed > 0 ? 1 : 0;
}
