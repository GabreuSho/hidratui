/**
 * @file riscv_example.cpp
 * @brief Exemplo de uso do núcleo RISC-V RV32I
 * 
 * Este arquivo demonstra como usar os componentes RISC-V
 * implementados para o simulador Hidra.
 */

#include <QDebug>
#include <QCoreApplication>

#include "riscv/riscv_machine.h"
#include "riscv/riscv_decoder.h"

using namespace riscv;

/**
 * @brief Exemplo básico: Executar programa simples
 * 
 * Programa de exemplo (em machine code):
 * 0x00000000: addi x1, x0, 5    ; x1 = 5
 * 0x00000004: addi x2, x0, 3    ; x2 = 3
 * 0x00000008: add  x3, x1, x2   ; x3 = x1 + x2 = 8
 * 0x0000000C: sub  x4, x1, x2   ; x4 = x1 - x2 = 2
 * 0x00000010: and  x5, x1, x2   ; x5 = x1 & x2 = 1
 * 0x00000014: or   x6, x1, x2   ; x6 = x1 | x2 = 7
 * 0x00000018: xor  x7, x1, x2   ; x7 = x1 ^ x2 = 6
 */
void runBasicExample() {
    qDebug() << "=== Exemplo Básico RISC-V ===\n";
    
    // Criar máquina
    RiscVMachine machine;
    
    // Carregar programa na memória (little-endian)
    // addi x1, x0, 5  -> 0x00500093
    machine.setMemoryValue(0x0000, 0x93);
    machine.setMemoryValue(0x0001, 0x00);
    machine.setMemoryValue(0x0002, 0x50);
    machine.setMemoryValue(0x0003, 0x00);
    
    // addi x2, x0, 3  -> 0x00300113
    machine.setMemoryValue(0x0004, 0x13);
    machine.setMemoryValue(0x0005, 0x01);
    machine.setMemoryValue(0x0006, 0x30);
    machine.setMemoryValue(0x0007, 0x00);
    
    // add x3, x1, x2  -> 0x002081B3
    machine.setMemoryValue(0x0008, 0xB3);
    machine.setMemoryValue(0x0009, 0x81);
    machine.setMemoryValue(0x000A, 0x20);
    machine.setMemoryValue(0x000B, 0x00);
    
    // sub x4, x1, x2  -> 0x40208233
    machine.setMemoryValue(0x000C, 0x33);
    machine.setMemoryValue(0x000D, 0x82);
    machine.setMemoryValue(0x000E, 0x20);
    machine.setMemoryValue(0x000F, 0x40);
    
    // and x5, x1, x2  -> 0x0020F2B3
    machine.setMemoryValue(0x0010, 0xB3);
    machine.setMemoryValue(0x0011, 0xF2);
    machine.setMemoryValue(0x0012, 0x20);
    machine.setMemoryValue(0x0013, 0x00);
    
    // or x6, x1, x2   -> 0x0020E333
    machine.setMemoryValue(0x0014, 0x33);
    machine.setMemoryValue(0x0015, 0xE3);
    machine.setMemoryValue(0x0016, 0x20);
    machine.setMemoryValue(0x0017, 0x00);
    
    // xor x7, x1, x2  -> 0x0020C3B3
    machine.setMemoryValue(0x0018, 0xB3);
    machine.setMemoryValue(0x0019, 0xC3);
    machine.setMemoryValue(0x001A, 0x20);
    machine.setMemoryValue(0x001B, 0x00);
    
    // Iniciar simulação
    machine.setRunning(true);
    
    // Conectar signal para logging
    QObject::connect(&machine, &RiscVMachine::instructionExecuted,
        [](RV32Addr pc, const DecodedInstruction &instr) {
            QString assembly = RiscVDecoder::formatAssembly(instr);
            qDebug() << QString("PC: 0x%1").arg(pc, 8, 16, QChar('0')).toLocal8Bit().constData()
                     << "|" << assembly.toLocal8Bit().constData();
        });
    
    // Executar 7 instruções
    for (int i = 0; i < 7; ++i) {
        machine.step();
    }
    
    // Verificar resultados
    qDebug() << "\n=== Resultados ===";
    qDebug() << "x1 (ra):" << machine.getRegisterFile().readRegister(1) << "(esperado: 5)";
    qDebug() << "x2 (sp):" << machine.getRegisterFile().readRegister(2) << "(esperado: 3)";
    qDebug() << "x3 (gp):" << machine.getRegisterFile().readRegister(3) << "(esperado: 8)";
    qDebug() << "x4 (tp):" << machine.getRegisterFile().readRegister(4) << "(esperado: 2)";
    qDebug() << "x5 (t0):" << machine.getRegisterFile().readRegister(5) << "(esperado: 1)";
    qDebug() << "x6 (t1):" << machine.getRegisterFile().readRegister(6) << "(esperado: 7)";
    qDebug() << "x7 (t2):" << machine.getRegisterFile().readRegister(7) << "(esperado: 6)";
}

/**
 * @brief Exemplo: Branch e Loop
 * 
 * Programa que soma números de 1 a 5:
 *   x1 = 0 (acumulador)
 *   x2 = 5 (contador)
 *   loop: x1 += x2, x2--, branch se x2 > 0
 */
void runBranchExample() {
    qDebug() << "\n=== Exemplo com Branch ===\n";
    
    RiscVMachine machine;
    
    // Programa em machine code:
    // 0x00: addi x1, x0, 0     ; acumulador = 0
    // 0x04: addi x2, x0, 5     ; contador = 5
    // 0x08: add  x1, x1, x2    ; acumulador += contador
    // 0x0C: addi x2, x2, -1    ; contador--
    // 0x10: bnez x2, -0x08     ; branch se x2 != 0 (volta para 0x08)
    // Resultado esperado: x1 = 15 (5+4+3+2+1)
    
    // Nota: Este é um exemplo simplificado - na prática você usaria
    // um assembler para gerar o machine code automaticamente
    
    machine.setRunning(true);
    
    // Executar várias iterações
    for (int i = 0; i < 20; ++i) {
        machine.step();
        
        uint32_t x1 = machine.getRegisterFile().readRegister(1);
        uint32_t x2 = machine.getRegisterFile().readRegister(2);
        uint32_t pc = machine.getRegisterFile().getPC();
        
        qDebug() << QString("Step %1: PC=0x%2, x1=%3, x2=%4")
                        .arg(i).arg(pc, 4, 16, QChar('0')).arg(x1).arg(x2);
        
        // Parar quando terminar
        if (x2 == 0 && i > 5) {
            break;
        }
    }
    
    qDebug() << "\nResultado final: x1 =" << machine.getRegisterFile().readRegister(1)
             << "(esperado: 15)";
}

/**
 * @brief Exemplo: Load/Store
 * 
 * Demonstra acesso à memória com LW e SW
 */
void runMemoryExample() {
    qDebug() << "\n=== Exemplo com Memória ===\n";
    
    RiscVMachine machine;
    
    // Inicializar memória com alguns valores
    // Endereço 0x100: valor 42
    machine.setMemoryValue(0x0100, 0x2A);  // 42 em little-endian
    machine.setMemoryValue(0x0101, 0x00);
    machine.setMemoryValue(0x0102, 0x00);
    machine.setMemoryValue(0x0103, 0x00);
    
    // Programa:
    // 0x00: lui x1, 0x1        ; x1 = 0x1000 (upper 20 bits)
    // 0x04: addi x1, x1, 256   ; x1 = 0x1000 + 256 = 0x1100 (ajustar conforme necessário)
    // Na verdade, para endereço 0x100:
    // 0x00: addi x1, x0, 256   ; x1 = 256 (0x100)
    // 0x04: lw x2, 0(x1)       ; x2 = MEM[x1 + 0] = 42
    // 0x08: addi x2, x2, 8     ; x2 = 50
    // 0x0C: sw x2, 4(x1)       ; MEM[x1 + 4] = 50
    
    machine.setRunning(true);
    
    // Executar algumas instruções
    for (int i = 0; i < 4; ++i) {
        machine.step();
    }
    
    // Verificar valor lido
    uint32_t x2 = machine.getRegisterFile().readRegister(2);
    qDebug() << "Valor lido da memória (x2):" << x2 << "(esperado: 42)";
    
    // Verificar valor escrito
    bool success = false;
    uint32_t memValue = 0;
    // Ler 4 bytes do endereço 0x104
    for (int j = 0; j < 4; ++j) {
        uint8_t byte = static_cast<uint8_t>(machine.getMemoryValue(0x0104 + j));
        memValue |= (static_cast<uint32_t>(byte) << (j * 8));
    }
    qDebug() << "Valor escrito na memória [0x104]:" << memValue << "(esperado: 50)";
}

/**
 * @brief Função principal de exemplo
 */
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    qDebug() << "Simulador RISC-V RV32I - Hidra\n";
    qDebug() << "Instruções suportadas: RV32I Base\n";
    
    // Executar exemplos
    runBasicExample();
    runBranchExample();
    runMemoryExample();
    
    qDebug() << "\n=== Todos os exemplos completados ===";
    
    return 0;
}
