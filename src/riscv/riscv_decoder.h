#ifndef RISCV_DECODER_H
#define RISCV_DECODER_H

#include <QObject>
#include <QString>
#include <cstdint>
#include "riscv_register_file.h"

/**
 * @brief Instruction Decoder para RISC-V RV32I
 * 
 * Responsabilidades:
 * - Parsear instruções de 32 bits nos 6 formatos (R, I, S, B, U, J)
 * - Extrair opcode, funct3, funct7, registradores e imediatos
 * - Validar alinhamento de 4 bytes
 * - Mapear para operações internas da ALU
 */

namespace riscv {

/**
 * @brief Tipos de instrução RISC-V
 */
enum class InstrType : uint8_t {
    NONE = 0,   // Inválida/não decodificada
    R_TYPE,     // Register-Register (ADD, SUB, AND, OR, XOR, SLT, etc.)
    I_TYPE,     // Immediate (ADDI, ANDI, ORI, XORI, SLTI, etc.) + LOADs
    S_TYPE,     // Store (SB, SH, SW)
    B_TYPE,     // Branch (BEQ, BNE, BLT, BGE, etc.)
    U_TYPE,     // Upper Immediate (LUI, AUIPC)
    J_TYPE      // Jump (JAL, JALR)
};

/**
 * @brief Opcodes principais do RISC-V RV32I
 * 
 * Nota: Os valores são os 7 bits menos significativos do opcode
 */
enum class Opcode : uint8_t {
    OP_LUI      = 0x37,   // 0110111
    OP_AUIPC    = 0x17,   // 0010111
    OP_JAL      = 0x6F,   // 1101111
    OP_JALR     = 0x67,   // 1100111
    OP_BRANCH   = 0x63,   // 1100011
    OP_LOAD     = 0x03,   // 0000011
    OP_STORE    = 0x23,   // 0100011
    OP_OPIMM    = 0x13,   // 0010011
    OP_OP       = 0x33,   // 0110011
    OP_SYSTEM   = 0x73,   // 1110011
    
    // Não são parte do RV32I base, mas úteis para extensões
    OP_MISC_MEM = 0x0F,   // 0001111 (fence)
    OP_AMO      = 0x2F    // 0101111 (atomic operations)
};

/**
 * @brief Funct3 para diferentes tipos de instrução
 */
enum class Funct3 : uint8_t {
    // Para R-type e I-type (ALU operations)
    ALU_ADD_SUB = 0x0,  // ADD/SUB
    ALU_SLL     = 0x1,  // Shift Left Logical
    ALU_SLT     = 0x2,  // Set Less Than (signed)
    ALU_SLTU    = 0x3,  // Set Less Than Unsigned
    ALU_XOR     = 0x4,  // XOR
    ALU_SR      = 0x5,  // Shift Right (logical/arithmetic depende de funct7)
    ALU_OR      = 0x6,  // OR
    ALU_AND     = 0x7,  // AND
    
    // Para LOAD instructions
    LOAD_LB  = 0x0,  // Load Byte (signed)
    LOAD_LH  = 0x1,  // Load Halfword (signed)
    LOAD_LW  = 0x2,  // Load Word
    LOAD_LBU = 0x4,  // Load Byte Unsigned
    LOAD_LHU = 0x5,  // Load Halfword Unsigned
    
    // Para STORE instructions
    STORE_SB = 0x0,  // Store Byte
    STORE_SH = 0x1,  // Store Halfword
    STORE_SW = 0x2,  // Store Word
    
    // Para BRANCH instructions
    BRANCH_BEQ  = 0x0,  // Branch if Equal
    BRANCH_BNE  = 0x1,  // Branch if Not Equal
    BRANCH_BLT  = 0x4,  // Branch if Less Than (signed)
    BRANCH_BGE  = 0x5,  // Branch if Greater or Equal (signed)
    BRANCH_BLTU = 0x6,  // Branch if Less Than Unsigned
    BRANCH_BGEU = 0x7   // Branch if Greater or Equal Unsigned
};

/**
 * @brief Funct7 para instruções R-type
 */
enum class Funct7 : uint8_t {
    ALU_ADD  = 0x00,  // ADD, SLL, SLT, SLTU, XOR, SRL, OR, AND
    ALU_SUB  = 0x20,  // SUB, SRA (0100000)
    
    // Para extensões M (multiplicação)
    MUL_DIV = 0x01    // MUL, MULH, MULHSU, MULHU, DIV, DIVU, REM, REMU
};

/**
 * @brief Códigos de operação interna para a ALU
 */
enum class ALUOp : uint8_t {
    NONE,
    ADD, SUB,
    AND, OR, XOR,
    SLL, SRL, SRA,
    SLT, SLTU,
    COPY_A,         // Passa operando A direto (para loads/stores)
    PASS_B          // Passa operando B direto
};

/**
 * @brief Estrutura que contém uma instrução decodificada
 */
struct DecodedInstruction {
    // Campos brutos da instrução
    uint32_t raw;           // Instrução completa (32 bits)
    
    // Campos decodificados comuns
    Opcode opcode;          // Opcode principal (7 bits)
    uint8_t funct3;         // Funct3 (3 bits)
    uint8_t funct7;         // Funct7 (7 bits, apenas para R-type)
    
    // Registradores
    int rd;                 // Destination register (5 bits)
    int rs1;                // Source register 1 (5 bits)
    int rs2;                // Source register 2 (5 bits)
    
    // Imediato (valor já estendido para 32 bits com sinal quando aplicável)
    int32_t immediate;
    
    // Tipo de instrução
    InstrType type;
    
    // Operação ALU resultante
    ALUOp aluOp;
    
    // Informações adicionais
    bool isBranch;          // true se é instrução de branch
    bool isJump;            // true se é instrução de jump
    bool isLoad;            // true se é load
    bool isStore;           // true se é store
    bool isALU;             // true se é operação ALU
    
    // Tamanho do imediato em bits (antes da extensão de sinal)
    int immBitWidth;
    
    DecodedInstruction() 
        : raw(0), opcode(Opcode::OP_OPIMM), funct3(0), funct7(0)
        , rd(0), rs1(0), rs2(0), immediate(0)
        , type(InstrType::NONE), aluOp(ALUOp::NONE)
        , isBranch(false), isJump(false), isLoad(false), isStore(false), isALU(false)
        , immBitWidth(0) {}
};

/**
 * @brief Classe responsável pelo decode de instruções RISC-V
 */
class RiscVDecoder : public QObject {
    Q_OBJECT

public:
    explicit RiscVDecoder(QObject *parent = nullptr);
    ~RiscVDecoder() = default;

    //////////////////////////////////////////////////
    // Decode de Instruções
    //////////////////////////////////////////////////
    
    /**
     * @brief Decodifica uma instrução de 32 bits
     * @param instruction Palavra de 32 bits contendo a instrução
     * @return DecodedInstruction com todos os campos preenchidos
     * 
     * Valida:
     * - Alinhamento (bits [1:0] devem ser 0 para instruções de 32 bits)
     * - Opcode válido
     * - Campos funct3/funct7 consistentes
     */
    DecodedInstruction decode(uint32_t instruction);
    
    /**
     * @brief Verifica se uma instrução é válida
     * @param instruction Palavra de 32 bits
     * @return true se a instrução parece válida
     */
    static bool isValidInstruction(uint32_t instruction);
    
    /**
     * @brief Obtém o mnemônico de uma instrução decodificada
     */
    static QString getMnemonic(const DecodedInstruction &instr);
    
    /**
     * @brief Formata uma instrução como string assembly (para debug)
     * @param instr Instrução decodificada
     * @return String no formato "mnemonic arg1, arg2, arg3"
     */
    static QString formatAssembly(const DecodedInstruction &instr);

    //////////////////////////////////////////////////
    // Constantes
    //////////////////////////////////////////////////
    
    /**
     * @brief Tamanho fixo das instruções RV32I em bytes
     */
    static constexpr int INSTRUCTION_SIZE = 4;
    
    /**
     * @brief Máscara para extrair opcode (bits [6:0])
     */
    static constexpr uint32_t OPCODE_MASK = 0x7F;
    
    /**
     * @brief Máscara para verificar alinhamento de instrução
     */
    static constexpr uint32_t ALIGNMENT_MASK = 0x03;

signals:
    /**
     * @brief Signal emitido quando uma instrução inválida é detectada
     * @param pc PC onde a instrução foi buscada
     * @param rawInstruction Valor bruto da instrução
     */
    void invalidInstruction(RV32Addr pc, uint32_t rawInstruction);
    
    /**
     * @brief Signal emitido quando há erro de alinhamento
     * @param pc PC não alinhado
     */
    void alignmentError(RV32Addr pc);

private:
    //////////////////////////////////////////////////
    // Métodos auxiliares de extração de campos
    //////////////////////////////////////////////////
    
    /**
     * @brief Extrai opcode (bits [6:0])
     */
    static uint8_t extractOpcode(uint32_t instr) {
        return static_cast<uint8_t>(instr & OPCODE_MASK);
    }
    
    /**
     * @brief Extrai rd (bits [11:7])
     */
    static uint8_t extractRd(uint32_t instr) {
        return static_cast<uint8_t>((instr >> 7) & 0x1F);
    }
    
    /**
     * @brief Extrai funct3 (bits [14:12])
     */
    static uint8_t extractFunct3(uint32_t instr) {
        return static_cast<uint8_t>((instr >> 12) & 0x07);
    }
    
    /**
     * @brief Extrai rs1 (bits [19:15])
     */
    static uint8_t extractRs1(uint32_t instr) {
        return static_cast<uint8_t>((instr >> 15) & 0x1F);
    }
    
    /**
     * @brief Extrai rs2 (bits [24:20])
     */
    static uint8_t extractRs2(uint32_t instr) {
        return static_cast<uint8_t>((instr >> 20) & 0x1F);
    }
    
    /**
     * @brief Extrai funct7 (bits [31:25])
     */
    static uint8_t extractFunct7(uint32_t instr) {
        return static_cast<uint8_t>((instr >> 25) & 0x7F);
    }
    
    //////////////////////////////////////////////////
    // Métodos de decode por tipo
    //////////////////////////////////////////////////
    
    /**
     * @brief Decodifica instrução do tipo R
     */
    void decodeRType(uint32_t instr, DecodedInstruction &decoded);
    
    /**
     * @brief Decodifica instrução do tipo I
     */
    void decodeIType(uint32_t instr, DecodedInstruction &decoded);
    
    /**
     * @brief Decodifica instrução do tipo S
     */
    void decodeSType(uint32_t instr, DecodedInstruction &decoded);
    
    /**
     * @brief Decodifica instrução do tipo B
     */
    void decodeBType(uint32_t instr, DecodedInstruction &decoded);
    
    /**
     * @brief Decodifica instrução do tipo U
     */
    void decodeUType(uint32_t instr, DecodedInstruction &decoded);
    
    /**
     * @brief Decodifica instrução do tipo J
     */
    void decodeJType(uint32_t instr, DecodedInstruction &decoded);
    
    //////////////////////////////////////////////////
    // Extensão de sinal
    //////////////////////////////////////////////////
    
    /**
     * @brief Estende um valor de N bits para 32 bits com sinal
     * @param value Valor a estender
     * @param bitWidth Largura original em bits
     * @return Valor estendido com sinal
     */
    static int32_t signExtend(uint32_t value, int bitWidth);
};

} // namespace riscv

#endif // RISCV_DECODER_H
