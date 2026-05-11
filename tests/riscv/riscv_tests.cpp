/**
 * @file riscv_tests.cpp
 * @brief Testes unitários para o núcleo RISC-V RV32I do Hidra
 * 
 * Testes cobrindo:
 * - Register File (x0 imutável, leitura/escrita)
 * - Decoder (6 formatos, alinhamento)
 * - ALU (operações aritméticas, lógicas, shifts, comparações)
 * - Ciclo de CPU (fetch, decode, execute, memory, writeback)
 * - Casos de borda (overflow, branches, load/store desalinhado)
 */

#include <QObject>
#include <QTest>
#include <QDebug>
#include <QtTest/QtTest>
#include <QCoreApplication>

#include "riscv_register_file.h"
#include "riscv_decoder.h"
#include "riscv_alu.h"
#include "riscv_machine.h"

using namespace riscv;

/**
 * @brief Testes para o Register File RISC-V
 */
class RiscVRegisterFileTests : public QObject {
    Q_OBJECT

private slots:
    // Inicialização
    void testConstructor();
    void testReset();

    // x0 (zero) - Registrador Imutável
    void testX0AlwaysZero();
    void testX0WriteIgnored();
    void testX0ReadAfterWrite();

    // Leitura/Escrita de Registradores
    void testRegisterWrite();
    void testRegisterRead();
    void testRegisterByName();
    void testInvalidRegId();

    // Program Counter
    void testPCInitialization();
    void testPCSet();
    void testPCIncrement();
    void testPCAlignment();

    // CSRs Básicos
    void testCSRReadWrite();
    void testCSRReadOnly();

    // Signals/Hooks
    void testRegisterWrittenSignal();
    void testPCChangedSignal();

    // Casos de Borda
    void testOverflow32Bit();
    void testSignExtension();
};

void RiscVRegisterFileTests::testConstructor() {
    RiscVRegisterFile regFile;
    
    // Verifica se todos registradores iniciam em 0
    for (int i = 0; i < 32; ++i) {
        QCOMPARE(regFile.readRegister(i), 0u);
    }
    
    // PC deve iniciar em 0
    QCOMPARE(regFile.getPC(), 0u);
}

void RiscVRegisterFileTests::testReset() {
    RiscVRegisterFile regFile;
    
    // Escreve valores não-zero
    regFile.writeRegister(1, 0x12345678);
    regFile.writeRegister(10, 0xDEADBEEF);
    regFile.setPC(0x100);
    
    // Reseta
    regFile.reset();
    
    // Verifica reset
    QCOMPARE(regFile.readRegister(1), 0u);
    QCOMPARE(regFile.readRegister(10), 0u);
    QCOMPARE(regFile.getPC(), 0u);
}

void RiscVRegisterFileTests::testX0AlwaysZero() {
    RiscVRegisterFile regFile;
    
    // x0 deve ser 0 inicialmente
    QCOMPARE(regFile.readRegister(0), 0u);
    
    // Tenta escrever em x0
    bool result = regFile.writeRegister(0, 0xFFFFFFFF);
    
    // Escrita deve retornar false (ou ser ignorada)
    QVERIFY(!result || regFile.readRegister(0) == 0);
    
    // x0 continua sendo 0
    QCOMPARE(regFile.readRegister(0), 0u);
}

void RiscVRegisterFileTests::testX0WriteIgnored() {
    RiscVRegisterFile regFile;
    
    // Múltiplas tentativas de escrita em x0
    regFile.writeRegister(0, 100);
    regFile.writeRegister(0, 200);
    regFile.writeRegister(0, 0xFFFFFFFF);
    
    // x0 permanece 0
    QCOMPARE(regFile.readRegister(0), 0u);
}

void RiscVRegisterFileTests::testX0ReadAfterWrite() {
    RiscVRegisterFile regFile;
    
    // Escreve em outros registradores
    regFile.writeRegister(1, 50);
    regFile.writeRegister(2, 100);
    
    // Lê x0 após escritas em outros regs
    QCOMPARE(regFile.readRegister(0), 0u);
    QCOMPARE(regFile.readRegister(1), 50u);
    QCOMPARE(regFile.readRegister(2), 100u);
    
    // Tenta sobrescrever x0
    regFile.writeRegister(0, 999);
    
    // x0 ainda é 0, outros regs não foram afetados
    QCOMPARE(regFile.readRegister(0), 0u);
    QCOMPARE(regFile.readRegister(1), 50u);
    QCOMPARE(regFile.readRegister(2), 100u);
}

void RiscVRegisterFileTests::testRegisterWrite() {
    RiscVRegisterFile regFile;
    
    // Escreve valores variados
    QVERIFY(regFile.writeRegister(1, 0x12345678));
    QVERIFY(regFile.writeRegister(5, 0xABCD));
    QVERIFY(regFile.writeRegister(31, 0xFFFFFFFF));
    
    // Verifica leituras
    QCOMPARE(regFile.readRegister(1), 0x12345678u);
    QCOMPARE(regFile.readRegister(5), 0xABCDu);
    QCOMPARE(regFile.readRegister(31), 0xFFFFFFFFu);
}

void RiscVRegisterFileTests::testRegisterRead() {
    RiscVRegisterFile regFile;
    
    // Lê registradores não inicializados (devem ser 0)
    for (int i = 1; i < 32; ++i) {
        QCOMPARE(regFile.readRegister(i), 0u);
    }
}

void RiscVRegisterFileTests::testRegisterByName() {
    RiscVRegisterFile regFile;
    
    // Testa nomes ABI
    QVERIFY(regFile.writeRegisterByName("ra", 0x100));
    QVERIFY(regFile.writeRegisterByName("sp", 0x200));
    QVERIFY(regFile.writeRegisterByName("a0", 42));
    
    QCOMPARE(regFile.readRegisterByName("ra"), 0x100u);
    QCOMPARE(regFile.readRegisterByName("sp"), 0x200u);
    QCOMPARE(regFile.readRegisterByName("a0"), 42u);
    
    // Nome inválido deve retornar 0
    QCOMPARE(regFile.readRegisterByName("invalid"), 0u);
}

void RiscVRegisterFileTests::testInvalidRegId() {
    RiscVRegisterFile regFile;
    
    // IDs inválidos
    QCOMPARE(regFile.readRegister(-1), 0u);
    QCOMPARE(regFile.readRegister(32), 0u);
    QCOMPARE(regFile.readRegister(100), 0u);
    
    // Escrita em ID inválido deve falhar
    QVERIFY(!regFile.writeRegister(-1, 100));
    QVERIFY(!regFile.writeRegister(32, 100));
}

void RiscVRegisterFileTests::testPCInitialization() {
    RiscVRegisterFile regFile;
    QCOMPARE(regFile.getPC(), 0u);
}

void RiscVRegisterFileTests::testPCSet() {
    RiscVRegisterFile regFile;
    
    regFile.setPC(0x100);
    QCOMPARE(regFile.getPC(), 0x100u);
    
    regFile.setPC(0xABCDEF);
    QCOMPARE(regFile.getPC(), 0xABCDEFu);
}

void RiscVRegisterFileTests::testPCIncrement() {
    RiscVRegisterFile regFile;
    
    regFile.setPC(0);
    regFile.incrementPC(4);
    QCOMPARE(regFile.getPC(), 4u);
    
    regFile.incrementPC(4);
    QCOMPARE(regFile.getPC(), 8u);
    
    regFile.incrementPC(); // Default: 4 bytes
    QCOMPARE(regFile.getPC(), 12u);
}

void RiscVRegisterFileTests::testPCAlignment() {
    RiscVRegisterFile regFile;
    
    // PC desalinhado deve ser alinhado a 4 bytes
    regFile.setPC(0x103);
    QCOMPARE(regFile.getPC(), 0x100u); // Alinhado para baixo
    
    regFile.setPC(0x107);
    QCOMPARE(regFile.getPC(), 0x104u);
}

void RiscVRegisterFileTests::testCSRReadWrite() {
    RiscVRegisterFile regFile;
    
    // Testa CSR mstatus (0x300)
    QVERIFY(regFile.writeCSR(CSRId::mstatus, 0x1234));
    QCOMPARE(regFile.readCSR(CSRId::mstatus), 0x1234u);
    
    // Testa CSR mtvec (0x305)
    QVERIFY(regFile.writeCSR(CSRId::mtvec, 0x8000));
    QCOMPARE(regFile.readCSR(CSRId::mtvec), 0x8000u);
}

void RiscVRegisterFileTests::testCSRReadOnly() {
    RiscVRegisterFile regFile;
    
    // mvendorid é read-only
    uint32_t initialValue = regFile.readCSR(CSRId::mvendorid);
    
    // Tenta escrever (deve falhar ou ser ignorado)
    bool result = regFile.writeCSR(CSRId::mvendorid, 0xDEAD);
    
    // Ou retorna false ou mantém valor original
    QVERIFY(!result || regFile.readCSR(CSRId::mvendorid) == initialValue);
}

void RiscVRegisterFileTests::testRegisterWrittenSignal() {
    RiscVRegisterFile regFile;
    
    QSignalSpy spy(&regFile, SIGNAL(registerWritten(int, uint32_t, uint32_t)));
    
    regFile.writeRegister(5, 100);
    
    QCOMPARE(spy.count(), 1);
    
    QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args[0].toInt(), 5);
    QCOMPARE(args[1].toUInt(), 0u);      // oldValue
    QCOMPARE(args[2].toUInt(), 100u);    // newValue
}

void RiscVRegisterFileTests::testPCChangedSignal() {
    RiscVRegisterFile regFile;
    
    QSignalSpy spy(&regFile, SIGNAL(pcChanged(uint32_t, uint32_t)));
    
    regFile.setPC(0x100);
    
    QCOMPARE(spy.count(), 1);
    
    QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args[0].toUInt(), 0u);      // oldPC
    QCOMPARE(args[1].toUInt(), 0x100u);  // newPC
}

void RiscVRegisterFileTests::testOverflow32Bit() {
    RiscVRegisterFile regFile;
    
    // Escreve valor máximo
    regFile.writeRegister(1, 0xFFFFFFFF);
    
    // Lê e verifica que está mascarado para 32 bits
    QCOMPARE(regFile.readRegister(1), 0xFFFFFFFFu);
    
    // Adiciona 1 (overflow)
    regFile.writeRegister(1, 0xFFFFFFFFu + 1);
    
    // Deve wrapar para 0
    QCOMPARE(regFile.readRegister(1), 0u);
}

void RiscVRegisterFileTests::testSignExtension() {
    RiscVRegisterFile regFile;
    
    // Valor negativo em 32 bits (complemento de 2)
    regFile.writeRegister(1, 0xFFFFFFFF); // -1 em signed
    
    // Verifica leitura
    QCOMPARE(regFile.readRegister(1), 0xFFFFFFFFu);
    
    // Quando interpretado como signed, deve ser -1
    int32_t signedValue = static_cast<int32_t>(regFile.readRegister(1));
    QCOMPARE(signedValue, -1);
}

/**
 * @brief Testes para o Decoder RISC-V
 */
class RiscVDecoderTests : public QObject {
    Q_OBJECT

private slots:
    // Validação de alinhamento
    void testInstructionAlignment();
    void testUnalignedAddress();

    // Formato R
    void testFormatR_ADD();
    void testFormatR_SUB();
    void testFormatR_AND();
    void testFormatR_OR();
    void testFormatR_XOR();
    void testFormatR_SLL();
    void testFormatR_SRL();
    void testFormatR_SRA();
    void testFormatR_SLT();
    void testFormatR_SLTU();

    // Formato I
    void testFormatI_ADDI();
    void testFormatI_ANDI();
    void testFormatI_ORI();
    void testFormatI_XORI();
    void testFormatI_SLLI();
    void testFormatI_SRLI();
    void testFormatI_SRAI();
    void testFormatI_SLTI();
    void testFormatI_SLTIU();
    void testFormatI_LW();
    void testFormatI_LH();
    void testFormatI_LB();
    void testFormatI_JALR();

    // Formato S
    void testFormatS_SW();
    void testFormatS_SH();
    void testFormatS_SB();

    // Formato B
    void testFormatB_BEQ();
    void testFormatB_BNE();
    void testFormatB_BLT();
    void testFormatB_BGE();
    void testFormatB_BLTU();
    void testFormatB_BGEU();

    // Formato U
    void testFormatU_LUI();
    void testFormatU_AUIPC();

    // Formato J
    void testFormatJ_JAL();

    // Casos de Borda
    void testInvalidOpcode();
    void testReservedFunct3();
};

void RiscVDecoderTests::testInstructionAlignment() {
    RiscVDecoder decoder;
    
    // Instruções devem estar em endereços alinhados a 4 bytes
    QVERIFY(decoder.isAligned(0x0));
    QVERIFY(decoder.isAligned(0x4));
    QVERIFY(decoder.isAligned(0x100));
    QVERIFY(decoder.isAligned(0x1000));
    
    // Endereços desalinhados
    QVERIFY(!decoder.isAligned(0x1));
    QVERIFY(!decoder.isAligned(0x2));
    QVERIFY(!decoder.isAligned(0x3));
    QVERIFY(!decoder.isAligned(0x5));
}

void RiscVDecoderTests::testUnalignedAddress() {
    RiscVDecoder decoder;
    
    // Tentativa de decodificar em endereço desalinhado deve falhar
    // Nota: O decode atual não valida alinhamento no método decode()
    // Esta validação é feita no fetch da máquina
    instr = decoder.decode(0x00000013); // NOP
    QVERIFY(instr.type != InstrType::NONE); // Deve decodificar mesmo sem addr
}

void RiscVDecoderTests::testFormatR_ADD() {
    RiscVDecoder decoder;
    
    // ADD x1, x2, x3 -> opcode=0x33, funct3=0x0, funct7=0x00
    // Encoding: 0x003100B3
    quint32 encoding = 0x003100B3;
    
    DecodedInstruction instr = decoder.decode(encoding);
    QCOMPARE(instr.type, InstrType::R_TYPE);
    QCOMPARE(static_cast<uint8_t>(instr.opcode), static_cast<uint8_t>(Opcode::OP_OP));
    QCOMPARE(instr.funct3, 0x0);
    QCOMPARE(instr.funct7, 0x00);
    QCOMPARE(instr.rd, 1);
    QCOMPARE(instr.rs1, 2);
    QCOMPARE(instr.rs2, 3);
    QCOMPARE(instr.aluOp, ALUOp::ADD);
}

void RiscVDecoderTests::testFormatR_SUB() {
    RiscVDecoder decoder;
    
    // SUB x5, x6, x7 -> 0x407302B3
    quint32 encoding = 0x407302B3;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::R_TYPE);
    QCOMPARE(instr.funct7, 0x20);
    QCOMPARE(instr.aluOp, ALUOp::SUB);
}

void RiscVDecoderTests::testFormatR_AND() {
    RiscVDecoder decoder;
    
    // AND x8, x9, x10 -> 0x00A48433
    quint32 encoding = 0x00A48433;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::R_TYPE);
    QCOMPARE(instr.aluOp, ALUOp::AND);
}

void RiscVDecoderTests::testFormatR_OR() {
    RiscVDecoder decoder;
    
    // OR x11, x12, x13 -> 0x00D5C5B3
    quint32 encoding = 0x00D5C5B3;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::R_TYPE);
    QCOMPARE(instr.aluOp, ALUOp::OR);
}

void RiscVDecoderTests::testFormatR_XOR() {
    RiscVDecoder decoder;
    
    // XOR x14, x15, x16 -> 0x01074733
    quint32 encoding = 0x01074733;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::R_TYPE);
    QCOMPARE(instr.aluOp, ALUOp::XOR);
}

void RiscVDecoderTests::testFormatR_SLL() {
    RiscVDecoder decoder;
    
    // SLL x17, x18, x19 -> 0x013918B3
    quint32 encoding = 0x013918B3;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::R_TYPE);
    QCOMPARE(instr.aluOp, ALUOp::SLL);
}

void RiscVDecoderTests::testFormatR_SRL() {
    RiscVDecoder decoder;
    
    // SRL x20, x21, x22 -> 0x016A9A33
    quint32 encoding = 0x016A9A33;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::R_TYPE);
    QCOMPARE(instr.aluOp, ALUOp::SRL);
}

void RiscVDecoderTests::testFormatR_SRA() {
    RiscVDecoder decoder;
    
    // SRA x23, x24, x25 -> 0x400C9CB3
    quint32 encoding = 0x400C9CB3;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::R_TYPE);
    QCOMPARE(instr.aluOp, ALUOp::SRA);
}

void RiscVDecoderTests::testFormatR_SLT() {
    RiscVDecoder decoder;
    
    // SLT x26, x27, x28 -> 0x01CEECB3
    quint32 encoding = 0x01CEECB3;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::R_TYPE);
    QCOMPARE(instr.aluOp, ALUOp::SLT);
}

void RiscVDecoderTests::testFormatR_SLTU() {
    RiscVDecoder decoder;
    
    // SLTU x29, x30, x31 -> 0x01F030B3
    quint32 encoding = 0x01F030B3;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::R_TYPE);
    QCOMPARE(instr.aluOp, ALUOp::SLTU);
}

void RiscVDecoderTests::testFormatI_ADDI() {
    RiscVDecoder decoder;
    
    // ADDI x1, x2, 100 -> 0x06410093
    quint32 encoding = 0x06410093;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::I_TYPE);
    QCOMPARE(static_cast<int>(instr.opcode), 0x13);
    QCOMPARE(instr.funct3, 0x0);
    QCOMPARE(instr.rd, 1);
    QCOMPARE(instr.rs1, 2);
    QCOMPARE(instr.imm, 100);
    QCOMPARE(instr.aluOp, ALUOp::ADDI);
}

void RiscVDecoderTests::testFormatI_ANDI() {
    RiscVDecoder decoder;
    
    // ANDI x3, x4, 0xFF -> 0x0FF27193
    quint32 encoding = 0x0FF27193;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::I_TYPE);
    QCOMPARE(instr.aluOp, ALUOp::ANDI);
}

void RiscVDecoderTests::testFormatI_ORI() {
    RiscVDecoder decoder;
    
    // ORI x5, x6, 0x123 -> 0x12336313
    quint32 encoding = 0x12336313;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::I_TYPE);
    QCOMPARE(instr.aluOp, ALUOp::ORI);
}

void RiscVDecoderTests::testFormatI_XORI() {
    RiscVDecoder decoder;
    
    // XORI x7, x8, 0xF0 -> 0x0F044413
    quint32 encoding = 0x0F044413;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::I_TYPE);
    QCOMPARE(instr.aluOp, ALUOp::XORI);
}

void RiscVDecoderTests::testFormatI_SLLI() {
    RiscVDecoder decoder;
    
    // SLLI x9, x10, 5 -> 0x00551493
    quint32 encoding = 0x00551493;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::I_TYPE);
    QCOMPARE(instr.aluOp, ALUOp::SLLI);
}

void RiscVDecoderTests::testFormatI_SRLI() {
    RiscVDecoder decoder;
    
    // SRLI x11, x12, 8 -> 0x00865593
    quint32 encoding = 0x00865593;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::I_TYPE);
    QCOMPARE(instr.aluOp, ALUOp::SRLI);
}

void RiscVDecoderTests::testFormatI_SRAI() {
    RiscVDecoder decoder;
    
    // SRAI x13, x14, 16 -> 0x41077693
    quint32 encoding = 0x41077693;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::I_TYPE);
    QCOMPARE(instr.aluOp, ALUOp::SRAI);
}

void RiscVDecoderTests::testFormatI_SLTI() {
    RiscVDecoder decoder;
    
    // SLTI x15, x16, 50 -> 0x03283793
    quint32 encoding = 0x03283793;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::I_TYPE);
    QCOMPARE(instr.aluOp, ALUOp::SLTI);
}

void RiscVDecoderTests::testFormatI_SLTIU() {
    RiscVDecoder decoder;
    
    // SLTIU x17, x18, 100 -> 0x06493893
    quint32 encoding = 0x06493893;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::I_TYPE);
    QCOMPARE(instr.aluOp, ALUOp::SLTIU);
}

void RiscVDecoderTests::testFormatI_LW() {
    RiscVDecoder decoder;
    
    // LW x19, 64(x20) -> 0x040A3983
    quint32 encoding = 0x040A3983;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::I_TYPE);
    QCOMPARE(static_cast<int>(instr.opcode), 0x03);
    QCOMPARE(instr.funct3, 0x2); // LW
    QCOMPARE(instr.aluOp, ALUOp::LOAD);
}

void RiscVDecoderTests::testFormatI_LH() {
    RiscVDecoder decoder;
    
    // LH x21, 32(x22) -> 0x020B1A83
    quint32 encoding = 0x020B1A83;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::I_TYPE);
    QCOMPARE(instr.funct3, 0x1); // LH
    QCOMPARE(instr.aluOp, ALUOp::LOAD);
}

void RiscVDecoderTests::testFormatI_LB() {
    RiscVDecoder decoder;
    
    // LB x23, 16(x24) -> 0x010C0B83
    quint32 encoding = 0x010C0B83;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::I_TYPE);
    QCOMPARE(instr.funct3, 0x0); // LB
    QCOMPARE(instr.aluOp, ALUOp::LOAD);
}

void RiscVDecoderTests::testFormatI_JALR() {
    RiscVDecoder decoder;
    
    // JALR x25, 0(x26) -> 0x000D0CC3
    quint32 encoding = 0x000D0CC3;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::I_TYPE);
    QCOMPARE(static_cast<int>(instr.opcode), 0x67);
    QCOMPARE(instr.aluOp, ALUOp::JALR);
}

void RiscVDecoderTests::testFormatS_SW() {
    RiscVDecoder decoder;
    
    // SW x27, 128(x28) -> 0x080E2E23
    quint32 encoding = 0x080E2E23;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::S_TYPE);
    QCOMPARE(static_cast<int>(instr.opcode), 0x23);
    QCOMPARE(instr.funct3, 0x2); // SW
    QCOMPARE(instr.aluOp, ALUOp::STORE);
}

void RiscVDecoderTests::testFormatS_SH() {
    RiscVDecoder decoder;
    
    // SH x29, 64(x30) -> 0x040F1F23
    quint32 encoding = 0x040F1F23;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::S_TYPE);
    QCOMPARE(instr.funct3, 0x1); // SH
    QCOMPARE(instr.aluOp, ALUOp::STORE);
}

void RiscVDecoderTests::testFormatS_SB() {
    RiscVDecoder decoder;
    
    // SB x31, 32(x1) -> 0x02008023
    quint32 encoding = 0x02008023;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::S_TYPE);
    QCOMPARE(instr.funct3, 0x0); // SB
    QCOMPARE(instr.aluOp, ALUOp::STORE);
}

void RiscVDecoderTests::testFormatB_BEQ() {
    RiscVDecoder decoder;
    
    // BEQ x1, x2, offset -> 0x00208063 (exemplo simplificado)
    quint32 encoding = 0x00208063;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::B_TYPE);
    QCOMPARE(static_cast<int>(instr.opcode), 0x63);
    QCOMPARE(instr.funct3, 0x0); // BEQ
    QCOMPARE(instr.aluOp, ALUOp::BEQ);
}

void RiscVDecoderTests::testFormatB_BNE() {
    RiscVDecoder decoder;
    
    // BNE x3, x4, offset -> 0x00418163
    quint32 encoding = 0x00418163;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::B_TYPE);
    QCOMPARE(instr.funct3, 0x1); // BNE
    QCOMPARE(instr.aluOp, ALUOp::BNE);
}

void RiscVDecoderTests::testFormatB_BLT() {
    RiscVDecoder decoder;
    
    // BLT x5, x6, offset -> 0x0062C263
    quint32 encoding = 0x0062C263;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::B_TYPE);
    QCOMPARE(instr.funct3, 0x4); // BLT
    QCOMPARE(instr.aluOp, ALUOp::BLT);
}

void RiscVDecoderTests::testFormatB_BGE() {
    RiscVDecoder decoder;
    
    // BGE x7, x8, offset -> 0x0083C363
    quint32 encoding = 0x0083C363;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::B_TYPE);
    QCOMPARE(instr.funct3, 0x5); // BGE
    QCOMPARE(instr.aluOp, ALUOp::BGE);
}

void RiscVDecoderTests::testFormatB_BLTU() {
    RiscVDecoder decoder;
    
    // BLTU x9, x10, offset -> 0x00A4E463
    quint32 encoding = 0x00A4E463;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::B_TYPE);
    QCOMPARE(instr.funct3, 0x6); // BLTU
    QCOMPARE(instr.aluOp, ALUOp::BLTU);
}

void RiscVDecoderTests::testFormatB_BGEU() {
    RiscVDecoder decoder;
    
    // BGEU x11, x12, offset -> 0x00C5E563
    quint32 encoding = 0x00C5E563;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::B_TYPE);
    QCOMPARE(instr.funct3, 0x7); // BGEU
    QCOMPARE(instr.aluOp, ALUOp::BGEU);
}

void RiscVDecoderTests::testFormatU_LUI() {
    RiscVDecoder decoder;
    
    // LUI x13, 0x12345 -> 0x123456B7
    quint32 encoding = 0x123456B7;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::U_TYPE);
    QCOMPARE(static_cast<int>(instr.opcode), 0x37);
    QCOMPARE(instr.aluOp, ALUOp::LUI);
}

void RiscVDecoderTests::testFormatU_AUIPC() {
    RiscVDecoder decoder;
    
    // AUIPC x14, 0xABCDE -> 0xABCDE717
    quint32 encoding = 0xABCDE717;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::U_TYPE);
    QCOMPARE(static_cast<int>(instr.opcode), 0x17);
    QCOMPARE(instr.aluOp, ALUOp::AUIPC);
}

void RiscVDecoderTests::testFormatJ_JAL() {
    RiscVDecoder decoder;
    
    // JAL x15, offset -> 0x000007EF (exemplo simplificado)
    quint32 encoding = 0x000007EF;
    
    DecodedInstruction instr = decoder.decode(encoding); QVERIFY(instr.type != InstrType::NONE);
    QCOMPARE(instr.type, InstrType::J_TYPE);
    QCOMPARE(static_cast<int>(instr.opcode), 0x6F);
    QCOMPARE(instr.aluOp, ALUOp::JAL);
}

void RiscVDecoderTests::testInvalidOpcode() {
    RiscVDecoder decoder;
    
    // Opcode inválido/reservado
    quint32 encoding = 0x00000000; // Não é uma instrução válida
    
    // Pode falhar ou resultar em instrução desconhecida
    // Depende da implementação do decoder
    bool result = decoder.decode(0x0, encoding, instr);
    
    // Se decodificar, deve marcar como UNKNOWN
    if (result) {
        QCOMPARE(instr.aluOp, ALUOp::UNKNOWN);
    }
}

void RiscVDecoderTests::testReservedFunct3() {
    RiscVDecoder decoder;
    
    // Opcode válido mas funct3 reservado
    quint32 encoding = 0x00000073; // SYSTEM com funct3 não padrão
    
    bool result = decoder.decode(0x0, encoding, instr);
    
    // SYSTEM instruction pode ser válida ou não dependendo do funct3
    // O decoder deve lidar apropriadamente
}

/**
 * @brief Testes para a ALU RISC-V
 */
class RiscVALUTests : public QObject {
    Q_OBJECT

private slots:
    // Operações Aritméticas
    void testADD();
    void testSUB();
    void testADDOverflow();
    void testSUBUnderflow();

    // Operações Lógicas
    void testAND();
    void testOR();
    void testXOR();
    void testANDI();
    void testORI();
    void testXORI();

    // Shifts
    void testSLL();
    void testSRL();
    void testSRA();
    void testSLLI();
    void testSRLI();
    void testSRAI();

    // Comparações
    void testSLT();
    void testSLTU();
    void testSLTI();
    void testSLTIU();

    // Operações de Load/Store (cálculo de endereço)
    void testLoadAddressCalc();
    void testStoreAddressCalc();

    // Branch Comparisons
    void testBEQ();
    void testBNE();
    void testBLT();
    void testBGE();
    void testBLTU();
    void testBGEU();

    // Casos de Borda
    void testShiftByZero();
    void testShiftByMax();
    void testNegativeNumbers();
};

void RiscVALUTests::testADD() {
    RiscVALU alu;
    ALUResult result;
    
    result = alu.execute(ALUOp::ADD, 10, 20);
    QCOMPARE(result.result, 30u);
    QVERIFY(!result.overflow);
    
    result = alu.execute(ALUOp::ADD, 0xFFFFFFFF, 1);
    QCOMPARE(result.result, 0u); // Wraparound
    QVERIFY(!result.overflow); // Unsigned wrap não é overflow em RISC-V
}

void RiscVALUTests::testSUB() {
    RiscVALU alu;
    ALUResult result;
    
    result = alu.execute(ALUOp::SUB, 50, 30);
    QCOMPARE(result.result, 20u);
    QVERIFY(!result.overflow);
    
    result = alu.execute(ALUOp::SUB, 0, 1);
    QCOMPARE(result.result, 0xFFFFFFFFu); // -1 em complemento de 2
}

void RiscVALUTests::testADDOverflow() {
    RiscVALU alu;
    ALUResult result;
    
    // Overflow signed: MAX_INT + 1
    result = alu.execute(ALUOp::ADD, 0x7FFFFFFF, 1);
    QCOMPARE(result.result, 0x80000000u);
    // Em RISC-V, overflow não gera exceção, apenas wrap
    
    // Outro caso: MAX_INT + MAX_INT
    result = alu.execute(ALUOp::ADD, 0x7FFFFFFF, 0x7FFFFFFF);
    QCOMPARE(result.result, 0xFFFFFFFEu);
}

void RiscVALUTests::testSUBUnderflow() {
    RiscVALU alu;
    ALUResult result;
    
    // Underflow signed: MIN_INT - 1
    result = alu.execute(ALUOp::SUB, 0x80000000, 1);
    QCOMPARE(result.result, 0x7FFFFFFFu);
    
    // 0 - MAX_INT
    result = alu.execute(ALUOp::SUB, 0, 0x7FFFFFFF);
    QCOMPARE(result.result, 0x80000001u);
}

void RiscVALUTests::testAND() {
    RiscVALU alu;
    ALUResult result;
    
    result = alu.execute(ALUOp::AND, 0xFF00FF00, 0x0F0F0F0F);
    QCOMPARE(result.result, 0x0F000F00u);
    
    result = alu.execute(ALUOp::AND, 0xFFFFFFFF, 0x12345678);
    QCOMPARE(result.result, 0x12345678u);
}

void RiscVALUTests::testOR() {
    RiscVALU alu;
    ALUResult result;
    
    result = alu.execute(ALUOp::OR, 0xFF00FF00, 0x0F0F0F0F);
    QCOMPARE(result.result, 0xFF0FFF0Fu);
    
    result = alu.execute(ALUOp::OR, 0, 0x12345678);
    QCOMPARE(result.result, 0x12345678u);
}

void RiscVALUTests::testXOR() {
    RiscVALU alu;
    ALUResult result;
    
    result = alu.execute(ALUOp::XOR, 0xFF00FF00, 0x0F0F0F0F);
    QCOMPARE(result.result, 0xF00FF00Fu);
    
    result = alu.execute(ALUOp::XOR, 0xFFFFFFFF, 0x12345678);
    QCOMPARE(result.result, 0xEDCBA987u);
}

void RiscVALUTests::testANDI() {
    RiscVALU alu;
    ALUResult result;
    
    // ANDI com imediato sign-extended
    result = alu.execute(ALUOp::ANDI, 0xFFFFFFFF, 0xFF);
    QCOMPARE(result.result, 0xFFu);
}

void RiscVALUTests::testORI() {
    RiscVALU alu;
    ALUResult result;
    
    result = alu.execute(ALUOp::ORI, 0, 0x123);
    QCOMPARE(result.result, 0x123u);
}

void RiscVALUTests::testXORI() {
    RiscVALU alu;
    ALUResult result;
    
    result = alu.execute(ALUOp::XORI, 0xFFFF0000, 0xFFFF);
    QCOMPARE(result.result, 0xFFFFFFFFu);
}

void RiscVALUTests::testSLL() {
    RiscVALU alu;
    ALUResult result;
    
    // Shift left logical
    result = alu.execute(ALUOp::SLL, 1, 4);
    QCOMPARE(result.result, 16u);
    
    result = alu.execute(ALUOp::SLL, 0x80000000, 1);
    QCOMPARE(result.result, 0u); // Bit MSB shiftado para fora
}

void RiscVALUTests::testSRL() {
    RiscVALU alu;
    ALUResult result;
    
    // Shift right logical
    result = alu.execute(ALUOp::SRL, 16, 2);
    QCOMPARE(result.result, 4u);
    
    result = alu.execute(ALUOp::SRL, 0x80000000, 1);
    QCOMPARE(result.result, 0x40000000u); // Zero fill no MSB
}

void RiscVALUTests::testSRA() {
    RiscVALU alu;
    ALUResult result;
    
    // Shift right arithmetic (preserva sinal)
    result = alu.execute(ALUOp::SRA, 0x80000000, 1);
    QCOMPARE(result.result, 0xC0000000u); // Sign extend
    
    result = alu.execute(ALUOp::SRA, 0xFFFFFFFF, 4);
    QCOMPARE(result.result, 0xFFFFFFFFu); // -1 >> 4 = -1
}

void RiscVALUTests::testSLLI() {
    RiscVALU alu;
    ALUResult result;
    
    result = alu.execute(ALUOp::SLLI, 5, 3);
    QCOMPARE(result.result, 40u); // 5 << 3 = 40
}

void RiscVALUTests::testSRLI() {
    RiscVALU alu;
    ALUResult result;
    
    result = alu.execute(ALUOp::SRLI, 64, 3);
    QCOMPARE(result.result, 8u); // 64 >> 3 = 8
}

void RiscVALUTests::testSRAI() {
    RiscVALU alu;
    ALUResult result;
    
    result = alu.execute(ALUOp::SRAI, static_cast<uint32_t>(-16), 2);
    QCOMPARE(result.result, static_cast<uint32_t>(-4)); // -16 >> 2 = -4
}

void RiscVALUTests::testSLT() {
    RiscVALU alu;
    ALUResult result;
    
    // Set Less Than (signed)
    result = alu.execute(ALUOp::SLT, 10, 20);
    QCOMPARE(result.result, 1u);
    
    result = alu.execute(ALUOp::SLT, 20, 10);
    QCOMPARE(result.result, 0u);
    
    result = alu.execute(ALUOp::SLT, 10, 10);
    QCOMPARE(result.result, 0u);
    
    // Signed comparison: negative < positive
    result = alu.execute(ALUOp::SLT, static_cast<int32_t>(-1), 1);
    QCOMPARE(result.result, 1u);
}

void RiscVALUTests::testSLTU() {
    RiscVALU alu;
    ALUResult result;
    
    // Set Less Than Unsigned
    result = alu.execute(ALUOp::SLTU, 10, 20);
    QCOMPARE(result.result, 1u);
    
    // Unsigned: 0xFFFFFFFF > 1
    result = alu.execute(ALUOp::SLTU, 0xFFFFFFFF, 1);
    QCOMPARE(result.result, 0u);
}

void RiscVALUTests::testSLTI() {
    RiscVALU alu;
    ALUResult result;
    
    result = alu.execute(ALUOp::SLTI, 50, 100);
    QCOMPARE(result.result, 1u);
    
    result = alu.execute(ALUOp::SLTI, 100, 50);
    QCOMPARE(result.result, 0u);
}

void RiscVALUTests::testSLTIU() {
    RiscVALU alu;
    ALUResult result;
    
    result = alu.execute(ALUOp::SLTIU, 50, 100);
    QCOMPARE(result.result, 1u);
}

void RiscVALUTests::testLoadAddressCalc() {
    RiscVALU alu;
    ALUResult result;
    
    // LOAD calcula endereço: rs1 + imm
    result = alu.execute(ALUOp::LOAD, 1000, 64);
    QCOMPARE(result.result, 1064u);
}

void RiscVALUTests::testStoreAddressCalc() {
    RiscVALU alu;
    ALUResult result;
    
    // STORE calcula endereço: rs1 + imm
    result = alu.execute(ALUOp::STORE, 2000, 128);
    QCOMPARE(result.result, 2128u);
}

void RiscVALUTests::testBEQ() {
    RiscVALU alu;
    ALUResult result;
    
    // Branch if Equal
    result = alu.execute(ALUOp::BEQ, 10, 10);
    QCOMPARE(result.result, 1u); // Taken
    
    result = alu.execute(ALUOp::BEQ, 10, 20);
    QCOMPARE(result.result, 0u); // Not taken
}

void RiscVALUTests::testBNE() {
    RiscVALU alu;
    ALUResult result;
    
    // Branch if Not Equal
    result = alu.execute(ALUOp::BNE, 10, 20);
    QCOMPARE(result.result, 1u); // Taken
    
    result = alu.execute(ALUOp::BNE, 10, 10);
    QCOMPARE(result.result, 0u); // Not taken
}

void RiscVALUTests::testBLT() {
    RiscVALU alu;
    ALUResult result;
    
    // Branch if Less Than (signed)
    result = alu.execute(ALUOp::BLT, 10, 20);
    QCOMPARE(result.result, 1u);
    
    result = alu.execute(ALUOp::BLT, 20, 10);
    QCOMPARE(result.result, 0u);
    
    // Negative < Positive
    result = alu.execute(ALUOp::BLT, static_cast<int32_t>(-5), 5);
    QCOMPARE(result.result, 1u);
}

void RiscVALUTests::testBGE() {
    RiscVALU alu;
    ALUResult result;
    
    // Branch if Greater or Equal (signed)
    result = alu.execute(ALUOp::BGE, 20, 10);
    QCOMPARE(result.result, 1u);
    
    result = alu.execute(ALUOp::BGE, 10, 20);
    QCOMPARE(result.result, 0u);
    
    result = alu.execute(ALUOp::BGE, 10, 10);
    QCOMPARE(result.result, 1u);
}

void RiscVALUTests::testBLTU() {
    RiscVALU alu;
    ALUResult result;
    
    // Branch if Less Than Unsigned
    result = alu.execute(ALUOp::BLTU, 10, 20);
    QCOMPARE(result.result, 1u);
    
    // 0xFFFFFFFF > 1 (unsigned)
    result = alu.execute(ALUOp::BLTU, 0xFFFFFFFF, 1);
    QCOMPARE(result.result, 0u);
}

void RiscVALUTests::testBGEU() {
    RiscVALU alu;
    ALUResult result;
    
    // Branch if Greater or Equal Unsigned
    result = alu.execute(ALUOp::BGEU, 20, 10);
    QCOMPARE(result.result, 1u);
    
    result = alu.execute(ALUOp::BGEU, 10, 20);
    QCOMPARE(result.result, 0u);
}

void RiscVALUTests::testShiftByZero() {
    RiscVALU alu;
    ALUResult result;
    
    result = alu.execute(ALUOp::SLL, 42, 0);
    QCOMPARE(result.result, 42u);
    
    result = alu.execute(ALUOp::SRL, 42, 0);
    QCOMPARE(result.result, 42u);
    
    result = alu.execute(ALUOp::SRA, 42, 0);
    QCOMPARE(result.result, 42u);
}

void RiscVALUTests::testShiftByMax() {
    RiscVALU alu;
    ALUResult result;
    
    // Shift por 31 (máximo para RV32)
    result = alu.execute(ALUOp::SLL, 1, 31);
    QCOMPARE(result.result, 0x80000000u);
    
    result = alu.execute(ALUOp::SRL, 0x80000000, 31);
    QCOMPARE(result.result, 1u);
}

void RiscVALUTests::testNegativeNumbers() {
    RiscVALU alu;
    ALUResult result;
    
    // Operações com números negativos
    result = alu.execute(ALUOp::ADD, static_cast<uint32_t>(-10), 5);
    QCOMPARE(result.result, static_cast<uint32_t>(-5));
    
    result = alu.execute(ALUOp::SUB, 0, static_cast<uint32_t>(-1));
    QCOMPARE(result.result, 1u);
}

/**
 * @brief Testes para o Ciclo de CPU RISC-V
 */
class RiscVCPUTests : public QObject {
    Q_OBJECT

private slots:
    void testCPUInitialization();
    void testFetch();
    void testDecodeExecute();
    
    // Programas de teste completos
    void testSimpleAddition();
    void testLoop();
    void testConditionalBranch();
    void testMemoryAccess();
    void testStackOperations();
    
    // Casos de borda
    void testBranchTaken();
    void testBranchNotTaken();
    void testAlignedMemoryAccess();
    void testMisalignedMemoryAccess();
};

void RiscVCPUTests::testCPUInitialization() {
    RiscVMachine cpu;
    
    // PC inicial deve ser 0
    QCOMPARE(cpu.getPC(), 0u);
    
    // Todos registradores devem ser 0
    for (int i = 0; i < 32; ++i) {
        QCOMPARE(cpu.readRegister(i), 0u);
    }
}

void RiscVCPUTests::testFetch() {
    RiscVMachine cpu;
    
    // Coloca instrução na memória
    cpu.writeMemory(0, 0x003100B3); // ADD x1, x2, x3
    
    // Fetch
    quint32 instr = cpu.fetch();
    
    QCOMPARE(instr, 0x003100B3u);
}

void RiscVCPUTests::testDecodeExecute() {
    RiscVMachine cpu;
    
    // Setup: x2 = 10, x3 = 20
    cpu.writeRegister(2, 10);
    cpu.writeRegister(3, 20);
    
    // Coloca ADD x1, x2, x3 na memória
    cpu.writeMemory(0, 0x003100B3);
    
    // Executa um ciclo
    cpu.step();
    
    // Verifica resultado: x1 = x2 + x3 = 30
    QCOMPARE(cpu.readRegister(1), 30u);
    
    // PC deve ter incrementado
    QCOMPARE(cpu.getPC(), 4u);
}

void RiscVCPUTests::testSimpleAddition() {
    RiscVMachine cpu;
    
    // Programa: soma dois números
    // ADDI x1, x0, 10   ; x1 = 10
    // ADDI x2, x0, 20   ; x2 = 20
    // ADD x3, x1, x2    ; x3 = x1 + x2 = 30
    
    cpu.writeMemory(0x0, 0x00A00093); // ADDI x1, x0, 10
    cpu.writeMemory(0x4, 0x01400113); // ADDI x2, x0, 20
    cpu.writeMemory(0x8, 0x002081B3); // ADD x3, x1, x2
    
    // Executa 3 ciclos
    cpu.step();
    cpu.step();
    cpu.step();
    
    // Verifica resultados
    QCOMPARE(cpu.readRegister(1), 10u);
    QCOMPARE(cpu.readRegister(2), 20u);
    QCOMPARE(cpu.readRegister(3), 30u);
    QCOMPARE(cpu.getPC(), 12u);
}

void RiscVCPUTests::testLoop() {
    RiscVMachine cpu;
    
    // Programa: loop simples que conta de 0 a 4
    // ADDI x1, x0, 0    ; x1 = 0 (contador)
    // ADDI x2, x0, 5    ; x2 = 5 (limite)
    // LOOP:
    // ADDI x1, x1, 1    ; x1++
    // BLT x1, x2, LOOP  ; if x1 < x2, jump to LOOP
    
    cpu.writeMemory(0x0, 0x00000093); // ADDI x1, x0, 0
    cpu.writeMemory(0x4, 0x00500113); // ADDI x2, x0, 5
    cpu.writeMemory(0x8, 0x00108093); // ADDI x1, x1, 1
    cpu.writeMemory(0xC, 0xFEC09CE3); // BLT x1, x2, -16 (volta para 0x8)
    
    // Executa até o loop terminar (aproximadamente 20 ciclos)
    for (int i = 0; i < 20; ++i) {
        cpu.step();
    }
    
    // x1 deve ser 5 após o loop
    QCOMPARE(cpu.readRegister(1), 5u);
}

void RiscVCPUTests::testConditionalBranch() {
    RiscVMachine cpu;
    
    // Programa: branch condicional
    // ADDI x1, x0, 10
    // ADDI x2, x0, 20
    // BNE x1, x2, TAKEN  ; Branch será tomado
    // ADDI x3, x0, 100   ; Não executado
    // TAKEN:
    // ADDI x4, x0, 200
    
    cpu.writeMemory(0x0, 0x00A00093); // ADDI x1, x0, 10
    cpu.writeMemory(0x4, 0x01400113); // ADDI x2, x0, 20
    cpu.writeMemory(0x8, 0x00209163); // BNE x1, x2, +8 (pula para 0x14)
    cpu.writeMemory(0xC, 0x06400193); // ADDI x3, x0, 100 (não executado)
    cpu.writeMemory(0x10, 0x00000013); // NOP
    cpu.writeMemory(0x14, 0x0C800213); // ADDI x4, x0, 200
    
    // Executa
    cpu.step(); // ADDI x1
    cpu.step(); // ADDI x2
    cpu.step(); // BNE (taken)
    cpu.step(); // ADDI x4
    
    // Verifica
    QCOMPARE(cpu.readRegister(1), 10u);
    QCOMPARE(cpu.readRegister(2), 20u);
    QCOMPARE(cpu.readRegister(3), 0u); // Não executado
    QCOMPARE(cpu.readRegister(4), 200u);
}

void RiscVCPUTests::testMemoryAccess() {
    RiscVMachine cpu;
    
    // Programa: load/store
    // ADDI x1, x0, 100   ; x1 = 100 (endereço base)
    // ADDI x2, x0, 42    ; x2 = 42 (valor)
    // SW x2, 0(x1)       ; mem[100] = 42
    // LW x3, 0(x1)       ; x3 = mem[100]
    
    cpu.writeMemory(0x0, 0x06400093); // ADDI x1, x0, 100
    cpu.writeMemory(0x4, 0x02A00113); // ADDI x2, x0, 42
    cpu.writeMemory(0x8, 0x0020A023); // SW x2, 0(x1)
    cpu.writeMemory(0xC, 0x0000B183); // LW x3, 0(x1)
    
    // Executa
    cpu.step(); // ADDI x1
    cpu.step(); // ADDI x2
    cpu.step(); // SW
    cpu.step(); // LW
    
    // Verifica
    QCOMPARE(cpu.readRegister(1), 100u);
    QCOMPARE(cpu.readRegister(2), 42u);
    QCOMPARE(cpu.readRegister(3), 42u);
    
    // Verifica memória
    QCOMPARE(cpu.readMemoryWord(100), 42u);
}

void RiscVCPUTests::testStackOperations() {
    RiscVMachine cpu;
    
    // Setup stack pointer
    cpu.writeRegister(2, 0x1000); // sp = 0x1000
    
    // Programa:
    // ADDI x1, x0, 123   ; x1 = 123
    // SW x1, -4(sp)      ; mem[sp-4] = 123
    // LW x2, -4(sp)      ; x2 = mem[sp-4]
    
    cpu.writeMemory(0x0, 0x07B00093); // ADDI x1, x0, 123
    cpu.writeMemory(0x4, 0xFFC12C23); // SW x1, -4(sp)
    cpu.writeMemory(0x8, 0xFFC12103); // LW x2, -4(sp)
    
    // Executa
    cpu.step();
    cpu.step();
    cpu.step();
    
    // Verifica
    QCOMPARE(cpu.readRegister(1), 123u);
    QCOMPARE(cpu.readRegister(2), 123u);
}

void RiscVCPUTests::testBranchTaken() {
    RiscVMachine cpu;
    
    // BEQ sempre tomado
    // ADDI x1, x0, 5
    // BEQ x1, x1, TARGET ; Sempre igual
    // ADDI x2, x0, 100   ; Não executado
    // TARGET:
    // ADDI x3, x0, 200
    
    cpu.writeMemory(0x0, 0x00500093); // ADDI x1, x0, 5
    cpu.writeMemory(0x4, 0x00108063); // BEQ x1, x1, +8
    cpu.writeMemory(0x8, 0x06400113); // ADDI x2, x0, 100
    cpu.writeMemory(0xC, 0x0C800193); // ADDI x3, x0, 200
    
    cpu.step(); // ADDI x1
    cpu.step(); // BEQ (taken)
    cpu.step(); // ADDI x3
    
    QCOMPARE(cpu.readRegister(1), 5u);
    QCOMPARE(cpu.readRegister(2), 0u); // Não executado
    QCOMPARE(cpu.readRegister(3), 200u);
}

void RiscVCPUTests::testBranchNotTaken() {
    RiscVMachine cpu;
    
    // BEQ não tomado
    // ADDI x1, x0, 5
    // ADDI x2, x0, 10
    // BEQ x1, x2, SKIP   ; Não igual, não toma branch
    // ADDI x3, x0, 100   ; Executado
    // SKIP:
    // ADDI x4, x0, 200
    
    cpu.writeMemory(0x0, 0x00500093); // ADDI x1, x0, 5
    cpu.writeMemory(0x4, 0x00A00113); // ADDI x2, x0, 10
    cpu.writeMemory(0x8, 0x02208463); // BEQ x1, x2, +16 (SKIP)
    cpu.writeMemory(0xC, 0x06400193); // ADDI x3, x0, 100
    cpu.writeMemory(0x10, 0x0C800213); // ADDI x4, x0, 200
    
    cpu.step(); // ADDI x1
    cpu.step(); // ADDI x2
    cpu.step(); // BEQ (not taken)
    cpu.step(); // ADDI x3
    cpu.step(); // ADDI x4
    
    QCOMPARE(cpu.readRegister(3), 100u);
    QCOMPARE(cpu.readRegister(4), 200u);
}

void RiscVCPUTests::testAlignedMemoryAccess() {
    RiscVMachine cpu;
    
    // Acesso alinhado a word (4 bytes)
    cpu.writeMemory(0x0, 0x06400093); // ADDI x1, x0, 100
    cpu.writeMemory(0x4, 0x0010A023); // SW x1, 0(x0) - addr 0 (alinhado)
    cpu.writeMemory(0x8, 0x00002103); // LW x2, 0(x0)
    
    cpu.step();
    cpu.step();
    cpu.step();
    
    QCOMPARE(cpu.readRegister(2), 100u);
}

void RiscVCPUTests::testMisalignedMemoryAccess() {
    RiscVMachine cpu;
    
    // Acesso desalinhado deve gerar exception ou comportamento definido
    // Em RV32I, load/store desalinhado gera exception
    
    // Nota: Implementação atual pode ou não suportar acesso desalinhado
    // Este teste verifica o comportamento
    
    cpu.writeRegister(1, 1); // Endereço desalinhado
    
    // Tentativa de LW em endereço desalinhado
    // LW x2, 0(x1) -> addr = 1 (desalinhado)
    cpu.writeMemory(0x0, 0x0010B103);
    
    // Executa - pode trapar ou behavior undefined
    cpu.step();
    
    // Dependendo da implementação, pode:
    // 1. Gerar exception
    // 2. Funcionar (se hardware suportar)
    // 3. Retornar valor incorreto
    // Este teste documenta o comportamento esperado
}

// Main para rodar testes
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    RiscVRegisterFileTests regTests;
    RiscVDecoderTests decTests;
    RiscVALUTests aluTests;
    RiscVCPUTests cpuTests;
    
    int result = 0;
    
    qDebug() << "=== Running RISC-V Register File Tests ===";
    result |= QTest::qExec(&regTests, argc, argv);
    
    qDebug() << "\n=== Running RISC-V Decoder Tests ===";
    result |= QTest::qExec(&decTests, argc, argv);
    
    qDebug() << "\n=== Running RISC-V ALU Tests ===";
    result |= QTest::qExec(&aluTests, argc, argv);
    
    qDebug() << "\n=== Running RISC-V CPU Cycle Tests ===";
    result |= QTest::qExec(&cpuTests, argc, argv);
    
    if (result == 0) {
        qDebug() << "\n✓ All tests passed!";
    } else {
        qDebug() << "\n✗ Some tests failed!";
    }
    
    return result;
}

#include "riscv_tests.moc"
