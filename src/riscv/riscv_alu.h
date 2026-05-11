#ifndef RISCV_ALU_H
#define RISCV_ALU_H

#include <QObject>
#include <cstdint>
#include "riscv_decoder.h"

/**
 * @brief Arithmetic Logic Unit para RISC-V RV32I
 * 
 * Responsabilidades:
 * - Executar operações aritméticas (ADD, SUB)
 * - Executar operações lógicas (AND, OR, XOR)
 * - Executar shifts (SLL, SRL, SRA)
 * - Executar comparações (SLT, SLTU)
 * - Detectar overflow para operações com sinal
 * - Operar em dados de 32 bits
 */

namespace riscv {

/**
 * @brief Resultado de uma operação da ALU
 */
struct ALUResult {
    uint32_t result;        // Resultado da operação (32 bits)
    bool overflow;          // true se houve overflow arithmetic (apenas para ADD/SUB signed)
    bool zero;              // true se resultado é zero
    bool negative;          // true se resultado é negativo (bit 31 = 1)
    
    ALUResult() 
        : result(0), overflow(false), zero(false), negative(false) {}
};

/**
 * @brief Classe que implementa a ALU RISC-V
 */
class RiscVALU : public QObject {
    Q_OBJECT

public:
    explicit RiscVALU(QObject *parent = nullptr);
    ~RiscVALU() = default;

    //////////////////////////////////////////////////
    // Execução de Operações
    //////////////////////////////////////////////////
    
    /**
     * @brief Executa uma operação ALU baseada na instrução decodificada
     * @param op Operação a executar
     * @param operandA Primeiro operando (geralmente rs1 ou PC)
     * @param operandB Segundo operando (geralmente rs2 ou immediate)
     * @return ALUResult com resultado e flags
     * 
     * Nota: Para operações de shift, operandB especifica o amount (bits [4:0])
     */
    ALUResult execute(ALUOp op, uint32_t operandA, uint32_t operandB);
    
    /**
     * @brief Executa operação ADD
     * @param a Operando A
     * @param b Operando B
     * @return a + b (com wraparound de 32 bits)
     */
    static uint32_t add(uint32_t a, uint32_t b);
    
    /**
     * @brief Executa operação SUB
     * @param a Operando A
     * @param b Operando B
     * @return a - b (com wraparound de 32 bits)
     */
    static uint32_t sub(uint32_t a, uint32_t b);
    
    /**
     * @brief Executa operação AND bit-a-bit
     */
    static uint32_t bitwiseAnd(uint32_t a, uint32_t b);
    
    /**
     * @brief Executa operação OR bit-a-bit
     */
    static uint32_t bitwiseOr(uint32_t a, uint32_t b);
    
    /**
     * @brief Executa operação XOR bit-a-bit
     */
    static uint32_t bitwiseXor(uint32_t a, uint32_t b);
    
    /**
     * @brief Executa shift left logical
     * @param value Valor a shiftar
     * @param amount Quantidade de bits (apenas bits [4:0] são usados)
     * @return value << amount
     */
    static uint32_t shiftLeftLogical(uint32_t value, uint32_t amount);
    
    /**
     * @brief Executa shift right logical
     * @param value Valor a shiftar
     * @param amount Quantidade de bits (apenas bits [4:0] são usados)
     * @return value >> amount (preenchendo com zeros)
     */
    static uint32_t shiftRightLogical(uint32_t value, uint32_t amount);
    
    /**
     * @brief Executa shift right arithmetic
     * @param value Valor a shiftar
     * @param amount Quantidade de bits (apenas bits [4:0] são usados)
     * @return value >>> amount (preservando bit de sinal)
     */
    static uint32_t shiftRightArithmetic(uint32_t value, uint32_t amount);
    
    /**
     * @brief Compara dois valores com sinal
     * @param a Operando A
     * @param b Operando B
     * @return 1 se a < b (signed), 0 caso contrário
     */
    static uint32_t compareLessThanSigned(int32_t a, int32_t b);
    
    /**
     * @brief Compara dois valores sem sinal
     * @param a Operando A
     * @param b Operando B
     * @return 1 se a < b (unsigned), 0 caso contrário
     */
    static uint32_t compareLessThanUnsigned(uint32_t a, uint32_t b);

    //////////////////////////////////////////////////
    // Detecção de Overflow
    //////////////////////////////////////////////////
    
    /**
     * @brief Verifica se houve overflow em ADD signed
     * @param a Operando A (interpretado como signed)
     * @param b Operando B (interpretado como signed)
     * @param result Resultado da soma
     * @return true se houve overflow
     */
    static bool checkOverflowAdd(int32_t a, int32_t b, int32_t result);
    
    /**
     * @brief Verifica se houve overflow em SUB signed
     * @param a Operando A (interpretado como signed)
     * @param b Operando B (interpretado como signed)
     * @param result Resultado da subtração
     * @return true se houve overflow
     */
    static bool checkOverflowSub(int32_t a, int32_t b, int32_t result);

signals:
    /**
     * @brief Signal emitido quando uma operação é executada
     * @param op Operação executada
     * @param operandA Operando A
     * @param operandB Operando B
     * @param result Resultado
     */
    void operationExecuted(ALUOp op, uint32_t operandA, uint32_t operandB, uint32_t result);
    
    /**
     * @brief Signal emitido quando overflow é detectado
     * @param op Operação que causou overflow
     * @param operandA Operando A
     * @param operandB Operando B
     */
    void overflowDetected(ALUOp op, uint32_t operandA, uint32_t operandB);

private:
    /**
     * @brief Aplica máscara de 32 bits
     */
    static uint32_t maskTo32Bits(uint64_t value) {
        return static_cast<uint32_t>(value & 0xFFFFFFFFu);
    }
};

} // namespace riscv

#endif // RISCV_ALU_H
