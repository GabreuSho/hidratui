/*******************************************************************************
 *
 * RV32IMMachine - RISC-V RV32IM simulation machine
 *
 * Implements a 32-bit RISC-V (RV32IM) machine with sparse memory
 * and full register file (x0-x31). Overrides the 8-bit framework
 * where necessary to support 32-bit fixed-length instructions.
 *
 ******************************************************************************/

#include "rv32immachine.h"
#include "core/byte.h"
#include <climits>
#include <cstdint>
#include <algorithm>
#include <QtGlobal>

//////////////////////////////////////////////////
// Static member definitions
//////////////////////////////////////////////////

QMap<QString, int> RV32IMMachine::abiNameToIndex_;
QMap<int, QString> RV32IMMachine::indexToAbiName_;

//////////////////////////////////////////////////
// Constructor & Destructor
//////////////////////////////////////////////////

RV32IMMachine::RV32IMMachine(QObject *parent)
    : Machine(parent)
{
    identifier = "RVM";

    //////////////////////////////////////////////////
    // Initialize static ABI name maps (once only)
    //////////////////////////////////////////////////
    static bool mapsInitialized = false;
    if (!mapsInitialized) {
        abiNameToIndex_["zero"] = 0;
        abiNameToIndex_["ra"]   = 1;
        abiNameToIndex_["sp"]   = 2;
        abiNameToIndex_["gp"]   = 3;
        abiNameToIndex_["tp"]   = 4;
        abiNameToIndex_["t0"]   = 5;
        abiNameToIndex_["t1"]   = 6;
        abiNameToIndex_["t2"]   = 7;
        abiNameToIndex_["s0"]   = 8;
        abiNameToIndex_["fp"]   = 8;
        abiNameToIndex_["s1"]   = 9;
        abiNameToIndex_["a0"]   = 10;
        abiNameToIndex_["a1"]   = 11;
        abiNameToIndex_["a2"]   = 12;
        abiNameToIndex_["a3"]   = 13;
        abiNameToIndex_["a4"]   = 14;
        abiNameToIndex_["a5"]   = 15;
        abiNameToIndex_["a6"]   = 16;
        abiNameToIndex_["a7"]   = 17;
        abiNameToIndex_["s2"]   = 18;
        abiNameToIndex_["s3"]   = 19;
        abiNameToIndex_["s4"]   = 20;
        abiNameToIndex_["s5"]   = 21;
        abiNameToIndex_["s6"]   = 22;
        abiNameToIndex_["s7"]   = 23;
        abiNameToIndex_["s8"]   = 24;
        abiNameToIndex_["s9"]   = 25;
        abiNameToIndex_["s10"]  = 26;
        abiNameToIndex_["s11"]  = 27;
        abiNameToIndex_["t3"]   = 28;
        abiNameToIndex_["t4"]   = 29;
        abiNameToIndex_["t5"]   = 30;
        abiNameToIndex_["t6"]   = 31;

        for (int i = 0; i < 32; i++)
            abiNameToIndex_[QString("x%1").arg(i)] = i;

        indexToAbiName_[0]  = "zero";
        indexToAbiName_[1]  = "ra";
        indexToAbiName_[2]  = "sp";
        indexToAbiName_[3]  = "gp";
        indexToAbiName_[4]  = "tp";
        indexToAbiName_[5]  = "t0";
        indexToAbiName_[6]  = "t1";
        indexToAbiName_[7]  = "t2";
        indexToAbiName_[8]  = "s0/fp";
        indexToAbiName_[9]  = "s1";
        indexToAbiName_[10] = "a0";
        indexToAbiName_[11] = "a1";
        indexToAbiName_[12] = "a2";
        indexToAbiName_[13] = "a3";
        indexToAbiName_[14] = "a4";
        indexToAbiName_[15] = "a5";
        indexToAbiName_[16] = "a6";
        indexToAbiName_[17] = "a7";
        indexToAbiName_[18] = "s2";
        indexToAbiName_[19] = "s3";
        indexToAbiName_[20] = "s4";
        indexToAbiName_[21] = "s5";
        indexToAbiName_[22] = "s6";
        indexToAbiName_[23] = "s7";
        indexToAbiName_[24] = "s8";
        indexToAbiName_[25] = "s9";
        indexToAbiName_[26] = "s10";
        indexToAbiName_[27] = "s11";
        indexToAbiName_[28] = "t3";
        indexToAbiName_[29] = "t4";
        indexToAbiName_[30] = "t5";
        indexToAbiName_[31] = "t6";
        mapsInitialized = true;
    }

    for (int i = 0; i < 32; i++)
        regFile[i] = 0;

    pcValue_ = 0;
    fetchedInstruction_ = 0;

    //////////////////////////////////////////////////
    // Create Register objects for TUI compatibility
    //////////////////////////////////////////////////
    for (int i = 0; i < 32; i++) {
        QString name = resolveRegisterName(i);
        registers.append(new Register(name, "", 32, true));
    }
    registers.append(new Register("PC", "", 32, false));
    PC = registers.last();

    memoryMask = 0xFFFFFFFF;
    littleEndian = true;

    buildSuccessful = true;
    buildSuccessfulRV32_ = true;
    firstErrorLine = -1;
    firstErrorLineRV32_ = -1;

    decodedOpcode_ = 0;
    decodedRd_ = 0;
    decodedRs1_ = 0;
    decodedRs2_ = 0;
    decodedFunct3_ = 0;
    decodedFunct7_ = 0;
    decodedImm_ = 0;

    instructionCount = 0;
    accessCount = 0;
    running = false;
    breakpoint = -1;

    //////////////////////////////////////////////////
    // Add dummy AddressingMode and Instruction
    //////////////////////////////////////////////////
    addressingModes.append(new AddressingMode("......00", AddressingMode::DIRECT, AddressingMode::NO_PATTERN));
    instructions.append(new Instruction(1, "00000000", Instruction::NOP, "nop"));
}

RV32IMMachine::~RV32IMMachine()
{
}

//////////////////////////////////////////////////
// Helper methods
//////////////////////////////////////////////////

int RV32IMMachine::signExtend(int value, int bitWidth)
{
    int mask = 1 << (bitWidth - 1);
    return (value ^ mask) - mask;
}

int RV32IMMachine::resolveRegisterIndex(const QString &name) const
{
    if (abiNameToIndex_.contains(name.toLower()))
        return abiNameToIndex_[name.toLower()];

    if (name.startsWith('x') || name.startsWith('X')) {
        bool ok;
        int index = name.mid(1).toInt(&ok);
        if (ok && index >= 0 && index <= 31)
            return index;
    }
    return -1;
}

QString RV32IMMachine::resolveRegisterName(int index) const
{
    if (indexToAbiName_.contains(index))
        return indexToAbiName_[index];
    return QString("x%1").arg(index);
}

void RV32IMMachine::writeRegister(int index, int value)
{
    if (index != 0)
        regFile[index] = value;
}

//////////////////////////////////////////////////
// Memory access (sparse storage)
//////////////////////////////////////////////////

int RV32IMMachine::getMemorySize() const
{
    return INT_MAX;
}

int RV32IMMachine::getMemoryValue(int address) const
{
    return sparseMemory_.value(address, 0);
}

void RV32IMMachine::setMemoryValue(int address, int value)
{
    if (value != 0) {
        sparseMemory_[address] = static_cast<uint8_t>(value & 0xFF);
    } else {
        sparseMemory_.remove(address);
    }
    changedAddresses_.insert(address);
}

bool RV32IMMachine::hasByteChanged(int address)
{
    if (changedAddresses_.contains(address)) {
        changedAddresses_.remove(address);
        return true;
    }
    return false;
}

void RV32IMMachine::clearMemory()
{
    sparseMemory_.clear();
    changedAddresses_.clear();
}

//////////////////////////////////////////////////
// RISC-V memory read/write (byte/half/word)
//////////////////////////////////////////////////

int RV32IMMachine::memoryReadByte(int address)
{
    return getMemoryValue(address);
}

int RV32IMMachine::memoryReadHalf(int address)
{
    int b0 = getMemoryValue(address);
    int b1 = getMemoryValue(address + 1);
    return b0 | (b1 << 8);
}

int RV32IMMachine::memoryReadWord(int address)
{
    int b0 = getMemoryValue(address);
    int b1 = getMemoryValue(address + 1);
    int b2 = getMemoryValue(address + 2);
    int b3 = getMemoryValue(address + 3);
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

void RV32IMMachine::memoryWriteByte(int address, int value)
{
    setMemoryValue(address, value & 0xFF);
}

void RV32IMMachine::memoryWriteHalf(int address, int value)
{
    setMemoryValue(address, value & 0xFF);
    setMemoryValue(address + 1, (value >> 8) & 0xFF);
}

void RV32IMMachine::memoryWriteWord(int address, int value)
{
    setMemoryValue(address, value & 0xFF);
    setMemoryValue(address + 1, (value >> 8) & 0xFF);
    setMemoryValue(address + 2, (value >> 16) & 0xFF);
    setMemoryValue(address + 3, (value >> 24) & 0xFF);
}

//////////////////////////////////////////////////
// Register access by name (ABI or numeric)
//////////////////////////////////////////////////

int RV32IMMachine::getRegisterValueByName(const QString &name)
{
    int index = resolveRegisterIndex(name);
    if (index < 0) return 0;
    return regFile[index];
}

void RV32IMMachine::setRegisterValueByName(const QString &name, int value)
{
    int index = resolveRegisterIndex(name);
    if (index >= 0)
        writeRegister(index, value);
}

//////////////////////////////////////////////////
// Override getRegisterValue / setRegisterValue
//////////////////////////////////////////////////

int RV32IMMachine::getRegisterValue(int id, bool signedData) const
{
    if (id >= 0 && id < 32) {
        if (signedData)
            return regFile[id];
        return regFile[id];
    }
    if (id == registers.size() - 1)
        return pcValue_;
    return 0;
}

int RV32IMMachine::getRegisterValue(QString registerName) const
{
    int index = const_cast<RV32IMMachine*>(this)->resolveRegisterIndex(registerName);
    if (index >= 0)
        return regFile[index];
    if (registerName.compare("PC", Qt::CaseInsensitive) == 0)
        return pcValue_;
    return 0;
}

void RV32IMMachine::setRegisterValue(int id, int value)
{
    if (id >= 0 && id < 32) {
        writeRegister(id, value);
        return;
    }
    if (id == registers.size() - 1) {
        pcValue_ = value;
        Machine::PC->setValue(value);
    }
}

void RV32IMMachine::setRegisterValue(QString registerName, int value)
{
    int index = resolveRegisterIndex(registerName);
    if (index >= 0) {
        writeRegister(index, value);
        return;
    }
    if (registerName.compare("PC", Qt::CaseInsensitive) == 0) {
        pcValue_ = value;
        Machine::PC->setValue(value);
    }
}

bool RV32IMMachine::hasRegister(QString registerName) const
{
    int idx = const_cast<RV32IMMachine*>(this)->resolveRegisterIndex(registerName);
    if (idx >= 0)
        return true;
    if (registerName.compare("PC", Qt::CaseInsensitive) == 0)
        return true;
    return Machine::hasRegister(registerName);
}

//////////////////////////////////////////////////
// Override PC access
//////////////////////////////////////////////////

int RV32IMMachine::getPCValue() const
{
    return pcValue_;
}

void RV32IMMachine::setPCValue(int value)
{
    pcValue_ = value;
    Machine::PC->setValue(value);
}

void RV32IMMachine::incrementPCValue(int units)
{
    pcValue_ += units;
    Machine::PC->setValue(pcValue_);
}

//////////////////////////////////////////////////
// Instruction strings (hash-based)
//////////////////////////////////////////////////

QString RV32IMMachine::getInstructionString(int address)
{
    return instructionStrings_.value(address, "");
}

void RV32IMMachine::setInstructionString(int address, QString value)
{
    instructionStrings_[address] = value;
}

void RV32IMMachine::clearInstructionStrings()
{
    instructionStrings_.clear();
}

//////////////////////////////////////////////////
// Address validation
//////////////////////////////////////////////////

bool RV32IMMachine::isValidAddress(QString addressString)
{
    bool ok;
    int value;
    addressString = addressString.trimmed();
    if (addressString.startsWith("0x", Qt::CaseInsensitive))
        value = addressString.toInt(&ok, 16);
    else
        value = addressString.toInt(&ok, 10);
    if (!ok) return false;
    return true;
}

bool RV32IMMachine::isValidOrg(QString offsetString)
{
    bool ok;
    int value;
    offsetString = offsetString.trimmed();
    if (offsetString.startsWith("0x", Qt::CaseInsensitive))
        value = offsetString.toInt(&ok, 16);
    else
        value = offsetString.toInt(&ok, 10);
    if (!ok) return false;
    return (value % 4 == 0);
}

//////////////////////////////////////////////////
// State management
//////////////////////////////////////////////////

void RV32IMMachine::clear()
{
    clearMemory();
    for (int i = 0; i < 32; i++)
        regFile[i] = 0;
    pcValue_ = 0;
    Machine::PC->setValue(0);
    instructionCount = 0;
    accessCount = 0;
    clearInstructionStrings();
    assemblerMemory_.clear();
    assemblerLabels_.clear();
    addressCorrespondingSourceLine.clear();
    sourceLineCorrespondingAddress.clear();
    addressCorrespondingLabel.clear();
    running = false;
    breakpoint = -1;
}

void RV32IMMachine::clearAfterBuild()
{
    for (int i = 0; i < 32; i++)
        regFile[i] = 0;
    pcValue_ = 0;
    Machine::PC->setValue(0);
    instructionCount = 0;
    accessCount = 0;
    clearInstructionStrings();
    running = false;
}

//////////////////////////////////////////////////
// PLACEHOLDER: generateDescriptions, step, assemble, etc.
// Will be filled in subsequent edits.
//////////////////////////////////////////////////

void RV32IMMachine::generateDescriptions()
{
    //////////////////////////////////////////////////
    // RV32I Base Integer Instructions
    //////////////////////////////////////////////////

    // R-type arithmetic/logical
    descriptions["add rd, rs1, rs2"] = "Adiciona rs1 e rs2, resultado em rd.";
    descriptions["sub rd, rs1, rs2"] = "Subtrai rs2 de rs1, resultado em rd.";
    descriptions["and rd, rs1, rs2"] = "Realiza AND lógico bit a bit entre rs1 e rs2, resultado em rd.";
    descriptions["or rd, rs1, rs2"] = "Realiza OR lógico bit a bit entre rs1 e rs2, resultado em rd.";
    descriptions["xor rd, rs1, rs2"] = "Realiza XOR lógico bit a bit entre rs1 e rs2, resultado em rd.";
    descriptions["sll rd, rs1, rs2"] = "Desloca rs1 à esquerda pelo número de bits indicado pelos 5 bits menos significativos de rs2, resultado em rd.";
    descriptions["srl rd, rs1, rs2"] = "Desloca rs1 à direita (lógico) pelo número de bits indicado pelos 5 bits menos significativos de rs2, resultado em rd.";
    descriptions["sra rd, rs1, rs2"] = "Desloca rs1 à direita (aritmético, preserva sinal) pelo número de bits indicado pelos 5 bits menos significativos de rs2, resultado em rd.";
    descriptions["slt rd, rs1, rs2"] = "Se rs1 &lt; rs2 (com sinal), rd = 1; caso contrário, rd = 0.";
    descriptions["sltu rd, rs1, rs2"] = "Se rs1 &lt; rs2 (sem sinal), rd = 1; caso contrário, rd = 0.";

    // I-type arithmetic/logical
    descriptions["addi rd, rs1, imm"] = "Adiciona rs1 e imediato, resultado em rd.";
    descriptions["andi rd, rs1, imm"] = "Realiza AND lógico bit a bit entre rs1 e imediato, resultado em rd.";
    descriptions["ori rd, rs1, imm"] = "Realiza OR lógico bit a bit entre rs1 e imediato, resultado em rd.";
    descriptions["xori rd, rs1, imm"] = "Realiza XOR lógico bit a bit entre rs1 e imediato, resultado em rd.";
    descriptions["slli rd, rs1, imm"] = "Desloca rs1 à esquerda por imm (5 bits), resultado em rd.";
    descriptions["srli rd, rs1, imm"] = "Desloca rs1 à direita (lógico) por imm (5 bits), resultado em rd.";
    descriptions["srai rd, rs1, imm"] = "Desloca rs1 à direita (aritmético) por imm (5 bits), resultado em rd.";
    descriptions["slti rd, rs1, imm"] = "Se rs1 &lt; imediato (com sinal), rd = 1; caso contrário, rd = 0.";
    descriptions["sltiu rd, rs1, imm"] = "Se rs1 &lt; imediato (sem sinal), rd = 1; caso contrário, rd = 0.";

    // Load/Store
    descriptions["lb rd, offset(rs1)"] = "Carrega byte (com sinal) da memória no endereço rs1 + offset para rd.";
    descriptions["lbu rd, offset(rs1)"] = "Carrega byte (sem sinal) da memória no endereço rs1 + offset para rd.";
    descriptions["lh rd, offset(rs1)"] = "Carrega halfword (com sinal) da memória no endereço rs1 + offset para rd.";
    descriptions["lhu rd, offset(rs1)"] = "Carrega halfword (sem sinal) da memória no endereço rs1 + offset para rd.";
    descriptions["lw rd, offset(rs1)"] = "Carrega word da memória no endereço rs1 + offset para rd.";
    descriptions["sb rs2, offset(rs1)"] = "Armazena o byte menos significativo de rs2 na memória no endereço rs1 + offset.";
    descriptions["sh rs2, offset(rs1)"] = "Armazena os 2 bytes menos significativos de rs2 na memória no endereço rs1 + offset.";
    descriptions["sw rs2, offset(rs1)"] = "Armazena rs2 (word) na memória no endereço rs1 + offset.";

    // Branches
    descriptions["beq rs1, rs2, offset"] = "Se rs1 == rs2, desvia para PC + offset.";
    descriptions["bne rs1, rs2, offset"] = "Se rs1 != rs2, desvia para PC + offset.";
    descriptions["blt rs1, rs2, offset"] = "Se rs1 &lt; rs2 (com sinal), desvia para PC + offset.";
    descriptions["bge rs1, rs2, offset"] = "Se rs1 >= rs2 (com sinal), desvia para PC + offset.";
    descriptions["bltu rs1, rs2, offset"] = "Se rs1 &lt; rs2 (sem sinal), desvia para PC + offset.";
    descriptions["bgeu rs1, rs2, offset"] = "Se rs1 >= rs2 (sem sinal), desvia para PC + offset.";

    // Jump & Link
    descriptions["jal rd, offset"] = "Desvia para PC + offset; rd recebe PC + 4 (endereço de retorno).";
    descriptions["jalr rd, offset(rs1)"] = "Desvia para (rs1 + offset) &amp; ~1; rd recebe PC + 4 (endereço de retorno).";

    // Upper Immediate
    descriptions["lui rd, imm"] = "Carrega imediato nos 20 bits superiores de rd; bits inferiores zerados.";
    descriptions["auipc rd, imm"] = "Adiciona imediato (shiftado 12 bits à esquerda) ao PC atual; resultado em rd.";

    // System
    descriptions["ecall"] = "Chamada de sistema — delega ao ambiente (pode encerrar o programa).";
    descriptions["ebreak"] = "Ponto de interrupção — sinaliza ao depurador.";

    //////////////////////////////////////////////////
    // RV32M Multiply/Divide Extension
    //////////////////////////////////////////////////
    descriptions["mul rd, rs1, rs2"] = "Multiplica rs1 por rs2; rd recebe os 32 bits menos significativos do produto.";
    descriptions["mulh rd, rs1, rs2"] = "Multiplica rs1 por rs2 (com sinal); rd recebe os 32 bits mais significativos do produto.";
    descriptions["mulhsu rd, rs1, rs2"] = "Multiplica rs1 (com sinal) por rs2 (sem sinal); rd recebe os 32 bits mais significativos.";
    descriptions["mulhu rd, rs1, rs2"] = "Multiplica rs1 por rs2 (sem sinal); rd recebe os 32 bits mais significativos do produto.";
    descriptions["div rd, rs1, rs2"] = "Divide rs1 por rs2 (com sinal); rd recebe o quociente truncado. Divisão por zero resulta em -1.";
    descriptions["divu rd, rs1, rs2"] = "Divide rs1 por rs2 (sem sinal); rd recebe o quociente. Divisão por zero resulta em 0xFFFFFFFF.";
    descriptions["rem rd, rs1, rs2"] = "Resto da divisão de rs1 por rs2 (com sinal); rd recebe o resto. Divisão por zero resulta em rs1.";
    descriptions["remu rd, rs1, rs2"] = "Resto da divisão de rs1 por rs2 (sem sinal); rd recebe o resto. Divisão por zero resulta em rs1.";

    //////////////////////////////////////////////////
    // Pseudo-instructions
    //////////////////////////////////////////////////
    descriptions["nop"] = "Nenhuma operação (expande para addi x0, x0, 0).";
    descriptions["mv rd, rs"] = "Copia o valor de rs para rd (expande para addi rd, rs, 0).";
    descriptions["li rd, imm"] = "Carrega imediato em rd (expande para lui/addi ou addi).";
    descriptions["la rd, label"] = "Carrega endereço do label em rd (expande para auipc/addi).";
    descriptions["j offset"] = "Desvio incondicional (expande para jal x0, offset).";
    descriptions["jal offset"] = "Desvio incondicional com link (expande para jal x1, offset).";
    descriptions["jr rs"] = "Desvia para endereço em rs (expande para jalr x0, 0(rs)).";
    descriptions["jalr rs"] = "Desvia para endereço em rs com link (expande para jalr x1, 0(rs)).";
    descriptions["ret"] = "Retorna de subrotina (expande para jalr x0, 0(ra)).";
    descriptions["beqz rs1, offset"] = "Se rs1 == 0, desvia (expande para beq rs1, x0, offset).";
    descriptions["bnez rs1, offset"] = "Se rs1 != 0, desvia (expande para bne rs1, x0, offset).";
    descriptions["bgez rs1, offset"] = "Se rs1 >= 0, desvia (expande para bge rs1, x0, offset).";
    descriptions["bltz rs1, offset"] = "Se rs1 &lt; 0, desvia (expande para blt rs1, x0, offset).";
    descriptions["blez rs1, offset"] = "Se rs1 &lt;= 0, desvia (expande para bge x0, rs1, offset).";
    descriptions["bgtz rs1, offset"] = "Se rs1 &gt; 0, desvia (expande para blt x0, rs1, offset).";
    descriptions["not rd, rs"] = "Inverte os bits de rs (expande para xori rd, rs, -1).";
    descriptions["neg rd, rs"] = "Negativo de rs (expande para sub rd, x0, rs).";
    descriptions["seqz rd, rs"] = "Se rs == 0, rd = 1 (expande para sltiu rd, rs, 1).";
    descriptions["snez rd, rs"] = "Se rs != 0, rd = 1 (expande para sltu rd, x0, rs).";
    descriptions["sltz rd, rs"] = "Se rs &lt; 0, rd = 1 (expande para slt rd, rs, x0).";
    descriptions["sgtz rd, rs"] = "Se rs &gt; 0, rd = 1 (expande para slt rd, x0, rs).";
    descriptions["call symbol"] = "Chama subrotina com PC-relativo (expande para auipc + jalr).";
    descriptions["tail symbol"] = "Chama subrotina sem link (expande para auipc + jalr x0).";
    descriptions["csrr rd, csr"] = "Lê registrador de controle e status (CSR).";
    descriptions["csrw csr, rs"] = "Escreve registrador de controle e status (CSR).";
    descriptions["csrs csr, rs"] = "Seta bits do CSR via CSRRS.";
    descriptions["csrc csr, rs"] = "Limpa bits do CSR via CSRRC.";
}
void RV32IMMachine::step()
{
    fetchInstruction();
    decodeInstruction();
    executeInstruction();
    if (getPCValue() == getBreakpoint())
        setRunning(false);
}

void RV32IMMachine::fetchInstruction()
{
    int b0 = getMemoryValue(pcValue_);
    int b1 = getMemoryValue(pcValue_ + 1);
    int b2 = getMemoryValue(pcValue_ + 2);
    int b3 = getMemoryValue(pcValue_ + 3);
    fetchedInstruction_ = (static_cast<uint32_t>(b3) << 24) |
                          (static_cast<uint32_t>(b2) << 16) |
                          (static_cast<uint32_t>(b1) << 8)  |
                          static_cast<uint32_t>(b0);
    incrementPCValue(4);
    currentInstruction = nullptr;
}

void RV32IMMachine::decodeInstruction()
{
    uint32_t inst = fetchedInstruction_;

    decodedOpcode_  = inst & 0x7F;
    decodedRd_      = (inst >> 7)  & 0x1F;
    decodedRs1_     = (inst >> 15) & 0x1F;
    decodedRs2_     = (inst >> 20) & 0x1F;
    decodedFunct3_  = (inst >> 12) & 0x7;
    decodedFunct7_  = (inst >> 25) & 0x7F;

    switch (decodedOpcode_) {
    case 0x37: // LUI (U-type)
    case 0x17: // AUIPC (U-type)
        decodedImm_ = static_cast<int>(inst & 0xFFFFF000);
        break;
    case 0x6F: // JAL (J-type)
    {
        int bit20    = (inst >> 31) & 1;
        int bit10_1  = (inst >> 21) & 0x3FF;
        int bit11    = (inst >> 20) & 1;
        int bit19_12 = (inst >> 12) & 0xFF;
        int offset = (bit20 << 20) | (bit19_12 << 12) | (bit11 << 11) | (bit10_1 << 1);
        decodedImm_ = signExtend(offset, 21);
        break;
    }
    case 0x67: // JALR (I-type)
    case 0x03: // Load (I-type)
    case 0x13: // OP-IMM (I-type)
    case 0x73: // SYSTEM (I-type)
    {
        int imm = (inst >> 20) & 0xFFF;
        decodedImm_ = signExtend(imm, 12);
        break;
    }
    case 0x23: // Store (S-type)
    {
        int imm11_5 = (inst >> 25) & 0x7F;
        int imm4_0  = (inst >> 7)  & 0x1F;
        int imm = (imm11_5 << 5) | imm4_0;
        decodedImm_ = signExtend(imm, 12);
        break;
    }
    case 0x63: // Branch (B-type)
    {
        int bit12   = (inst >> 31) & 1;
        int bit10_5 = (inst >> 25) & 0x3F;
        int bit4_1  = (inst >> 8)  & 0xF;
        int bit11   = (inst >> 7)  & 1;
        int offset = (bit12 << 12) | (bit11 << 11) | (bit10_5 << 5) | (bit4_1 << 1);
        decodedImm_ = signExtend(offset, 13);
        break;
    }
    default:
        decodedImm_ = 0;
        break;
    }
}

void RV32IMMachine::executeInstruction()
{
    int rs1v = regFile[decodedRs1_];
    int rs2v = regFile[decodedRs2_];
    int rd   = decodedRd_;
    int funct3 = decodedFunct3_;
    int funct7 = decodedFunct7_;
    int imm  = decodedImm_;
    int pc   = getPCValue();

    switch (decodedOpcode_) {

    // LUI
    case 0x37:
        writeRegister(rd, imm);
        break;

    // AUIPC
    case 0x17:
        writeRegister(rd, (pc - 4) + imm);
        break;

    // JAL
    case 0x6F:
        writeRegister(rd, pc);
        setPCValue((pc - 4) + imm);
        break;

    // JALR
    case 0x67:
    {
        int target = (rs1v + imm) & ~1;
        writeRegister(rd, pc);
        setPCValue(target);
        break;
    }

    // Branch (B-type)
    case 0x63:
    {
        bool taken = false;
        int32_t s1 = static_cast<int32_t>(rs1v);
        int32_t s2 = static_cast<int32_t>(rs2v);
        uint32_t u1 = static_cast<uint32_t>(rs1v);
        uint32_t u2 = static_cast<uint32_t>(rs2v);

        switch (funct3) {
        case 0x0: taken = (rs1v == rs2v); break;       // BEQ
        case 0x1: taken = (rs1v != rs2v); break;       // BNE
        case 0x4: taken = (s1 < s2);  break;            // BLT
        case 0x5: taken = (s1 >= s2); break;            // BGE
        case 0x6: taken = (u1 < u2);  break;            // BLTU
        case 0x7: taken = (u1 >= u2); break;            // BGEU
        default:  taken = false; break;
        }

        if (taken)
            setPCValue((pc - 4) + imm);
        break;
    }

    // Load (I-type)
    case 0x03:
    {
        int addr = (rs1v + imm) & 0xFFFFFFFF;
        int val  = 0;
        switch (funct3) {
        case 0x0: val = signExtend(memoryReadByte(addr), 8);  break;  // LB
        case 0x1: val = signExtend(memoryReadHalf(addr), 16); break;  // LH
        case 0x2: val = memoryReadWord(addr); break;                    // LW
        case 0x4: val = memoryReadByte(addr) & 0xFF; break;            // LBU
        case 0x5: val = memoryReadHalf(addr) & 0xFFFF; break;          // LHU
        default:  val = 0; break;
        }
        writeRegister(rd, val);
        break;
    }

    // Store (S-type)
    case 0x23:
    {
        int addr = (rs1v + imm) & 0xFFFFFFFF;
        switch (funct3) {
        case 0x0: memoryWriteByte(addr, rs2v & 0xFF); break;     // SB
        case 0x1: memoryWriteHalf(addr, rs2v & 0xFFFF); break;   // SH
        case 0x2: memoryWriteWord(addr, rs2v); break;             // SW
        default: break;
        }
        break;
    }

    // OP-IMM (I-type)
    case 0x13:
    {
        int shamt = decodedRs2_;
        switch (funct3) {
        case 0x0: writeRegister(rd, rs1v + imm);  break;    // ADDI
        case 0x2: writeRegister(rd, (static_cast<int32_t>(rs1v) < static_cast<int32_t>(imm)) ? 1 : 0); break; // SLTI
        case 0x3: writeRegister(rd, (static_cast<uint32_t>(rs1v) < static_cast<uint32_t>(imm)) ? 1 : 0); break; // SLTIU
        case 0x4: writeRegister(rd, rs1v ^ imm);  break;    // XORI
        case 0x6: writeRegister(rd, rs1v | imm);  break;    // ORI
        case 0x7: writeRegister(rd, rs1v & imm);  break;    // ANDI
        case 0x1: writeRegister(rd, rs1v << shamt); break;  // SLLI
        case 0x5:
            if (funct7 & 0x20)
                writeRegister(rd, static_cast<int>(static_cast<int32_t>(rs1v) >> shamt)); // SRAI
            else
                writeRegister(rd, static_cast<int>(static_cast<uint32_t>(rs1v) >> shamt)); // SRLI
            break;
        default: break;
        }
        break;
    }

    // OP (R-type) — includes M extension
    case 0x33:
    {
        switch (funct3) {
        case 0x0:
            if (funct7 == 0x00)
                writeRegister(rd, rs1v + rs2v);      // ADD
            else if (funct7 == 0x01)
                writeRegister(rd, static_cast<int>(static_cast<int32_t>(rs1v) * static_cast<int32_t>(rs2v))); // MUL
            else if (funct7 == 0x20)
                writeRegister(rd, rs1v - rs2v);      // SUB
            break;
        case 0x1:
            if (funct7 == 0x00)
                writeRegister(rd, rs1v << (rs2v & 0x1F)); // SLL
            else if (funct7 == 0x01) {
                int64_t result = static_cast<int64_t>(static_cast<int32_t>(rs1v)) *
                                  static_cast<int64_t>(static_cast<int32_t>(rs2v)); // MULH
                writeRegister(rd, static_cast<int>(result >> 32));
            }
            break;
        case 0x2:
            if (funct7 == 0x00)
                writeRegister(rd, (static_cast<int32_t>(rs1v) < static_cast<int32_t>(rs2v)) ? 1 : 0); // SLT
            else if (funct7 == 0x01) {
                int64_t result = static_cast<int64_t>(static_cast<int32_t>(rs1v)) *
                                  static_cast<uint64_t>(static_cast<uint32_t>(rs2v)); // MULHSU
                writeRegister(rd, static_cast<int>(result >> 32));
            }
            break;
        case 0x3:
            if (funct7 == 0x00)
                writeRegister(rd, (static_cast<uint32_t>(rs1v) < static_cast<uint32_t>(rs2v)) ? 1 : 0); // SLTU
            else if (funct7 == 0x01) {
                uint64_t result = static_cast<uint64_t>(static_cast<uint32_t>(rs1v)) *
                                   static_cast<uint64_t>(static_cast<uint32_t>(rs2v)); // MULHU
                writeRegister(rd, static_cast<int>(result >> 32));
            }
            break;
        case 0x4:
            if (funct7 == 0x00)
                writeRegister(rd, rs1v ^ rs2v);      // XOR
            else if (funct7 == 0x01) {
                if (rs2v == 0) writeRegister(rd, -1); // DIV
                else if (static_cast<int32_t>(rs1v) == INT32_MIN && static_cast<int32_t>(rs2v) == -1) writeRegister(rd, rs1v);
                else writeRegister(rd, static_cast<int>(static_cast<int32_t>(rs1v) / static_cast<int32_t>(rs2v)));
            }
            break;
        case 0x5:
            if (funct7 == 0x00)
                writeRegister(rd, static_cast<int>(static_cast<uint32_t>(rs1v) >> (rs2v & 0x1F))); // SRL
            else if (funct7 == 0x01) {
                if (rs2v == 0) writeRegister(rd, static_cast<int>(0xFFFFFFFF)); // DIVU
                else writeRegister(rd, static_cast<int>(static_cast<uint32_t>(rs1v) / static_cast<uint32_t>(rs2v)));
            } else if (funct7 == 0x20)
                writeRegister(rd, static_cast<int>(static_cast<int32_t>(rs1v) >> (rs2v & 0x1F))); // SRA
            break;
        case 0x6:
            if (funct7 == 0x00)
                writeRegister(rd, rs1v | rs2v);      // OR
            else if (funct7 == 0x01) {
                if (rs2v == 0) writeRegister(rd, rs1v); // REM
                else if (static_cast<int32_t>(rs1v) == INT32_MIN && static_cast<int32_t>(rs2v) == -1) writeRegister(rd, 0);
                else writeRegister(rd, static_cast<int>(static_cast<int32_t>(rs1v) % static_cast<int32_t>(rs2v)));
            }
            break;
        case 0x7:
            if (funct7 == 0x00)
                writeRegister(rd, rs1v & rs2v);      // AND
            else if (funct7 == 0x01) {
                if (rs2v == 0) writeRegister(rd, rs1v); // REMU
                else writeRegister(rd, static_cast<int>(static_cast<uint32_t>(rs1v) % static_cast<uint32_t>(rs2v)));
            }
            break;
        default: break;
        }
        break;
    }

    // SYSTEM
    case 0x73:
        setRunning(false);
        break;

    default:
        setRunning(false);
        break;
    }

    instructionCount++;
    accessCount += 4;
}
void RV32IMMachine::assemble(QString sourceCode)
{
    running = false;
    buildSuccessful = false;
    buildSuccessfulRV32_ = false;
    firstErrorLine = -1;
    firstErrorLineRV32_ = -1;

    //////////////////////////////////////////////////
    // Regular expressions
    //////////////////////////////////////////////////
    static QRegExp matchComments("(;.*$)");
    static QRegExp validLabel("[a-zA-Z_][a-zA-Z0-9_]*");
    static QRegExp whitespace("\\s+");
    static QRegExp commaSep("\\s*,\\s*");

    //////////////////////////////////////////////////
    // Simplify source code
    //////////////////////////////////////////////////
    QStringList sourceLines = sourceCode.split("\n");
    for (int i = 0; i < sourceLines.size(); i++) {
        sourceLines[i].replace(matchComments, "");
        sourceLines[i] = sourceLines[i].trimmed();
    }

    //////////////////////////////////////////////////
    // Clear assembler data
    //////////////////////////////////////////////////
    assemblerMemory_.clear();
    assemblerLabels_.clear();
    labelPCMap.clear();

    //////////////////////////////////////////////////
    // Instruction encoding helpers (local)
    //////////////////////////////////////////////////
    auto encodeR = [](uint32_t funct7, uint32_t rs2, uint32_t rs1, uint32_t funct3, uint32_t rd, uint32_t opcode) -> uint32_t {
        return (funct7 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
    };
    auto encodeI = [](int imm, uint32_t rs1, uint32_t funct3, uint32_t rd, uint32_t opcode) -> uint32_t {
        return (static_cast<uint32_t>(imm & 0xFFF) << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
    };
    auto encodeS = [](int imm, uint32_t rs2, uint32_t rs1, uint32_t funct3, uint32_t opcode) -> uint32_t {
        uint32_t imm11_5 = (static_cast<uint32_t>(imm) >> 5) & 0x7F;
        uint32_t imm4_0  = static_cast<uint32_t>(imm) & 0x1F;
        return (imm11_5 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (imm4_0 << 7) | opcode;
    };
    auto encodeB = [](int imm, uint32_t rs2, uint32_t rs1, uint32_t funct3, uint32_t opcode) -> uint32_t {
        uint32_t bit12   = (static_cast<uint32_t>(imm) >> 12) & 1;
        uint32_t bit10_5 = (static_cast<uint32_t>(imm) >> 5)  & 0x3F;
        uint32_t bit4_1  = (static_cast<uint32_t>(imm) >> 1)  & 0xF;
        uint32_t bit11   = (static_cast<uint32_t>(imm) >> 11) & 1;
        return (bit12 << 31) | (bit10_5 << 25) | (rs2 << 20) | (rs1 << 15) |
               (funct3 << 12) | (bit4_1 << 8) | (bit11 << 7) | opcode;
    };
    auto encodeU = [](int imm, uint32_t rd, uint32_t opcode) -> uint32_t {
        return (static_cast<uint32_t>(imm) & 0xFFFFF000) | (rd << 7) | opcode;
    };
    auto encodeJ = [](int imm, uint32_t rd, uint32_t opcode) -> uint32_t {
        uint32_t bit20    = (static_cast<uint32_t>(imm) >> 20) & 1;
        uint32_t bit10_1  = (static_cast<uint32_t>(imm) >> 1)  & 0x3FF;
        uint32_t bit11    = (static_cast<uint32_t>(imm) >> 11) & 1;
        uint32_t bit19_12 = (static_cast<uint32_t>(imm) >> 12) & 0xFF;
        return (bit20 << 31) | (bit19_12 << 12) | (bit11 << 20) | (bit10_1 << 21) | (rd << 7) | opcode;
    };

    auto writeWordToAssembler = [&](int addr, uint32_t word) {
        assemblerMemory_[addr]     = static_cast<uint8_t>(word & 0xFF);
        assemblerMemory_[addr + 1] = static_cast<uint8_t>((word >> 8) & 0xFF);
        assemblerMemory_[addr + 2] = static_cast<uint8_t>((word >> 16) & 0xFF);
        assemblerMemory_[addr + 3] = static_cast<uint8_t>((word >> 24) & 0xFF);
    };

    // Parse register from string, return index or -1
    auto parseReg = [&](const QString &s) -> int {
        return resolveRegisterIndex(s.trimmed());
    };

    // Parse immediate value or label
    auto parseImm = [&](const QString &s, bool &ok) -> int {
        ok = true;
        QString t = s.trimmed();
        if (assemblerLabels_.contains(t.toLower()))
            return assemblerLabels_[t.toLower()];
        if (labelPCMap.contains(t.toLower()))
            return labelPCMap[t.toLower()];
        if (t.startsWith("0x", Qt::CaseInsensitive)) {
            int val = t.toInt(&ok, 16);
            if (!ok) { ok = false; return 0; }
            return val;
        }
        int val = t.toInt(&ok, 10);
        if (!ok) { ok = false; return 0; }
        return val;
    };

    // Check if value fits in 12-bit signed immediate
    auto fitsIn12 = [](int val) -> bool {
        return (val >= -2048 && val <= 2047);
    };

    // Compute pseudo-instruction size for pass 1
    auto pseudoSize = [&](const QString &mnemonic, const QString &arguments) -> int {
        if (mnemonic == "li") {
            QStringList parts = arguments.split(commaSep);
            QString immStr = parts.size() > 1 ? parts.last().trimmed() : arguments.trimmed();
            bool ok;
            int val;
            if (immStr.startsWith("0x", Qt::CaseInsensitive))
                val = immStr.mid(2).toInt(&ok, 16);
            else
                val = immStr.toInt(&ok, 10);
            if (ok && fitsIn12(val)) return 4;
            return 8;
        }
        if (mnemonic == "la" || mnemonic == "call" || mnemonic == "tail")
            return 8;
        return 4;
    };

    //////////////////////////////////////////////////
    // FIRST PASS: Scan for labels, calculate addresses
    //////////////////////////////////////////////////
    int currentAddr = 0;

    for (int lineNumber = 0; lineNumber < sourceLines.size(); lineNumber++) {
        try {
            QString line = sourceLines[lineNumber];
            if (line.isEmpty()) continue;

            while (line.contains(":")) {
                QString labelName = line.section(":", 0, 0).trimmed();
                if (!validLabel.exactMatch(labelName))
                    throw Machine::invalidLabel;
                if (assemblerLabels_.contains(labelName.toLower()))
                    throw Machine::duplicateLabel;
                assemblerLabels_.insert(labelName.toLower(), currentAddr);
                labelPCMap.insert(labelName.toLower(), currentAddr);
                line = line.section(":", 1).trimmed();
            }

            if (line.isEmpty()) continue;

            QString mnemonic = line.section(whitespace, 0, 0).toLower();
            QString arguments = line.section(whitespace, 1);

            if (mnemonic == ".org") {
                bool ok;
                int val = parseImm(arguments, ok);
                if (!ok || val < 0 || val % 4 != 0)
                    throw Machine::invalidAddress;
                currentAddr = val;
            } else if (mnemonic == ".word") {
                currentAddr += 4;
            } else if (mnemonic == ".half") {
                currentAddr += 2;
            } else if (mnemonic == ".byte") {
                currentAddr += 1;
            } else if (mnemonic == ".align") {
                bool ok;
                int align = parseImm(arguments, ok);
                if (!ok || align < 0) throw Machine::invalidValue;
                int alignment = 1 << align;
                if (currentAddr % alignment != 0)
                    currentAddr += alignment - (currentAddr % alignment);
            } else if (mnemonic == ".text" || mnemonic == ".data" ||
                       mnemonic == ".globl" || mnemonic == ".global" ||
                       mnemonic == ".section") {
                // No-op
            } else {
                currentAddr += pseudoSize(mnemonic, arguments);
            }
        } catch (Machine::ErrorCode errorCode) {
            if (firstErrorLineRV32_ == -1)
                firstErrorLineRV32_ = lineNumber;
            firstErrorLine = lineNumber;
            emitError(lineNumber, errorCode);
        }
    }

    if (firstErrorLineRV32_ >= 0) return;

    //////////////////////////////////////////////////
    // SECOND PASS: Generate machine code
    //////////////////////////////////////////////////
    currentAddr = 0;
    assemblerMemory_.clear();

    for (int lineNumber = 0; lineNumber < sourceLines.size(); lineNumber++) {
        try {
            QString line = sourceLines[lineNumber];
            if (line.isEmpty()) continue;

            while (line.contains(":"))
                line = line.section(":", 1).trimmed();
            if (line.isEmpty()) continue;

            QString mnemonic = line.section(whitespace, 0, 0).toLower();
            QString arguments = line.section(whitespace, 1);
            QStringList args;
            if (!arguments.trimmed().isEmpty()) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
                args = arguments.split(commaSep, Qt::SkipEmptyParts);
#else
                args = arguments.split(commaSep, QString::SkipEmptyParts);
#endif
            }
            for (int a = 0; a < args.size(); a++)
                args[a] = args[a].trimmed();

            //////////////////////////////////////////////////
            // Directives
            //////////////////////////////////////////////////
            if (mnemonic == ".org") {
                bool ok;
                currentAddr = parseImm(arguments, ok);
                if (!ok) throw Machine::invalidAddress;
                continue;
            }
            if (mnemonic == ".word") {
                bool ok;
                int val = parseImm(arguments, ok);
                if (!ok) throw Machine::invalidValue;
                writeWordToAssembler(currentAddr, static_cast<uint32_t>(val));
                currentAddr += 4;
                continue;
            }
            if (mnemonic == ".half") {
                bool ok;
                int val = parseImm(arguments, ok);
                if (!ok) throw Machine::invalidValue;
                assemblerMemory_[currentAddr]     = static_cast<uint8_t>(val & 0xFF);
                assemblerMemory_[currentAddr + 1] = static_cast<uint8_t>((val >> 8) & 0xFF);
                currentAddr += 2;
                continue;
            }
            if (mnemonic == ".byte") {
                bool ok;
                int val = parseImm(arguments, ok);
                if (!ok) throw Machine::invalidValue;
                assemblerMemory_[currentAddr] = static_cast<uint8_t>(val & 0xFF);
                currentAddr += 1;
                continue;
            }
            if (mnemonic == ".align") {
                bool ok;
                int align = parseImm(arguments, ok);
                if (!ok) throw Machine::invalidValue;
                int alignment = 1 << align;
                if (currentAddr % alignment != 0)
                    currentAddr += alignment - (currentAddr % alignment);
                continue;
            }
            if (mnemonic == ".text" || mnemonic == ".data" ||
                mnemonic == ".globl" || mnemonic == ".global" ||
                mnemonic == ".section")
                continue;

            //////////////////////////////////////////////////
            // Pseudo-instructions
            //////////////////////////////////////////////////
            if (mnemonic == "nop") {
                writeWordToAssembler(currentAddr, encodeI(0, 0, 0, 0, 0x13));
                currentAddr += 4; continue;
            }
            if (mnemonic == "mv") {
                if (args.size() != 2) throw Machine::wrongNumberOfArguments;
                int rd = parseReg(args[0]); if (rd < 0) throw Machine::invalidArgument;
                int rs = parseReg(args[1]); if (rs < 0) throw Machine::invalidArgument;
                writeWordToAssembler(currentAddr, encodeI(0, rs, 0, rd, 0x13));
                currentAddr += 4; continue;
            }
            if (mnemonic == "not") {
                if (args.size() != 2) throw Machine::wrongNumberOfArguments;
                int rd = parseReg(args[0]); if (rd < 0) throw Machine::invalidArgument;
                int rs = parseReg(args[1]); if (rs < 0) throw Machine::invalidArgument;
                writeWordToAssembler(currentAddr, encodeI(-1, rs, 4, rd, 0x13));
                currentAddr += 4; continue;
            }
            if (mnemonic == "neg") {
                if (args.size() != 2) throw Machine::wrongNumberOfArguments;
                int rd = parseReg(args[0]); if (rd < 0) throw Machine::invalidArgument;
                int rs = parseReg(args[1]); if (rs < 0) throw Machine::invalidArgument;
                writeWordToAssembler(currentAddr, encodeR(0x20, rs, 0, 0, rd, 0x33));
                currentAddr += 4; continue;
            }
            if (mnemonic == "seqz") {
                if (args.size() != 2) throw Machine::wrongNumberOfArguments;
                int rd = parseReg(args[0]); if (rd < 0) throw Machine::invalidArgument;
                int rs = parseReg(args[1]); if (rs < 0) throw Machine::invalidArgument;
                writeWordToAssembler(currentAddr, encodeI(1, rs, 3, rd, 0x13));
                currentAddr += 4; continue;
            }
            if (mnemonic == "snez") {
                if (args.size() != 2) throw Machine::wrongNumberOfArguments;
                int rd = parseReg(args[0]); if (rd < 0) throw Machine::invalidArgument;
                int rs = parseReg(args[1]); if (rs < 0) throw Machine::invalidArgument;
                writeWordToAssembler(currentAddr, encodeR(0, rs, 0, 3, rd, 0x33));
                currentAddr += 4; continue;
            }
            if (mnemonic == "sltz") {
                if (args.size() != 2) throw Machine::wrongNumberOfArguments;
                int rd = parseReg(args[0]); if (rd < 0) throw Machine::invalidArgument;
                int rs = parseReg(args[1]); if (rs < 0) throw Machine::invalidArgument;
                writeWordToAssembler(currentAddr, encodeR(0, 0, rs, 2, rd, 0x33));
                currentAddr += 4; continue;
            }
            if (mnemonic == "sgtz") {
                if (args.size() != 2) throw Machine::wrongNumberOfArguments;
                int rd = parseReg(args[0]); if (rd < 0) throw Machine::invalidArgument;
                int rs = parseReg(args[1]); if (rs < 0) throw Machine::invalidArgument;
                writeWordToAssembler(currentAddr, encodeR(0, rs, 0, 2, rd, 0x33));
                currentAddr += 4; continue;
            }
            if (mnemonic == "beqz") {
                if (args.size() != 2) throw Machine::wrongNumberOfArguments;
                int rs = parseReg(args[0]); if (rs < 0) throw Machine::invalidArgument;
                bool ok; int offset = parseImm(args[1], ok); if (!ok) throw Machine::invalidArgument;
                if (assemblerLabels_.contains(args[1].trimmed().toLower()))
                    offset = offset - currentAddr;
                writeWordToAssembler(currentAddr, encodeB(offset, 0, rs, 0, 0x63));
                currentAddr += 4; continue;
            }
            if (mnemonic == "bnez") {
                if (args.size() != 2) throw Machine::wrongNumberOfArguments;
                int rs = parseReg(args[0]); if (rs < 0) throw Machine::invalidArgument;
                bool ok; int offset = parseImm(args[1], ok); if (!ok) throw Machine::invalidArgument;
                if (assemblerLabels_.contains(args[1].trimmed().toLower()))
                    offset = offset - currentAddr;
                writeWordToAssembler(currentAddr, encodeB(offset, 0, rs, 1, 0x63));
                currentAddr += 4; continue;
            }
            if (mnemonic == "blez") {
                if (args.size() != 2) throw Machine::wrongNumberOfArguments;
                int rs = parseReg(args[0]); if (rs < 0) throw Machine::invalidArgument;
                bool ok; int offset = parseImm(args[1], ok); if (!ok) throw Machine::invalidArgument;
                if (assemblerLabels_.contains(args[1].trimmed().toLower()))
                    offset = offset - currentAddr;
                writeWordToAssembler(currentAddr, encodeB(offset, rs, 0, 5, 0x63));
                currentAddr += 4; continue;
            }
            if (mnemonic == "bgez") {
                if (args.size() != 2) throw Machine::wrongNumberOfArguments;
                int rs = parseReg(args[0]); if (rs < 0) throw Machine::invalidArgument;
                bool ok; int offset = parseImm(args[1], ok); if (!ok) throw Machine::invalidArgument;
                if (assemblerLabels_.contains(args[1].trimmed().toLower()))
                    offset = offset - currentAddr;
                writeWordToAssembler(currentAddr, encodeB(offset, 0, rs, 5, 0x63));
                currentAddr += 4; continue;
            }
            if (mnemonic == "bltz") {
                if (args.size() != 2) throw Machine::wrongNumberOfArguments;
                int rs = parseReg(args[0]); if (rs < 0) throw Machine::invalidArgument;
                bool ok; int offset = parseImm(args[1], ok); if (!ok) throw Machine::invalidArgument;
                if (assemblerLabels_.contains(args[1].trimmed().toLower()))
                    offset = offset - currentAddr;
                writeWordToAssembler(currentAddr, encodeB(offset, 0, rs, 4, 0x63));
                currentAddr += 4; continue;
            }
            if (mnemonic == "bgtz") {
                if (args.size() != 2) throw Machine::wrongNumberOfArguments;
                int rs = parseReg(args[0]); if (rs < 0) throw Machine::invalidArgument;
                bool ok; int offset = parseImm(args[1], ok); if (!ok) throw Machine::invalidArgument;
                if (assemblerLabels_.contains(args[1].trimmed().toLower()))
                    offset = offset - currentAddr;
                writeWordToAssembler(currentAddr, encodeB(offset, rs, 0, 4, 0x63));
                currentAddr += 4; continue;
            }
            if (mnemonic == "j") {
                if (args.size() != 1) throw Machine::wrongNumberOfArguments;
                bool ok; int offset = parseImm(args[0], ok); if (!ok) throw Machine::invalidArgument;
                if (assemblerLabels_.contains(args[0].trimmed().toLower()))
                    offset = offset - currentAddr;
                writeWordToAssembler(currentAddr, encodeJ(offset, 0, 0x6F));
                currentAddr += 4; continue;
            }
            if (mnemonic == "jal") {
                if (args.size() == 1) {
                    bool ok; int offset = parseImm(args[0], ok); if (!ok) throw Machine::invalidArgument;
                    if (assemblerLabels_.contains(args[0].trimmed().toLower()))
                        offset = offset - currentAddr;
                    writeWordToAssembler(currentAddr, encodeJ(offset, 1, 0x6F));
                    currentAddr += 4;
                } else if (args.size() == 2) {
                    int rd = parseReg(args[0]); if (rd < 0) throw Machine::invalidArgument;
                    bool ok; int offset = parseImm(args[1], ok); if (!ok) throw Machine::invalidArgument;
                    if (assemblerLabels_.contains(args[1].trimmed().toLower()))
                        offset = offset - currentAddr;
                    writeWordToAssembler(currentAddr, encodeJ(offset, rd, 0x6F));
                    currentAddr += 4;
                } else throw Machine::wrongNumberOfArguments;
                continue;
            }
            if (mnemonic == "jr") {
                if (args.size() != 1) throw Machine::wrongNumberOfArguments;
                int rs = parseReg(args[0]); if (rs < 0) throw Machine::invalidArgument;
                writeWordToAssembler(currentAddr, encodeI(0, rs, 0, 0, 0x67));
                currentAddr += 4; continue;
            }
            if (mnemonic == "jalr") {
                if (args.size() == 1) {
                    int rs = parseReg(args[0]); if (rs < 0) throw Machine::invalidArgument;
                    writeWordToAssembler(currentAddr, encodeI(0, rs, 0, 1, 0x67));
                    currentAddr += 4;
                } else if (args.size() == 2) {
                    int rd = parseReg(args[0]); if (rd < 0) throw Machine::invalidArgument;
                    static QRegExp offsetReg("(-?\\d+)\\(([^)]+)\\)");
                    if (offsetReg.exactMatch(args[1])) {
                        int imm = offsetReg.cap(1).toInt();
                        int rs = parseReg(offsetReg.cap(2)); if (rs < 0) throw Machine::invalidArgument;
                        writeWordToAssembler(currentAddr, encodeI(imm, rs, 0, rd, 0x67));
                    } else {
                        int rs = parseReg(args[1]); if (rs < 0) throw Machine::invalidArgument;
                        writeWordToAssembler(currentAddr, encodeI(0, rs, 0, rd, 0x67));
                    }
                    currentAddr += 4;
                } else if (args.size() == 3) {
                    int rd = parseReg(args[0]); if (rd < 0) throw Machine::invalidArgument;
                    int rs = parseReg(args[1]); if (rs < 0) throw Machine::invalidArgument;
                    bool ok; int imm = parseImm(args[2], ok); if (!ok) throw Machine::invalidArgument;
                    writeWordToAssembler(currentAddr, encodeI(imm, rs, 0, rd, 0x67));
                    currentAddr += 4;
                } else throw Machine::wrongNumberOfArguments;
                continue;
            }
            if (mnemonic == "ret") {
                writeWordToAssembler(currentAddr, encodeI(0, 1, 0, 0, 0x67));
                currentAddr += 4; continue;
            }
            if (mnemonic == "call") {
                if (args.size() != 1) throw Machine::wrongNumberOfArguments;
                bool ok; int target = parseImm(args[0], ok); if (!ok) throw Machine::invalidArgument;
                int offset = target - currentAddr;
                int upper = (offset + 0x800) >> 12;
                int lower = offset - (upper << 12);
                writeWordToAssembler(currentAddr, encodeU(upper << 12, 1, 0x17));
                writeWordToAssembler(currentAddr + 4, encodeI(lower, 1, 0, 1, 0x67));
                currentAddr += 8; continue;
            }
            if (mnemonic == "tail") {
                if (args.size() != 1) throw Machine::wrongNumberOfArguments;
                bool ok; int target = parseImm(args[0], ok); if (!ok) throw Machine::invalidArgument;
                int offset = target - currentAddr;
                int upper = (offset + 0x800) >> 12;
                int lower = offset - (upper << 12);
                writeWordToAssembler(currentAddr, encodeU(upper << 12, 6, 0x17));
                writeWordToAssembler(currentAddr + 4, encodeI(lower, 6, 0, 0, 0x67));
                currentAddr += 8; continue;
            }
            if (mnemonic == "li") {
                if (args.size() != 2) throw Machine::wrongNumberOfArguments;
                int rd = parseReg(args[0]); if (rd < 0) throw Machine::invalidArgument;
                bool ok; int val = parseImm(args[1], ok); if (!ok) throw Machine::invalidValue;
                if (fitsIn12(val)) {
                    writeWordToAssembler(currentAddr, encodeI(val, 0, 0, rd, 0x13));
                    currentAddr += 4;
                } else {
                    int upper = (val + 0x800) >> 12;
                    int lower = val - (upper << 12);
                    writeWordToAssembler(currentAddr, encodeU(upper << 12, rd, 0x37));
                    writeWordToAssembler(currentAddr + 4, encodeI(lower, rd, 0, rd, 0x13));
                    currentAddr += 8;
                }
                continue;
            }
            if (mnemonic == "la") {
                if (args.size() != 2) throw Machine::wrongNumberOfArguments;
                int rd = parseReg(args[0]); if (rd < 0) throw Machine::invalidArgument;
                bool ok; int target = parseImm(args[1], ok); if (!ok) throw Machine::invalidArgument;
                int offset = target - currentAddr;
                int upper = (offset + 0x800) >> 12;
                int lower = offset - (upper << 12);
                writeWordToAssembler(currentAddr, encodeU(upper << 12, rd, 0x17));
                writeWordToAssembler(currentAddr + 4, encodeI(lower, rd, 0, rd, 0x13));
                currentAddr += 8; continue;
            }
            if (mnemonic == "csrr") {
                if (args.size() != 2) throw Machine::wrongNumberOfArguments;
                int rd = parseReg(args[0]); if (rd < 0) throw Machine::invalidArgument;
                bool ok; int csr = parseImm(args[1], ok); if (!ok) throw Machine::invalidArgument;
                writeWordToAssembler(currentAddr, encodeI(csr, 0, 1, rd, 0x73));
                currentAddr += 4; continue;
            }
            if (mnemonic == "csrw" || mnemonic == "csrs") {
                if (args.size() != 2) throw Machine::wrongNumberOfArguments;
                bool ok; int csr = parseImm(args[0], ok); if (!ok) throw Machine::invalidArgument;
                int rs = parseReg(args[1]); if (rs < 0) throw Machine::invalidArgument;
                writeWordToAssembler(currentAddr, encodeI(csr, rs, 1, 0, 0x73));
                currentAddr += 4; continue;
            }
            if (mnemonic == "csrc") {
                if (args.size() != 2) throw Machine::wrongNumberOfArguments;
                bool ok; int csr = parseImm(args[0], ok); if (!ok) throw Machine::invalidArgument;
                int rs = parseReg(args[1]); if (rs < 0) throw Machine::invalidArgument;
                writeWordToAssembler(currentAddr, encodeI(csr, rs, 2, 0, 0x73));
                currentAddr += 4; continue;
            }

            //////////////////////////////////////////////////
            // Real RV32IM instructions
            //////////////////////////////////////////////////

            // U-type: LUI, AUIPC
            if (mnemonic == "lui") {
                if (args.size() != 2) throw Machine::wrongNumberOfArguments;
                int rd = parseReg(args[0]); if (rd < 0) throw Machine::invalidArgument;
                bool ok; int imm = parseImm(args[1], ok); if (!ok) throw Machine::invalidValue;
                writeWordToAssembler(currentAddr, encodeU(imm, rd, 0x37));
                currentAddr += 4; continue;
            }
            if (mnemonic == "auipc") {
                if (args.size() != 2) throw Machine::wrongNumberOfArguments;
                int rd = parseReg(args[0]); if (rd < 0) throw Machine::invalidArgument;
                bool ok; int imm = parseImm(args[1], ok); if (!ok) throw Machine::invalidValue;
                writeWordToAssembler(currentAddr, encodeU(imm, rd, 0x17));
                currentAddr += 4; continue;
            }

            // B-type: Branch
            if (mnemonic == "beq" || mnemonic == "bne" ||
                mnemonic == "blt" || mnemonic == "bge" ||
                mnemonic == "bltu" || mnemonic == "bgeu") {
                if (args.size() != 3) throw Machine::wrongNumberOfArguments;
                int rs1 = parseReg(args[0]); if (rs1 < 0) throw Machine::invalidArgument;
                int rs2 = parseReg(args[1]); if (rs2 < 0) throw Machine::invalidArgument;
                bool ok; int offset = parseImm(args[2], ok); if (!ok) throw Machine::invalidArgument;
                if (assemblerLabels_.contains(args[2].trimmed().toLower()))
                    offset = offset - currentAddr;
                uint32_t funct3 = 0;
                if (mnemonic == "beq")  funct3 = 0;
                else if (mnemonic == "bne")  funct3 = 1;
                else if (mnemonic == "blt")  funct3 = 4;
                else if (mnemonic == "bge")  funct3 = 5;
                else if (mnemonic == "bltu") funct3 = 6;
                else if (mnemonic == "bgeu") funct3 = 7;
                writeWordToAssembler(currentAddr, encodeB(offset, rs2, rs1, funct3, 0x63));
                currentAddr += 4; continue;
            }

            // I-type: Load
            if (mnemonic == "lb" || mnemonic == "lh" || mnemonic == "lw" ||
                mnemonic == "lbu" || mnemonic == "lhu") {
                if (args.size() != 2) throw Machine::wrongNumberOfArguments;
                int rd = parseReg(args[0]); if (rd < 0) throw Machine::invalidArgument;
                static QRegExp offsetReg("(-?\\w+)\\(([^)]+)\\)");
                int imm = 0; int rs1 = -1;
                if (offsetReg.exactMatch(args[1])) {
                    bool ok; imm = parseImm(offsetReg.cap(1), ok); if (!ok) imm = 0;
                    rs1 = parseReg(offsetReg.cap(2));
                } else {
                    rs1 = parseReg(args[1]);
                    if (rs1 < 0) {
                        bool ok; imm = parseImm(args[1], ok); if (!ok) throw Machine::invalidArgument;
                        rs1 = 0;
                    }
                }
                if (rs1 < 0) throw Machine::invalidArgument;
                uint32_t funct3 = 0;
                if (mnemonic == "lb")  funct3 = 0;
                else if (mnemonic == "lh")  funct3 = 1;
                else if (mnemonic == "lw")  funct3 = 2;
                else if (mnemonic == "lbu") funct3 = 4;
                else if (mnemonic == "lhu") funct3 = 5;
                writeWordToAssembler(currentAddr, encodeI(imm, rs1, funct3, rd, 0x03));
                currentAddr += 4; continue;
            }

            // S-type: Store
            if (mnemonic == "sb" || mnemonic == "sh" || mnemonic == "sw") {
                if (args.size() != 2) throw Machine::wrongNumberOfArguments;
                int rs2 = parseReg(args[0]); if (rs2 < 0) throw Machine::invalidArgument;
                static QRegExp offsetRegS("(-?\\w+)\\(([^)]+)\\)");
                int imm = 0; int rs1 = -1;
                if (offsetRegS.exactMatch(args[1])) {
                    bool ok; imm = parseImm(offsetRegS.cap(1), ok); if (!ok) imm = 0;
                    rs1 = parseReg(offsetRegS.cap(2));
                } else {
                    rs1 = parseReg(args[1]);
                    if (rs1 < 0) {
                        bool ok; imm = parseImm(args[1], ok); if (!ok) throw Machine::invalidArgument;
                        rs1 = 0;
                    }
                }
                if (rs1 < 0) throw Machine::invalidArgument;
                uint32_t funct3 = 0;
                if (mnemonic == "sb") funct3 = 0;
                else if (mnemonic == "sh") funct3 = 1;
                else if (mnemonic == "sw") funct3 = 2;
                writeWordToAssembler(currentAddr, encodeS(imm, rs2, rs1, funct3, 0x23));
                currentAddr += 4; continue;
            }

            // I-type: OP-IMM
            if (mnemonic == "addi" || mnemonic == "slti" || mnemonic == "sltiu" ||
                mnemonic == "xori" || mnemonic == "ori" || mnemonic == "andi" ||
                mnemonic == "slli" || mnemonic == "srli" || mnemonic == "srai") {
                if (args.size() != 3) throw Machine::wrongNumberOfArguments;
                int rd = parseReg(args[0]); if (rd < 0) throw Machine::invalidArgument;
                int rs1 = parseReg(args[1]); if (rs1 < 0) throw Machine::invalidArgument;
                bool ok; int imm = parseImm(args[2], ok); if (!ok) throw Machine::invalidValue;
                uint32_t funct3 = 0; uint32_t funct7_enc = 0;
                if (mnemonic == "addi")       funct3 = 0;
                else if (mnemonic == "slti")  funct3 = 2;
                else if (mnemonic == "sltiu") funct3 = 3;
                else if (mnemonic == "xori")  funct3 = 4;
                else if (mnemonic == "ori")   funct3 = 6;
                else if (mnemonic == "andi")  funct3 = 7;
                else if (mnemonic == "slli")  { funct3 = 1; imm = imm & 0x1F; }
                else if (mnemonic == "srli")  { funct3 = 5; imm = imm & 0x1F; }
else if (mnemonic == "srai") { funct3 = 5; imm = (0x20 << 5) | (imm & 0x1F); }
                writeWordToAssembler(currentAddr, encodeI(imm, rs1, funct3, rd, 0x13));
                currentAddr += 4; continue;
            }

            // R-type: OP (arithmetic/logic + M extension)
            if (mnemonic == "add" || mnemonic == "sub" ||
                mnemonic == "sll" || mnemonic == "slt" || mnemonic == "sltu" ||
                mnemonic == "xor" || mnemonic == "srl" || mnemonic == "sra" ||
                mnemonic == "or" || mnemonic == "and" ||
                mnemonic == "mul" || mnemonic == "mulh" || mnemonic == "mulhsu" ||
                mnemonic == "mulhu" || mnemonic == "div" || mnemonic == "divu" ||
                mnemonic == "rem" || mnemonic == "remu") {
                if (args.size() != 3) throw Machine::wrongNumberOfArguments;
                int rd = parseReg(args[0]); if (rd < 0) throw Machine::invalidArgument;
                int rs1 = parseReg(args[1]); if (rs1 < 0) throw Machine::invalidArgument;
                int rs2 = parseReg(args[2]); if (rs2 < 0) throw Machine::invalidArgument;
                uint32_t funct3 = 0; uint32_t funct7_enc = 0;
                if (mnemonic == "add")         { funct3 = 0; funct7_enc = 0x00; }
                else if (mnemonic == "sub")    { funct3 = 0; funct7_enc = 0x20; }
                else if (mnemonic == "sll")    { funct3 = 1; funct7_enc = 0x00; }
                else if (mnemonic == "slt")    { funct3 = 2; funct7_enc = 0x00; }
                else if (mnemonic == "sltu")   { funct3 = 3; funct7_enc = 0x00; }
                else if (mnemonic == "xor")    { funct3 = 4; funct7_enc = 0x00; }
                else if (mnemonic == "srl")    { funct3 = 5; funct7_enc = 0x00; }
                else if (mnemonic == "sra")    { funct3 = 5; funct7_enc = 0x20; }
                else if (mnemonic == "or")     { funct3 = 6; funct7_enc = 0x00; }
                else if (mnemonic == "and")    { funct3 = 7; funct7_enc = 0x00; }
                else if (mnemonic == "mul")    { funct3 = 0; funct7_enc = 0x01; }
                else if (mnemonic == "mulh")   { funct3 = 1; funct7_enc = 0x01; }
                else if (mnemonic == "mulhsu") { funct3 = 2; funct7_enc = 0x01; }
                else if (mnemonic == "mulhu")  { funct3 = 3; funct7_enc = 0x01; }
                else if (mnemonic == "div")    { funct3 = 4; funct7_enc = 0x01; }
                else if (mnemonic == "divu")   { funct3 = 5; funct7_enc = 0x01; }
                else if (mnemonic == "rem")    { funct3 = 6; funct7_enc = 0x01; }
                else if (mnemonic == "remu")   { funct3 = 7; funct7_enc = 0x01; }
                writeWordToAssembler(currentAddr, encodeR(funct7_enc, rs2, rs1, funct3, rd, 0x33));
                currentAddr += 4; continue;
            }

            // ECALL / EBREAK
            if (mnemonic == "ecall") {
                writeWordToAssembler(currentAddr, encodeI(0, 0, 0, 0, 0x73));
                currentAddr += 4; continue;
            }
            if (mnemonic == "ebreak") {
                writeWordToAssembler(currentAddr, encodeI(1, 0, 0, 0, 0x73));
                currentAddr += 4; continue;
            }

            // Unknown mnemonic
            throw Machine::invalidInstruction;

        } catch (Machine::ErrorCode errorCode) {
            if (firstErrorLineRV32_ == -1)
                firstErrorLineRV32_ = lineNumber;
            firstErrorLine = lineNumber;
            emitError(lineNumber, errorCode);
        }
    }

    if (firstErrorLineRV32_ >= 0) return;

    //////////////////////////////////////////////////
    // Copy assembler memory to sparse memory
    //////////////////////////////////////////////////
    for (auto it = assemblerMemory_.constBegin(); it != assemblerMemory_.constEnd(); ++it) {
        setMemoryValue(it.key(), it.value());
    }

    buildSuccessful = true;
    buildSuccessfulRV32_ = true;
    clearAfterBuild();
}
void RV32IMMachine::updateInstructionStrings()
{
    clearInstructionStrings();
    // Collect word-aligned start addresses from byte-level sparse memory keys.
    // A word-aligned address might not appear directly in sparseMemory_ if
    // the byte at that offset is zero (setMemoryValue removes zero entries),
    // so we round each byte address down to the nearest 4-byte boundary.
    QSet<int> wordAddrs;
    for (int addr : sparseMemory_.keys()) {
        wordAddrs.insert(addr & ~3);
    }
    QList<int> sortedAddrs = wordAddrs.values();
    std::sort(sortedAddrs.begin(), sortedAddrs.end());
    for (int addr : sortedAddrs) {
        int argsSize = 0;
        QString str = generateInstructionString(addr, argsSize);
        if (!str.isEmpty()) {
            instructionStrings_[addr] = str;
        }
    }
}

QString RV32IMMachine::generateInstructionString(int address, int &argumentsSize)
{
    argumentsSize = 3; // Remaining 3 bytes after the first in a 4-byte instruction

    // Read the 32-bit word at this address
    uint32_t inst = static_cast<uint32_t>(memoryReadWord(address));
    if (inst == 0)
        return "";

    int opcode = inst & 0x7F;
    int rd     = (inst >> 7)  & 0x1F;
    int rs1    = (inst >> 15) & 0x1F;
    int rs2    = (inst >> 20) & 0x1F;
    int funct3 = (inst >> 12) & 0x7;
    int funct7 = (inst >> 25) & 0x7F;

    auto regName = [this](int idx) -> QString { return resolveRegisterName(idx); };

    switch (opcode) {
    case 0x37: // LUI
        return QString("LUI %1, 0x%2").arg(regName(rd)).arg(static_cast<uint32_t>(inst & 0xFFFFF000), 8, 16, QChar('0'));
    case 0x17: // AUIPC
        return QString("AUIPC %1, 0x%2").arg(regName(rd)).arg(static_cast<uint32_t>(inst & 0xFFFFF000), 8, 16, QChar('0'));
    case 0x6F: // JAL
    {
        int bit20    = (inst >> 31) & 1;
        int bit10_1  = (inst >> 21) & 0x3FF;
        int bit11    = (inst >> 20) & 1;
        int bit19_12 = (inst >> 12) & 0xFF;
        int offset = (bit20 << 20) | (bit19_12 << 12) | (bit11 << 11) | (bit10_1 << 1);
        offset = signExtend(offset, 21);
        return QString("JAL %1, %2").arg(regName(rd)).arg(offset);
    }
    case 0x67: // JALR
    {
        int imm = (inst >> 20) & 0xFFF;
        imm = signExtend(imm, 12);
        return QString("JALR %1, %2(%3)").arg(regName(rd)).arg(imm).arg(regName(rs1));
    }
    case 0x63: // Branch
    {
        int bit12   = (inst >> 31) & 1;
        int bit10_5 = (inst >> 25) & 0x3F;
        int bit4_1  = (inst >> 8)  & 0xF;
        int bit11   = (inst >> 7)  & 1;
        int offset = (bit12 << 12) | (bit11 << 11) | (bit10_5 << 5) | (bit4_1 << 1);
        offset = signExtend(offset, 13);
        QString mnemonic;
        switch (funct3) {
        case 0: mnemonic = "BEQ"; break;
        case 1: mnemonic = "BNE"; break;
        case 4: mnemonic = "BLT"; break;
        case 5: mnemonic = "BGE"; break;
        case 6: mnemonic = "BLTU"; break;
        case 7: mnemonic = "BGEU"; break;
        default: mnemonic = "B.?"; break;
        }
        return QString("%1 %2, %3, %4").arg(mnemonic).arg(regName(rs1)).arg(regName(rs2)).arg(offset);
    }
    case 0x03: // Load
    {
        int imm = (inst >> 20) & 0xFFF;
        imm = signExtend(imm, 12);
        QString mnemonic;
        switch (funct3) {
        case 0: mnemonic = "LB"; break;
        case 1: mnemonic = "LH"; break;
        case 2: mnemonic = "LW"; break;
        case 4: mnemonic = "LBU"; break;
        case 5: mnemonic = "LHU"; break;
        default: mnemonic = "L.?"; break;
        }
        return QString("%1 %2, %3(%4)").arg(mnemonic).arg(regName(rd)).arg(imm).arg(regName(rs1));
    }
    case 0x23: // Store
    {
        int imm11_5 = (inst >> 25) & 0x7F;
        int imm4_0  = (inst >> 7)  & 0x1F;
        int imm = (imm11_5 << 5) | imm4_0;
        imm = signExtend(imm, 12);
        QString mnemonic;
        switch (funct3) {
        case 0: mnemonic = "SB"; break;
        case 1: mnemonic = "SH"; break;
        case 2: mnemonic = "SW"; break;
        default: mnemonic = "S.?"; break;
        }
        return QString("%1 %2, %3(%4)").arg(mnemonic).arg(regName(rs2)).arg(imm).arg(regName(rs1));
    }
    case 0x13: // OP-IMM
    {
        int imm = (inst >> 20) & 0xFFF;
        imm = signExtend(imm, 12);
        int shamt = rs2;
        QString mnemonic;
        switch (funct3) {
        case 0: mnemonic = "ADDI"; break;
        case 1: mnemonic = "SLLI"; return QString("SLLI %1, %2, %3").arg(regName(rd)).arg(regName(rs1)).arg(shamt);
        case 2: mnemonic = "SLTI"; break;
        case 3: mnemonic = "SLTIU"; break;
        case 4: mnemonic = "XORI"; break;
        case 5:
            if (funct7 & 0x20) return QString("SRAI %1, %2, %3").arg(regName(rd)).arg(regName(rs1)).arg(shamt);
            else return QString("SRLI %1, %2, %3").arg(regName(rd)).arg(regName(rs1)).arg(shamt);
        case 6: mnemonic = "ORI"; break;
        case 7: mnemonic = "ANDI"; break;
        default: mnemonic = "OP-IMM.?"; break;
        }
        return QString("%1 %2, %3, %4").arg(mnemonic).arg(regName(rd)).arg(regName(rs1)).arg(imm);
    }
    case 0x33: // OP (R-type + M extension)
    {
        QString mnemonic;
        if (funct7 == 0x01) {
            // M extension
            switch (funct3) {
            case 0: mnemonic = "MUL"; break;
            case 1: mnemonic = "MULH"; break;
            case 2: mnemonic = "MULHSU"; break;
            case 3: mnemonic = "MULHU"; break;
            case 4: mnemonic = "DIV"; break;
            case 5: mnemonic = "DIVU"; break;
            case 6: mnemonic = "REM"; break;
            case 7: mnemonic = "REMU"; break;
            default: mnemonic = "M.?"; break;
            }
        } else {
            switch (funct3) {
            case 0: mnemonic = (funct7 == 0x20) ? "SUB" : "ADD"; break;
            case 1: mnemonic = "SLL"; break;
            case 2: mnemonic = "SLT"; break;
            case 3: mnemonic = "SLTU"; break;
            case 4: mnemonic = "XOR"; break;
            case 5: mnemonic = (funct7 == 0x20) ? "SRA" : "SRL"; break;
            case 6: mnemonic = "OR"; break;
            case 7: mnemonic = "AND"; break;
            default: mnemonic = "OP.?"; break;
            }
        }
        return QString("%1 %2, %3, %4").arg(mnemonic).arg(regName(rd)).arg(regName(rs1)).arg(regName(rs2));
    }
    case 0x73: // SYSTEM
    {
        int imm = (inst >> 20) & 0xFFF;
        if (imm == 0) return "ECALL";
        if (imm == 1) return "EBREAK";
        return "SYSTEM";
    }
    default:
        return QString("??? 0x%1").arg(inst, 8, 16, QChar('0'));
    }
}
QString RV32IMMachine::generateArgumentsString(int, Instruction*, AddressingMode::AddressingModeCode, int &) { return ""; }

//////////////////////////////////////////////////
// Override breakpoint access
//////////////////////////////////////////////////
int RV32IMMachine::getBreakpoint() const {
    return breakpoint;
}

void RV32IMMachine::setBreakpoint(int value) {
    breakpoint = value;
}

//////////////////////////////////////////////////
// Override label/source-line mapping
//////////////////////////////////////////////////
QString RV32IMMachine::getAddressCorrespondingLabel(int address) {
    return addressCorrespondingLabel.value(address, "");
}

int RV32IMMachine::getAddressCorrespondingSourceLine(int address) {
    return addressCorrespondingSourceLine.value(address, -1);
}

int RV32IMMachine::getSourceLineCorrespondingAddress(int line) {
    return sourceLineCorrespondingAddress.value(line, -1);
}

int RV32IMMachine::getPCCorrespondingSourceLine() {
    return getAddressCorrespondingSourceLine(getPCValue());
}
