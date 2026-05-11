/**
 * @file tst_riscv.cpp
 * @brief Testes de integração RISC-V usando o framework SimulationTestCase
 * 
 * Estes testes validam a integração do núcleo RISC-V com o framework
 * de simulação existente do Hidra.
 */

#include <QtTest/QtTest>
#include "simulationtestcase.h"
#include "riscv_machine.h"

class TestRiscV : public SimulationTestCase
{
    Q_OBJECT

public:
    TestRiscV() : SimulationTestCase(new riscv::RiscVMachine()) {}

private slots:
    // Teste básico de inicialização
    void testInicializacao() {
        auto* machine = getMachine();
        QVERIFY(machine != nullptr);
        
        // Verifica se PC inicia em 0
        QCOMPARE(machine->getPC(), 0);
        
        // Verifica se registrador x0 é sempre zero
        QCOMPARE(machine->getRegister(0), 0);
    }

    // Teste de soma simples (ADD)
    void testSomaSimples() {
        // Programa: ADD x1, x0, x2 (x1 = 0 + x2)
        // Instrução: 0x00200083 (ADD x1, x0, x2)
        quint32 instr = 0x0020008B;  // add x1, x0, x2
        
        getMachine()->setRegister(2, 10);  // x2 = 10
        getMachine()->writeMemory(0, instr);
        
        simulateStep();
        
        QCOMPARE(getMachine()->getRegister(1), 10);  // x1 = 10
    }

    // Teste de load imediato (LI via ADDI)
    void testLoadImediato() {
        // ADDI x3, x0, 42 => x3 = 42
        quint32 instr = 0x02A00193;  // addi x3, x0, 42
        
        getMachine()->writeMemory(0, instr);
        simulateStep();
        
        QCOMPARE(getMachine()->getRegister(3), 42);
    }

    // Teste de branch condicional
    void testBranchCondiciona() {
        // Setup: x1 = 5, x2 = 5
        getMachine()->setRegister(1, 5);
        getMachine()->setRegister(2, 5);
        
        // BEQ x1, x2, label (deve tomar o branch)
        // Offset = 1 (para pular próxima instrução)
        quint32 branch = 0x00208063;  // beq x1, x2, +4
        
        getMachine()->writeMemory(0, branch);
        getMachine()->writeMemory(4, 0x00000013);  // NOP
        
        int initialPC = getMachine()->getPC();
        simulateStep();
        
        // PC deve ter avançado para o target do branch
        QVERIFY(getMachine()->getPC() > initialPC + 4);
    }

    // Teste de acesso à memória
    void testAcessoMemoria() {
        // SW x3, 0(x0) => store x3 na endereço 0
        getMachine()->setRegister(3, 0xDEADBEEF);
        getMachine()->writeMemory(0, 0x00302023);  // sw x3, 0(x0)
        
        simulateStep();
        
        // Verifica se valor foi escrito na memória
        quint32 value = getMachine()->readMemory(0);
        QCOMPARE(value, quint32(0xDEADBEEF));
    }
};

QTEST_APPLESS_MAIN(TestRiscV)
#include "tst_riscv.moc"
