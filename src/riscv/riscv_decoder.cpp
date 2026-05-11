#include "riscv_decoder.h"
#include <QDebug>

namespace riscv {

RiscVDecoder::RiscVDecoder(QObject *parent)
    : QObject(parent)
{
}

DecodedInstruction RiscVDecoder::decode(uint32_t instruction) {
    DecodedInstruction decoded;
    decoded.raw = instruction;
    
    // Extrai campos básicos comuns a todos os formatos
    decoded.opcode  = static_cast<Opcode>(extractOpcode(instruction));
    decoded.funct3  = extractFunct3(instruction);
    decoded.funct7  = extractFunct7(instruction);
    decoded.rd      = extractRd(instruction);
    decoded.rs1     = extractRs1(instruction);
    decoded.rs2     = extractRs2(instruction);
    
    // Verifica alinhamento (instruções RV32I devem ter bits [1:0] = 00)
    if ((instruction & ALIGNMENT_MASK) != 0) {
        emit alignmentError(0);  // PC seria passado em uso real
        decoded.type = InstrType::NONE;
        return decoded;
    }
    
    // Dispatch baseado no opcode
    switch (decoded.opcode) {
        case Opcode::OP_OP:
            decodeRType(instruction, decoded);
            break;
            
        case Opcode::OP_OPIMM:
            decodeIType(instruction, decoded);
            break;
            
        case Opcode::OP_LOAD:
            decodeIType(instruction, decoded);  // Loads usam formato I
            decoded.isLoad = true;
            break;
            
        case Opcode::OP_STORE:
            decodeSType(instruction, decoded);
            decoded.isStore = true;
            break;
            
        case Opcode::OP_BRANCH:
            decodeBType(instruction, decoded);
            decoded.isBranch = true;
            break;
            
        case Opcode::OP_LUI:
        case Opcode::OP_AUIPC:
            decodeUType(instruction, decoded);
            break;
            
        case Opcode::OP_JAL:
            decodeJType(instruction, decoded);
            decoded.isJump = true;
            break;
            
        case Opcode::OP_JALR:
            decodeIType(instruction, decoded);  // JALR usa formato I
            decoded.isJump = true;
            break;
            
        case Opcode::OP_SYSTEM:
            // Instruções de sistema (ECALL, EBREAK, etc.)
            // Formato I-like, mas tratado separadamente
            decodeIType(instruction, decoded);
            break;
            
        default:
            // Opcode não reconhecido para RV32I base
            decoded.type = InstrType::NONE;
            emit invalidInstruction(0, instruction);
            break;
    }
    
    return decoded;
}

bool RiscVDecoder::isValidInstruction(uint32_t instruction) {
    // Verifica alinhamento básico
    if ((instruction & ALIGNMENT_MASK) != 0) {
        return false;
    }
    
    uint8_t opcode = extractOpcode(instruction);
    
    // Verifica se opcode é válido para RV32I
    switch (static_cast<Opcode>(opcode)) {
        case Opcode::OP_LUI:
        case Opcode::OP_AUIPC:
        case Opcode::OP_JAL:
        case Opcode::OP_JALR:
        case Opcode::OP_BRANCH:
        case Opcode::OP_LOAD:
        case Opcode::OP_STORE:
        case Opcode::OP_OPIMM:
        case Opcode::OP_OP:
        case Opcode::OP_SYSTEM:
            return true;
            
        default:
            return false;
    }
}

QString RiscVDecoder::getMnemonic(const DecodedInstruction &instr) {
    switch (instr.opcode) {
        case Opcode::OP_LUI:      return "lui";
        case Opcode::OP_AUIPC:    return "auipc";
        case Opcode::OP_JAL:      return "jal";
        case Opcode::OP_JALR:     return "jalr";
        
        case Opcode::OP_BRANCH:
            switch (static_cast<Funct3>(instr.funct3)) {
                case Funct3::BRANCH_BEQ:  return "beq";
                case Funct3::BRANCH_BNE:  return "bne";
                case Funct3::BRANCH_BLT:  return "blt";
                case Funct3::BRANCH_BGE:  return "bge";
                case Funct3::BRANCH_BLTU: return "bltu";
                case Funct3::BRANCH_BGEU: return "bgeu";
                default: return "unknown";
            }
            
        case Opcode::OP_LOAD:
            switch (static_cast<Funct3>(instr.funct3)) {
                case Funct3::LOAD_LB:  return "lb";
                case Funct3::LOAD_LH:  return "lh";
                case Funct3::LOAD_LW:  return "lw";
                case Funct3::LOAD_LBU: return "lbu";
                case Funct3::LOAD_LHU: return "lhu";
                default: return "unknown";
            }
            
        case Opcode::OP_STORE:
            switch (static_cast<Funct3>(instr.funct3)) {
                case Funct3::STORE_SB: return "sb";
                case Funct3::STORE_SH: return "sh";
                case Funct3::STORE_SW: return "sw";
                default: return "unknown";
            }
            
        case Opcode::OP_OPIMM:
            switch (static_cast<Funct3>(instr.funct3)) {
                case Funct3::ALU_ADD_SUB: return "addi";
                case Funct3::ALU_SLL:     return "slli";
                case Funct3::ALU_SLT:     return "slti";
                case Funct3::ALU_SLTU:    return "sltiu";
                case Funct3::ALU_XOR:     return "xori";
                case Funct3::ALU_SR:
                    return (instr.funct7 == 0x40) ? "srai" : "srli";
                case Funct3::ALU_OR:      return "ori";
                case Funct3::ALU_AND:     return "andi";
                default: return "unknown";
            }
            
        case Opcode::OP_OP:
            switch (static_cast<Funct3>(instr.funct3)) {
                case Funct3::ALU_ADD_SUB:
                    return (instr.funct7 == 0x20) ? "sub" : "add";
                case Funct3::ALU_SLL:     return "sll";
                case Funct3::ALU_SLT:     return "slt";
                case Funct3::ALU_SLTU:    return "sltu";
                case Funct3::ALU_XOR:     return "xor";
                case Funct3::ALU_SR:
                    return (instr.funct7 == 0x20) ? "sra" : "srl";
                case Funct3::ALU_OR:      return "or";
                case Funct3::ALU_AND:     return "and";
                default: return "unknown";
            }
            
        case Opcode::OP_SYSTEM:
            if (instr.funct3 == 0x0) {
                if (instr.immediate == 0x000) return "ecall";
                if (instr.immediate == 0x001) return "ebreak";
                return "sys";
            }
            if (instr.funct3 == 0x1) return "csrrw";
            if (instr.funct3 == 0x2) return "csrrs";
            if (instr.funct3 == 0x3) return "csrrc";
            return "sys";
            
        default:
            return "unknown";
    }
}

QString RiscVDecoder::formatAssembly(const DecodedInstruction &instr) {
    QString mnemonic = getMnemonic(instr);
    QStringList args;
    
    // Adiciona rd se presente e não for zero (ou se for instrução que usa rd)
    if (instr.rd != 0 || instr.isALU || instr.isLoad || 
        instr.opcode == Opcode::OP_LUI || instr.opcode == Opcode::OP_AUIPC ||
        instr.opcode == Opcode::OP_JAL || instr.opcode == Opcode::OP_JALR) {
        args << RiscVRegisterFile::getRegName(instr.rd);
    }
    
    // Adiciona rs1 se presente
    if (instr.rs1 != 0 || instr.isLoad || instr.isStore || 
        instr.isBranch || instr.opcode == Opcode::OP_JALR) {
        if (!args.isEmpty()) args << ",";
        args << RiscVRegisterFile::getRegName(instr.rs1);
    }
    
    // Adiciona rs2 ou imediato dependendo do tipo
    switch (instr.type) {
        case InstrType::R_TYPE:
            if (instr.rs2 != 0 || instr.isALU) {
                if (!args.isEmpty()) args << ",";
                args << RiscVRegisterFile::getRegName(instr.rs2);
            }
            break;
            
        case InstrType::I_TYPE:
        case InstrType::S_TYPE:
        case InstrType::B_TYPE:
        case InstrType::U_TYPE:
        case InstrType::J_TYPE:
            if (!args.isEmpty()) args << ",";
            if (instr.type == InstrType::U_TYPE) {
                // Imediato de 20 bits shiftado
                args << QString("0x%1").arg(instr.immediate & 0xFFFFF000u, 8, 16, QChar('0'));
            } else if (instr.type == InstrType::J_TYPE && instr.opcode == Opcode::OP_JAL) {
                // JAL mostra offset relativo
                args << QString("%1").arg(instr.immediate);
            } else if (instr.isBranch) {
                // Branch mostra offset relativo
                args << QString("%1").arg(instr.immediate);
            } else {
                // Imediato normal
                args << QString("%1").arg(instr.immediate);
            }
            break;
            
        default:
            break;
    }
    
    return QString("%1 %2").arg(mnemonic, args.join(" "));
}

void RiscVDecoder::decodeRType(uint32_t instr, DecodedInstruction &decoded) {
    decoded.type = InstrType::R_TYPE;
    decoded.isALU = true;
    decoded.immBitWidth = 0;  // Sem imediato
    decoded.immediate = 0;
    
    // Determina operação ALU baseada em funct3 e funct7
    Funct3 funct3 = static_cast<Funct3>(decoded.funct3);
    Funct7 funct7 = static_cast<Funct7>(decoded.funct7);
    
    switch (funct3) {
        case Funct3::ALU_ADD_SUB:
            decoded.aluOp = (funct7 == Funct7::ALU_SUB) ? ALUOp::SUB : ALUOp::ADD;
            break;
        case Funct3::ALU_SLL:
            decoded.aluOp = ALUOp::SLL;
            break;
        case Funct3::ALU_SLT:
            decoded.aluOp = ALUOp::SLT;
            break;
        case Funct3::ALU_SLTU:
            decoded.aluOp = ALUOp::SLTU;
            break;
        case Funct3::ALU_XOR:
            decoded.aluOp = ALUOp::XOR;
            break;
        case Funct3::ALU_SR:
            decoded.aluOp = (funct7 == Funct7::ALU_SUB) ? ALUOp::SRA : ALUOp::SRL;
            break;
        case Funct3::ALU_OR:
            decoded.aluOp = ALUOp::OR;
            break;
        case Funct3::ALU_AND:
            decoded.aluOp = ALUOp::AND;
            break;
        default:
            decoded.aluOp = ALUOp::NONE;
            break;
    }
}

void RiscVDecoder::decodeIType(uint32_t instr, DecodedInstruction &decoded) {
    decoded.type = InstrType::I_TYPE;
    
    // Extrai imediato de 12 bits (bits [31:20])
    int32_t imm_raw = static_cast<int32_t>(instr >> 20);
    decoded.immediate = signExtend(imm_raw, 12);
    decoded.immBitWidth = 12;
    
    // Determina operação baseada no opcode e funct3
    if (decoded.opcode == Opcode::OP_LOAD) {
        decoded.isLoad = true;
        decoded.aluOp = ALUOp::ADD;  // Address calculation: rs1 + imm
    } else if (decoded.opcode == Opcode::OP_JALR) {
        decoded.isJump = true;
        decoded.aluOp = ALUOp::ADD;  // Target address: rs1 + imm
    } else if (decoded.opcode == Opcode::OP_OPIMM) {
        decoded.isALU = true;
        
        Funct3 funct3 = static_cast<Funct3>(decoded.funct3);
        Funct7 funct7 = static_cast<Funct7>(decoded.funct7);
        
        switch (funct3) {
            case Funct3::ALU_ADD_SUB:
                decoded.aluOp = ALUOp::ADD;
                break;
            case Funct3::ALU_SLL:
                decoded.aluOp = ALUOp::SLL;
                break;
            case Funct3::ALU_SLT:
                decoded.aluOp = ALUOp::SLT;
                break;
            case Funct3::ALU_SLTU:
                decoded.aluOp = ALUOp::SLTU;
                break;
            case Funct3::ALU_XOR:
                decoded.aluOp = ALUOp::XOR;
                break;
            case Funct3::ALU_SR:
                decoded.aluOp = (funct7 == Funct7::ALU_SUB) ? ALUOp::SRA : ALUOp::SRL;
                break;
            case Funct3::ALU_OR:
                decoded.aluOp = ALUOp::OR;
                break;
            case Funct3::ALU_AND:
                decoded.aluOp = ALUOp::AND;
                break;
            default:
                decoded.aluOp = ALUOp::NONE;
                break;
        }
    } else if (decoded.opcode == Opcode::OP_SYSTEM) {
        // Instruções de sistema - imediato tem significado especial
        decoded.aluOp = ALUOp::NONE;
    }
}

void RiscVDecoder::decodeSType(uint32_t instr, DecodedInstruction &decoded) {
    decoded.type = InstrType::S_TYPE;
    decoded.isStore = true;
    decoded.aluOp = ALUOp::ADD;  // Address calculation: rs1 + imm
    
    // Imediato de 12 bits split: [31:25] são bits [11:5], [11:7] são bits [4:0]
    uint32_t imm_11_5 = (instr >> 25) & 0x7F;
    uint32_t imm_4_0  = (instr >> 7) & 0x1F;
    uint32_t imm_raw  = (imm_11_5 << 5) | imm_4_0;
    
    decoded.immediate = signExtend(imm_raw, 12);
    decoded.immBitWidth = 12;
}

void RiscVDecoder::decodeBType(uint32_t instr, DecodedInstruction &decoded) {
    decoded.type = InstrType::B_TYPE;
    decoded.isBranch = true;
    decoded.aluOp = ALUOp::NONE;  // Comparação feita na unidade de branch
    
    // Imediato de 13 bits split:
    // bit 12: bit 31
    // bit 11: bit 7
    // bits 10-5: bits 30-25
    // bit 4: bit 8
    // bits 3-1: 0 (sempre alinhado a 2 bytes)
    // bit 0: 0 (sempre alinhado a 2 bytes)
    uint32_t imm_12   = (instr >> 31) & 0x01;
    uint32_t imm_11   = (instr >> 7) & 0x01;
    uint32_t imm_10_5 = (instr >> 25) & 0x3F;
    uint32_t imm_4    = (instr >> 8) & 0x01;
    
    uint32_t imm_raw = (imm_12 << 12) | (imm_11 << 11) | (imm_10_5 << 5) | (imm_4 << 4);
    decoded.immediate = signExtend(imm_raw, 13);
    decoded.immBitWidth = 13;
}

void RiscVDecoder::decodeUType(uint32_t instr, DecodedInstruction &decoded) {
    decoded.type = InstrType::U_TYPE;
    
    // Imediato de 20 bits (bits [31:12]), shiftado para esquerda por 12
    int32_t imm_raw = static_cast<int32_t>(instr & 0xFFFFF000u);
    decoded.immediate = imm_raw;  // Já está na posição correta
    decoded.immBitWidth = 20;
    
    if (decoded.opcode == Opcode::OP_LUI) {
        decoded.aluOp = ALUOp::PASS_B;  // rd = imm
    } else if (decoded.opcode == Opcode::OP_AUIPC) {
        decoded.aluOp = ALUOp::ADD;  // rd = PC + imm
    }
}

void RiscVDecoder::decodeJType(uint32_t instr, DecodedInstruction &decoded) {
    decoded.type = InstrType::J_TYPE;
    decoded.isJump = true;
    
    // Imediato de 21 bits split para JAL:
    // bit 20: bit 31
    // bits 19-12: bits 19-12
    // bit 11: bit 20
    // bits 10-1: bits 30-21
    // bit 0: 0 (sempre alinhado a 2 bytes)
    uint32_t imm_20   = (instr >> 31) & 0x01;
    uint32_t imm_19_12 = (instr >> 12) & 0xFF;
    uint32_t imm_11   = (instr >> 20) & 0x01;
    uint32_t imm_10_1 = (instr >> 21) & 0x3FF;
    
    uint32_t imm_raw = (imm_20 << 20) | (imm_19_12 << 12) | (imm_11 << 11) | (imm_10_1 << 1);
    decoded.immediate = signExtend(imm_raw, 21);
    decoded.immBitWidth = 21;
    
    decoded.aluOp = ALUOp::ADD;  // Target address: PC + imm
}

int32_t RiscVDecoder::signExtend(uint32_t value, int bitWidth) {
    // Máscara para isolar os bits relevantes
    uint32_t mask = (1u << bitWidth) - 1;
    uint32_t masked = value & mask;
    
    // Verifica se o bit de sinal está setado
    uint32_t signBit = 1u << (bitWidth - 1);
    
    if (masked & signBit) {
        // Bit de sinal = 1, estende com 1s
        return static_cast<int32_t>(masked | (~mask));
    } else {
        // Bit de sinal = 0, estende com 0s
        return static_cast<int32_t>(masked);
    }
}

} // namespace riscv
