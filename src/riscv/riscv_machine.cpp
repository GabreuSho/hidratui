#include "riscv_machine.h"
#include <QDebug>

namespace riscv {

RiscVMachine::RiscVMachine(QObject *parent)
    : Machine(parent)
    , regFile_(this)
    , decoder_(this)
    , alu_(this)
    , fetchedInstr_(0)
    , aluResult_(0)
    , memoryData_(0)
    , nextPC_(0)
    , controlHazard_(false)
    , isLoad_(false)
    , isStore_(false)
    , isBranch_(false)
    , isJump_(false)
    , branchTaken_(false)
    , littleEndian_(true)  // RISC-V é little-endian por padrão
    , startAddress_(0)
    , cycleCount_(0)
    , instructionCount_(0)
    , memoryAccessCount_(0)
{
    initialize();
}

RiscVMachine::~RiscVMachine() {
    // Qt cuida da limpeza dos objetos filhos via parent-child relationship
}

void RiscVMachine::initialize() {
    // Configura identificador único para esta máquina
    identifier = "RISCV";
    
    // Configura memória (padrão 4KB, pode ser ajustado)
    setMemorySize(4096);
    
    // Configura endianness (RISC-V é little-endian)
    littleEndian_ = true;
    
    // Inicializa registradores através do RegisterFile
    // Nota: Não usamos o sistema de registradores da classe base Machine
    // porque RISC-V tem 32 GPRs fixos, diferente das máquinas existentes
    
    // Limpa estado inicial
    clear();
    
    qDebug() << "RiscVMachine initialized:"
             << "Memory size:" << getMemorySize()
             << "Endianness:" << (littleEndian_ ? "little" : "big");
}

void RiscVMachine::reset() {
    // Reseta register file (inclui PC e CSRs)
    regFile_.reset();
    regFile_.resetPC(startAddress_);
    
    // Reseta estado de execução
    fetchedInstr_ = 0;
    currentInstr_ = DecodedInstruction();
    aluResult_ = 0;
    memoryData_ = 0;
    nextPC_ = startAddress_;
    controlHazard_ = false;
    
    isLoad_ = false;
    isStore_ = false;
    isBranch_ = false;
    isJump_ = false;
    branchTaken_ = false;
    
    // Reseta contadores
    cycleCount_ = 0;
    instructionCount_ = 0;
    memoryAccessCount_ = 0;
    
    // Limpa flags da classe base
    clearFlags();
    
    qDebug() << "RiscVMachine reset to address:" << QString("0x%1").arg(startAddress_, 8, 16, QChar('0'));
}

void RiscVMachine::clear() {
    // Limpa memória
    clearMemory();
    
    // Limpa strings de instrução
    clearInstructionStrings();
    
    // Reseta estado
    reset();
}

void RiscVMachine::setStartAddress(uint32_t address) {
    // Alinha endereço a 4 bytes
    startAddress_ = address & 0xFFFFFFFCu;
}

void RiscVMachine::step() {
    if (!isRunning()) {
        return;
    }
    
    // Incrementa contador de ciclos
    cycleCount_++;
    
    // Ciclo single-cycle completo:
    // 1. Fetch
    if (!fetch()) {
        // Erro no fetch (ex: acesso inválido à memória)
        setRunning(false);
        return;
    }
    
    // 2. Decode (feito no fetch para eficiência)
    currentInstr_ = decoder_.decode(fetchedInstr_);
    
    // Atualiza estado para as próximas fases
    isLoad_ = currentInstr_.isLoad;
    isStore_ = currentInstr_.isStore;
    isBranch_ = currentInstr_.isBranch;
    isJump_ = currentInstr_.isJump;
    branchTaken_ = false;
    
    // 3. Execute
    execute();
    
    // 4. Memory Access
    if (isLoad_ || isStore_) {
        memoryAccess();
    }
    
    // 5. Writeback
    writeBack();
    
    // 6. Update PC
    updatePC();
    
    // Incrementa contador de instruções
    instructionCount_++;
    
    // Emite signal para UI/debugging
    emit instructionExecuted(regFile_.getPC(), currentInstr_);
    
    // Verifica breakpoint
    if (regFile_.getPC() == static_cast<uint32_t>(getBreakpoint())) {
        setRunning(false);
    }
}

void RiscVMachine::decodeInstruction() {
    // Decode já é feito no step(), mas mantemos este método
    // para compatibilidade com a interface Machine
    
    // O decode real acontece em step() após o fetch
    // Este método pode ser usado para re-decode se necessário
    if (fetchedInstr_ != 0) {
        currentInstr_ = decoder_.decode(fetchedInstr_);
    }
}

void RiscVMachine::executeInstruction() {
    // Execução já é feita no step(), mas mantemos este método
    // para compatibilidade com a interface Machine
    
    execute();
    
    if (isLoad_ || isStore_) {
        memoryAccess();
    }
    
    writeBack();
}

bool RiscVMachine::fetch() {
    uint32_t pc = regFile_.getPC();
    
    // Verifica alinhamento do PC
    if ((pc & 0x3) != 0) {
        qDebug() << "Instruction address misaligned:" << QString("0x%1").arg(pc, 8, 16, QChar('0'));
        emit trapOccurred(0, pc);  // Exception code 0 = Instruction address misaligned
        return false;
    }
    
    // Lê 4 bytes da memória
    bool success = false;
    fetchedInstr_ = readWord(pc, success);
    
    if (!success) {
        qDebug() << "Instruction fetch failed at address:" << QString("0x%1").arg(pc, 8, 16, QChar('0'));
        emit memoryAccessFault(pc, false);
        return false;
    }
    
    // Valida instrução
    if (!RiscVDecoder::isValidInstruction(fetchedInstr_)) {
        qDebug() << "Invalid instruction:" << QString("0x%1").arg(fetchedInstr_, 8, 16, QChar('0'));
        emit decoder_.invalidInstruction(pc, fetchedInstr_);
        // Continua execução mas marca como instrução inválida
    }
    
    return true;
}

void RiscVMachine::execute() {
    uint32_t operandA = 0;
    uint32_t operandB = 0;
    
    // Prepara operandos baseados no tipo de instrução
    switch (currentInstr_.type) {
        case InstrType::R_TYPE:
            // R-type: operandos vêm de dois registradores
            operandA = regFile_.readRegister(currentInstr_.rs1);
            operandB = regFile_.readRegister(currentInstr_.rs2);
            break;
            
        case InstrType::I_TYPE:
            if (currentInstr_.isLoad) {
                // Load: operandA = rs1, operandB = immediate (para cálculo de endereço)
                operandA = regFile_.readRegister(currentInstr_.rs1);
                operandB = static_cast<uint32_t>(currentInstr_.immediate);
            } else if (currentInstr_.isJump && currentInstr_.opcode == Opcode::OP_JALR) {
                // JALR: operandA = rs1, operandB = immediate
                operandA = regFile_.readRegister(currentInstr_.rs1);
                operandB = static_cast<uint32_t>(currentInstr_.immediate);
            } else {
                // ALU immediate: operandA = rs1, operandB = immediate
                operandA = regFile_.readRegister(currentInstr_.rs1);
                operandB = static_cast<uint32_t>(currentInstr_.immediate);
            }
            break;
            
        case InstrType::S_TYPE:
            // Store: operandA = rs1 (base), operandB = rs2 (valor a armazenar)
            // O imediato é usado para cálculo de endereço
            operandA = regFile_.readRegister(currentInstr_.rs1);
            operandB = regFile_.readRegister(currentInstr_.rs2);
            break;
            
        case InstrType::B_TYPE:
            // Branch: operandA = rs1, operandB = rs2 (para comparação)
            operandA = regFile_.readRegister(currentInstr_.rs1);
            operandB = regFile_.readRegister(currentInstr_.rs2);
            break;
            
        case InstrType::U_TYPE:
            // U-type (LUI/AUIPC): operandB = immediate upper
            operandA = 0;
            operandB = static_cast<uint32_t>(currentInstr_.immediate);
            
            // Caso especial: AUIPC usa PC atual como base
            if (currentInstr_.opcode == Opcode::OP_AUIPC) {
                operandA = regFile_.getPC();
            }
            break;
            
        case InstrType::J_TYPE:
            // J-type (JAL): operandA = PC, operandB = immediate offset
            operandA = regFile_.getPC();
            operandB = static_cast<uint32_t>(currentInstr_.immediate);
            break;
            
        default:
            break;
    }
    
    // Executa operação na ALU
    ALUResult aluResult = alu_.execute(currentInstr_.aluOp, operandA, operandB);
    aluResult_ = aluResult.result;
    
    // Para branches, avalia condição
    if (isBranch_) {
        bool condition = false;
        
        int32_t a = static_cast<int32_t>(operandA);
        int32_t b = static_cast<int32_t>(operandB);
        uint32_t au = operandA;
        uint32_t bu = operandB;
        
        switch (static_cast<Funct3>(currentInstr_.funct3)) {
            case Funct3::BRANCH_BEQ:
                condition = (a == b);
                break;
            case Funct3::BRANCH_BNE:
                condition = (a != b);
                break;
            case Funct3::BRANCH_BLT:
                condition = (a < b);
                break;
            case Funct3::BRANCH_BGE:
                condition = (a >= b);
                break;
            case Funct3::BRANCH_BLTU:
                condition = (au < bu);
                break;
            case Funct3::BRANCH_BGEU:
                condition = (au >= bu);
                break;
            default:
                break;
        }
        
        branchTaken_ = condition;
        
        if (condition) {
            // Branch tomado: target = PC + offset
            nextPC_ = regFile_.getPC() + static_cast<uint32_t>(currentInstr_.immediate);
        } else {
            // Branch não tomado: next PC = PC + 4
            nextPC_ = regFile_.getPC() + 4;
        }
    } else if (isJump_) {
        // Jumps: target = PC + offset (JAL) ou rs1 + offset (JALR)
        if (currentInstr_.opcode == Opcode::OP_JALR) {
            nextPC_ = aluResult_ & 0xFFFFFFFEu;  // Clear LSB para alinhamento
        } else {
            nextPC_ = aluResult_;
        }
        
        // Salva return address em rd (PC + 4)
        // Isso será feito no writeback
    } else {
        // Sem branch/jump: next PC = PC + 4
        nextPC_ = regFile_.getPC() + 4;
    }
}

void RiscVMachine::memoryAccess() {
    uint32_t address = aluResult_;  // Endereço calculado na fase execute (rs1 + imm)
    bool success = false;
    
    if (isLoad_) {
        // Load: lê da memória
        switch (static_cast<Funct3>(currentInstr_.funct3)) {
            case Funct3::LOAD_LB: {
                // Load Byte (signed)
                uint8_t byte = readByte(address, success);
                if (success) {
                    memoryData_ = static_cast<uint32_t>(static_cast<int8_t>(byte));  // Sign-extend
                }
                break;
            }
            
            case Funct3::LOAD_LH: {
                // Load Halfword (signed)
                if ((address & 0x1) != 0) {
                    qDebug() << "Halfword load address misaligned:" << QString("0x%1").arg(address, 8, 16, QChar('0'));
                    emit trapOccurred(4, address);  // Exception code 4 = Load address misaligned
                    success = false;
                } else {
                    uint16_t halfword = readHalfword(address, success);
                    if (success) {
                        memoryData_ = static_cast<uint32_t>(static_cast<int16_t>(halfword));  // Sign-extend
                    }
                }
                break;
            }
            
            case Funct3::LOAD_LW: {
                // Load Word
                if ((address & 0x3) != 0) {
                    qDebug() << "Word load address misaligned:" << QString("0x%1").arg(address, 8, 16, QChar('0'));
                    emit trapOccurred(4, address);
                    success = false;
                } else {
                    memoryData_ = readWord(address, success);
                }
                break;
            }
            
            case Funct3::LOAD_LBU: {
                // Load Byte Unsigned
                uint8_t byte = readByte(address, success);
                if (success) {
                    memoryData_ = static_cast<uint32_t>(byte);  // Zero-extend
                }
                break;
            }
            
            case Funct3::LOAD_LHU: {
                // Load Halfword Unsigned
                if ((address & 0x1) != 0) {
                    qDebug() << "Halfword load address misaligned:" << QString("0x%1").arg(address, 8, 16, QChar('0'));
                    emit trapOccurred(4, address);
                    success = false;
                } else {
                    uint16_t halfword = readHalfword(address, success);
                    if (success) {
                        memoryData_ = static_cast<uint32_t>(halfword);  // Zero-extend
                    }
                }
                break;
            }
            
            default:
                qDebug() << "Unknown load type:" << currentInstr_.funct3;
                success = false;
                break;
        }
        
        if (success) {
            memoryAccessCount_++;
        }
        
    } else if (isStore_) {
        // Store: escreve na memória
        uint32_t value = regFile_.readRegister(currentInstr_.rs2);  // Valor vem de rs2
        
        switch (static_cast<Funct3>(currentInstr_.funct3)) {
            case Funct3::STORE_SB: {
                // Store Byte
                success = writeByte(address, static_cast<uint8_t>(value & 0xFF));
                break;
            }
            
            case Funct3::STORE_SH: {
                // Store Halfword
                if ((address & 0x1) != 0) {
                    qDebug() << "Halfword store address misaligned:" << QString("0x%1").arg(address, 8, 16, QChar('0'));
                    emit trapOccurred(6, address);  // Exception code 6 = Store address misaligned
                    success = false;
                } else {
                    success = writeHalfword(address, static_cast<uint16_t>(value & 0xFFFF));
                }
                break;
            }
            
            case Funct3::STORE_SW: {
                // Store Word
                if ((address & 0x3) != 0) {
                    qDebug() << "Word store address misaligned:" << QString("0x%1").arg(address, 8, 16, QChar('0'));
                    emit trapOccurred(6, address);
                    success = false;
                } else {
                    success = writeWord(address, value);
                }
                break;
            }
            
            default:
                qDebug() << "Unknown store type:" << currentInstr_.funct3;
                success = false;
                break;
        }
        
        if (success) {
            memoryAccessCount_++;
        }
    }
    
    if (!success) {
        emit memoryAccessFault(address, isStore_);
    }
}

void RiscVMachine::writeBack() {
    // Determina qual valor escrever em rd
    uint32_t writeValue = 0;
    bool shouldWrite = false;
    
    // Não escreve em x0 (rd == 0) ou se rd é inválido
    if (currentInstr_.rd == 0) {
        return;
    }
    
    if (isLoad_) {
        // Load: escreve dado lido da memória
        writeValue = memoryData_;
        shouldWrite = true;
    } else if (isJump_) {
        // JAL/JALR: escreve return address (PC + 4) em rd
        writeValue = regFile_.getPC() + 4;
        shouldWrite = true;
    } else if (currentInstr_.isALU || 
               currentInstr_.opcode == Opcode::OP_LUI ||
               currentInstr_.opcode == Opcode::OP_AUIPC) {
        // ALU operations: escreve resultado da ALU
        writeValue = aluResult_;
        shouldWrite = true;
    }
    
    // Escreve no register file se necessário
    if (shouldWrite) {
        regFile_.writeRegister(currentInstr_.rd, writeValue);
    }
}

void RiscVMachine::updatePC() {
    // Atualiza PC para próximo valor
    regFile_.setPC(nextPC_);
}

uint32_t RiscVMachine::readWord(uint32_t address, bool &success) {
    success = false;
    
    // Verifica bounds da memória
    if (address + 3 >= static_cast<uint32_t>(getMemorySize())) {
        return 0;
    }
    
    uint32_t word = 0;
    
    if (littleEndian_) {
        // Little-endian: byte menos significativo no menor endereço
        uint8_t b0 = static_cast<uint8_t>(getMemoryValue(address));
        uint8_t b1 = static_cast<uint8_t>(getMemoryValue(address + 1));
        uint8_t b2 = static_cast<uint8_t>(getMemoryValue(address + 2));
        uint8_t b3 = static_cast<uint8_t>(getMemoryValue(address + 3));
        
        word = static_cast<uint32_t>(b0) |
               (static_cast<uint32_t>(b1) << 8) |
               (static_cast<uint32_t>(b2) << 16) |
               (static_cast<uint32_t>(b3) << 24);
    } else {
        // Big-endian: byte mais significativo no menor endereço
        uint8_t b0 = static_cast<uint8_t>(getMemoryValue(address));
        uint8_t b1 = static_cast<uint8_t>(getMemoryValue(address + 1));
        uint8_t b2 = static_cast<uint8_t>(getMemoryValue(address + 2));
        uint8_t b3 = static_cast<uint8_t>(getMemoryValue(address + 3));
        
        word = static_cast<uint32_t>(b3) |
               (static_cast<uint32_t>(b2) << 8) |
               (static_cast<uint32_t>(b1) << 16) |
               (static_cast<uint32_t>(b0) << 24);
    }
    
    success = true;
    return word;
}

bool RiscVMachine::writeWord(uint32_t address, uint32_t value) {
    // Verifica bounds da memória
    if (address + 3 >= static_cast<uint32_t>(getMemorySize())) {
        return false;
    }
    
    if (littleEndian_) {
        setMemoryValue(address,     value & 0xFF);
        setMemoryValue(address + 1, (value >> 8) & 0xFF);
        setMemoryValue(address + 2, (value >> 16) & 0xFF);
        setMemoryValue(address + 3, (value >> 24) & 0xFF);
    } else {
        setMemoryValue(address,     (value >> 24) & 0xFF);
        setMemoryValue(address + 1, (value >> 16) & 0xFF);
        setMemoryValue(address + 2, (value >> 8) & 0xFF);
        setMemoryValue(address + 3, value & 0xFF);
    }
    
    return true;
}

uint8_t RiscVMachine::readByte(uint32_t address, bool &success) {
    success = false;
    
    if (address >= static_cast<uint32_t>(getMemorySize())) {
        return 0;
    }
    
    success = true;
    return static_cast<uint8_t>(getMemoryValue(address));
}

bool RiscVMachine::writeByte(uint32_t address, uint8_t value) {
    if (address >= static_cast<uint32_t>(getMemorySize())) {
        return false;
    }
    
    setMemoryValue(address, value);
    return true;
}

uint16_t RiscVMachine::readHalfword(uint32_t address, bool &success) {
    success = false;
    
    if (address + 1 >= static_cast<uint32_t>(getMemorySize())) {
        return 0;
    }
    
    uint16_t halfword = 0;
    
    if (littleEndian_) {
        uint8_t b0 = static_cast<uint8_t>(getMemoryValue(address));
        uint8_t b1 = static_cast<uint8_t>(getMemoryValue(address + 1));
        
        halfword = static_cast<uint16_t>(b0) | (static_cast<uint16_t>(b1) << 8);
    } else {
        uint8_t b0 = static_cast<uint8_t>(getMemoryValue(address));
        uint8_t b1 = static_cast<uint8_t>(getMemoryValue(address + 1));
        
        halfword = static_cast<uint16_t>(b1) | (static_cast<uint16_t>(b0) << 8);
    }
    
    success = true;
    return halfword;
}

bool RiscVMachine::writeHalfword(uint32_t address, uint16_t value) {
    if (address + 1 >= static_cast<uint32_t>(getMemorySize())) {
        return false;
    }
    
    if (littleEndian_) {
        setMemoryValue(address,     value & 0xFF);
        setMemoryValue(address + 1, (value >> 8) & 0xFF);
    } else {
        setMemoryValue(address,     (value >> 8) & 0xFF);
        setMemoryValue(address + 1, value & 0xFF);
    }
    
    return true;
}

void RiscVMachine::assemble(QString sourceCode) {
    // Stub para implementação futura do assembler RISC-V
    // Por enquanto, delega para a classe base (que não entende RISC-V)
    
    qWarning() << "RiscVMachine::assemble() - Assembler RISC-V ainda não implementado.";
    qWarning() << "Use importMemory() ou setMemoryValue() para carregar programas.";
    
    // Chama implementação da classe base para não quebrar interface
    Machine::assemble(sourceCode);
}

} // namespace riscv
