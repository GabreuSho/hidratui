#include "riscv_alu.h"

namespace riscv {

RiscVALU::RiscVALU(QObject *parent)
    : QObject(parent)
{
}

ALUResult RiscVALU::execute(ALUOp op, uint32_t operandA, uint32_t operandB) {
    ALUResult result;
    
    switch (op) {
        case ALUOp::ADD: {
            int32_t a = static_cast<int32_t>(operandA);
            int32_t b = static_cast<int32_t>(operandB);
            int32_t res = static_cast<int32_t>(add(operandA, operandB));
            
            result.result = static_cast<uint32_t>(res);
            result.overflow = checkOverflowAdd(a, b, res);
            
            if (result.overflow) {
                emit overflowDetected(op, operandA, operandB);
            }
            break;
        }
        
        case ALUOp::SUB: {
            int32_t a = static_cast<int32_t>(operandA);
            int32_t b = static_cast<int32_t>(operandB);
            int32_t res = static_cast<int32_t>(sub(operandA, operandB));
            
            result.result = static_cast<uint32_t>(res);
            result.overflow = checkOverflowSub(a, b, res);
            
            if (result.overflow) {
                emit overflowDetected(op, operandA, operandB);
            }
            break;
        }
        
        case ALUOp::AND:
            result.result = bitwiseAnd(operandA, operandB);
            result.overflow = false;  // Operações lógicas não causam overflow
            break;
            
        case ALUOp::OR:
            result.result = bitwiseOr(operandA, operandB);
            result.overflow = false;
            break;
            
        case ALUOp::XOR:
            result.result = bitwiseXor(operandA, operandB);
            result.overflow = false;
            break;
            
        case ALUOp::SLL:
            // Apenas bits [4:0] de operandB são usados para shift amount
            result.result = shiftLeftLogical(operandA, operandB & 0x1F);
            result.overflow = false;
            break;
            
        case ALUOp::SRL:
            result.result = shiftRightLogical(operandA, operandB & 0x1F);
            result.overflow = false;
            break;
            
        case ALUOp::SRA:
            result.result = shiftRightArithmetic(operandA, operandB & 0x1F);
            result.overflow = false;
            break;
            
        case ALUOp::SLT: {
            // Set Less Than (signed comparison)
            int32_t a = static_cast<int32_t>(operandA);
            int32_t b = static_cast<int32_t>(operandB);
            result.result = compareLessThanSigned(a, b);
            result.overflow = false;
            break;
        }
        
        case ALUOp::SLTU: {
            // Set Less Than Unsigned
            result.result = compareLessThanUnsigned(operandA, operandB);
            result.overflow = false;
            break;
        }
        
        case ALUOp::COPY_A:
            // Passa operando A direto (usado em alguns casos especiais)
            result.result = operandA;
            result.overflow = false;
            break;
            
        case ALUOp::PASS_B:
            // Passa operando B direto (usado para LUI, por exemplo)
            result.result = operandB;
            result.overflow = false;
            break;
            
        case ALUOp::NONE:
        default:
            result.result = 0;
            result.overflow = false;
            break;
    }
    
    // Atualiza flags de status
    result.zero = (result.result == 0);
    result.negative = ((result.result & 0x80000000u) != 0);
    
    // Emite signal para debugging/monitoramento
    emit operationExecuted(op, operandA, operandB, result.result);
    
    return result;
}

uint32_t RiscVALU::add(uint32_t a, uint32_t b) {
    return maskTo32Bits(static_cast<uint64_t>(a) + static_cast<uint64_t>(b));
}

uint32_t RiscVALU::sub(uint32_t a, uint32_t b) {
    return maskTo32Bits(static_cast<uint64_t>(a) - static_cast<uint64_t>(b));
}

uint32_t RiscVALU::bitwiseAnd(uint32_t a, uint32_t b) {
    return a & b;
}

uint32_t RiscVALU::bitwiseOr(uint32_t a, uint32_t b) {
    return a | b;
}

uint32_t RiscVALU::bitwiseXor(uint32_t a, uint32_t b) {
    return a ^ b;
}

uint32_t RiscVALU::shiftLeftLogical(uint32_t value, uint32_t amount) {
    // Em RV32I, apenas os 5 bits menos significativos de amount são usados
    uint32_t shiftAmount = amount & 0x1F;
    return value << shiftAmount;
}

uint32_t RiscVALU::shiftRightLogical(uint32_t value, uint32_t amount) {
    uint32_t shiftAmount = amount & 0x1F;
    return value >> shiftAmount;
}

uint32_t RiscVALU::shiftRightArithmetic(uint32_t value, uint32_t amount) {
    uint32_t shiftAmount = amount & 0x1F;
    
    // Para shift arithmetic, preservamos o bit de sinal
    // Em C++, right shift de signed integers é implementation-defined,
    // mas na prática a maioria dos compiladores faz arithmetic shift
    int32_t signedValue = static_cast<int32_t>(value);
    return static_cast<uint32_t>(signedValue >> shiftAmount);
}

uint32_t RiscVALU::compareLessThanSigned(int32_t a, int32_t b) {
    return (a < b) ? 1 : 0;
}

uint32_t RiscVALU::compareLessThanUnsigned(uint32_t a, uint32_t b) {
    return (a < b) ? 1 : 0;
}

bool RiscVALU::checkOverflowAdd(int32_t a, int32_t b, int32_t result) {
    // Overflow ocorre em ADD signed quando:
    // - Dois positivos somam e resulta em negativo
    // - Dois negativos somam e resulta em positivo
    
    bool aPositive = (a >= 0);
    bool bPositive = (b >= 0);
    bool resultPositive = (result >= 0);
    
    if (aPositive && bPositive && !resultPositive) {
        return true;  // Positive + Positive = Negative (overflow)
    }
    
    if (!aPositive && !bPositive && resultPositive) {
        return true;  // Negative + Negative = Positive (overflow)
    }
    
    return false;
}

bool RiscVALU::checkOverflowSub(int32_t a, int32_t b, int32_t result) {
    // Overflow ocorre em SUB signed quando:
    // - Positivo - Negativo = Negativo
    // - Negativo - Positivo = Positivo
    
    bool aPositive = (a >= 0);
    bool bPositive = (b >= 0);
    bool resultPositive = (result >= 0);
    
    if (aPositive && !bPositive && !resultPositive) {
        return true;  // Positive - Negative = Negative (overflow)
    }
    
    if (!aPositive && bPositive && resultPositive) {
        return true;  // Negative - Positive = Positive (overflow)
    }
    
    return false;
}

} // namespace riscv
