// RV32IM Machine Tests
#include <QCoreApplication>
#include <QString>
#include <iostream>
#include "machines/rv32immachine.h"

static void runSteps(RV32IMMachine &machine, int maxSteps = 30) {
    machine.setRunning(true);
    for (int i = 0; i < maxSteps && machine.isRunning(); i++)
        machine.step();
}

static void printInstrs(RV32IMMachine &machine, int start, int count) {
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
    m.updateInstructionStrings(); printInstrs(m, 0, 4);
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
    m.updateInstructionStrings(); printInstrs(m, 0, 6);
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
.org 0x0000
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
    m.updateInstructionStrings(); printInstrs(m, 0, 8);
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
.org 0x0000
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
.org 0x0000
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
.org 0x0000
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
.org 0x0000
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
.org 0x0000
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
    m.updateInstructionStrings(); printInstrs(m, 0, 12);
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
.org 0x0000
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

    std::cout << "\n=== Results: " << passed << " passed, " << failed << " failed ===\n" << std::endl;
    return failed > 0 ? 1 : 0;
}
