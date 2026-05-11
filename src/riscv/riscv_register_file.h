#ifndef RISCV_REGISTER_FILE_H
#define RISCV_REGISTER_FILE_H

#include <QObject>
#include <QVector>
#include <QString>
#include <cstdint>

/**
 * @brief Implementação do Register File para RISC-V RV32I
 * 
 * Características:
 * - 32 registradores de propósito geral (x0-x31), cada um com 32 bits
 * - x0 (zero) é imutável e sempre retorna 0
 * - PC (Program Counter) separado
 * - Suporte básico para CSRs (Control and Status Registers)
 * - Hooks para debugging via signals Qt
 */

namespace riscv {

// Tipos básicos para RV32I
using RV32Reg = uint32_t;
using RV32Addr = uint32_t;

/**
 * @brief Enumeração dos registradores RISC-V
 */
enum class RegId : int {
    x0  = 0,   // zero - hardwired zero
    x1  = 1,   // ra - return address
    x2  = 2,   // sp - stack pointer
    x3  = 3,   // gp - global pointer
    x4  = 4,   // tp - thread pointer
    x5  = 5,   // t0 - temporary
    x6  = 6,   // t1 - temporary
    x7  = 7,   // t2 - temporary
    x8  = 8,   // s0/fp - saved register/frame pointer
    x9  = 9,   // s1 - saved register
    x10 = 10,  // a0 - function argument/return value
    x11 = 11,  // a1 - function argument/return value
    x12 = 12,  // a2 - function argument
    x13 = 13,  // a3 - function argument
    x14 = 14,  // a4 - function argument
    x15 = 15,  // a5 - function argument
    x16 = 16,  // a6 - function argument
    x17 = 17,  // a7 - function argument
    x18 = 18,  // s2 - saved register
    x19 = 19,  // s3 - saved register
    x20 = 20,  // s4 - saved register
    x21 = 21,  // s5 - saved register
    x22 = 22,  // s6 - saved register
    x23 = 23,  // s7 - saved register
    x24 = 24,  // s8 - saved register
    x25 = 25,  // s9 - saved register
    x26 = 26,  // s10 - saved register
    x27 = 27,  // s11 - saved register
    x28 = 28,  // t3 - temporary
    x29 = 29,  // t4 - temporary
    x30 = 30,  // t5 - temporary
    x31 = 31,  // t6 - temporary
    INVALID = -1
};

/**
 * @brief Nomes ABI dos registradores (para display/debug)
 */
const char* const REG_ABI_NAMES[32] = {
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

/**
 * @brief IDs de CSRs básicos (RV32I)
 */
enum class CSRId : uint16_t {
    // Machine Information Registers
    mvendorid = 0xF11,
    marchid   = 0xF12,
    mimpid    = 0xF13,
    mhartid   = 0xF14,
    
    // Machine Trap Setup
    mstatus   = 0x300,
    mie       = 0x304,
    mtvec     = 0x305,
    
    // Machine Trap Handling
    mscratch  = 0x340,
    mepc      = 0x341,
    mcause    = 0x342,
    mtval     = 0x343,
    mip       = 0x344,
    
    // Machine Counter/Timers
    mcycle    = 0xB00,
    minstret  = 0xB02,
    
    // Cycle counter (alias)
    cycle     = 0xC00
};

/**
 * @brief Classe que gerencia o Register File RISC-V
 */
class RiscVRegisterFile : public QObject {
    Q_OBJECT

public:
    explicit RiscVRegisterFile(QObject *parent = nullptr);
    ~RiscVRegisterFile() = default;

    //////////////////////////////////////////////////
    // Leitura/Escrita de Registradores
    //////////////////////////////////////////////////
    
    /**
     * @brief Lê o valor de um registrador GPR
     * @param regId ID do registrador (0-31)
     * @return Valor do registrador (sempre 0 para x0)
     */
    RV32Reg readRegister(int regId) const;
    
    /**
     * @brief Lê o valor de um registrador pelo nome ABI
     * @param name Nome ABI (ex: "zero", "ra", "a0")
     * @return Valor do registrador ou 0 se inválido
     */
    RV32Reg readRegisterByName(const QString &name) const;
    
    /**
     * @brief Escreve um valor em um registrador GPR
     * @param regId ID do registrador (0-31)
     * @param value Valor a escrever (ignorado se regId == 0)
     * @return true se a escrita foi bem-sucedida
     */
    bool writeRegister(int regId, RV32Reg value);
    
    /**
     * @brief Escreve um valor em um registrador pelo nome ABI
     * @param name Nome ABI do registrador
     * @param value Valor a escrever
     * @return true se a escrita foi bem-sucedida
     */
    bool writeRegisterByName(const QString &name, RV32Reg value);

    //////////////////////////////////////////////////
    // Program Counter
    //////////////////////////////////////////////////
    
    /**
     * @brief Lê o valor atual do PC
     */
    RV32Addr getPC() const { return pc_; }
    
    /**
     * @brief Define um novo valor para o PC
     * @param address Novo endereço (será alinhado a 4 bytes)
     */
    void setPC(RV32Addr address);
    
    /**
     * @brief Incrementa o PC
     * @param increment Valor a incrementar (padrão: 4 bytes para RV32I)
     */
    void incrementPC(RV32Addr increment = 4);

    //////////////////////////////////////////////////
    // CSRs (Control and Status Registers)
    //////////////////////////////////////////////////
    
    /**
     * @brief Lê um CSR
     * @param csrId ID do CSR
     * @return Valor do CSR ou 0 se não implementado
     */
    RV32Reg readCSR(CSRId csrId) const;
    
    /**
     * @brief Lê um CSR por ID numérico
     */
    RV32Reg readCSRById(uint16_t csrId) const;
    
    /**
     * @brief Escreve em um CSR
     * @param csrId ID do CSR
     * @param value Valor a escrever
     * @return true se a escrita foi permitida
     */
    bool writeCSR(CSRId csrId, RV32Reg value);
    
    /**
     * @brief Escreve em um CSR por ID numérico
     */
    bool writeCSRById(uint16_t csrId, RV32Reg value);

    //////////////////////////////////////////////////
    // Utilitários
    //////////////////////////////////////////////////
    
    /**
     * @brief Obtém o número total de registradores GPR
     */
    static constexpr int NUM_GPRS = 32;
    
    /**
     * @brief Obtém o tamanho em bits dos registradores
     */
    static constexpr int REG_BIT_WIDTH = 32;
    
    /**
     * @brief Verifica se um ID de registrador é válido
     */
    static bool isValidRegId(int regId) {
        return regId >= 0 && regId < NUM_GPRS;
    }
    
    /**
     * @brief Obtém o nome ABI de um registrador
     */
    static QString getRegName(int regId);
    
    /**
     * @brief Obtém o ID de um registrador pelo nome ABI
     * @return ID do registrador ou -1 se não encontrado
     */
    static int getRegIdByName(const QString &name);
    
    /**
     * @brief Reseta todos os registradores (exceto x0 que é sempre 0)
     */
    void reset();
    
    /**
     * @brief Reseta apenas o PC
     */
    void resetPC(RV32Addr startAddress = 0);

signals:
    /**
     * @brief Signal emitido quando um registrador é escrito
     * @param regId ID do registrador modificado
     * @param oldValue Valor anterior
     * @param newValue Novo valor
     */
    void registerWritten(int regId, RV32Reg oldValue, RV32Reg newValue);
    
    /**
     * @brief Signal emitido quando o PC muda
     * @param oldPC Valor anterior do PC
     * @param newPC Novo valor do PC
     */
    void pcChanged(RV32Addr oldPC, RV32Addr newPC);
    
    /**
     * @brief Signal emitido quando um CSR é modificado
     * @param csrId ID do CSR
     * @param newValue Novo valor
     */
    void csrWritten(uint16_t csrId, RV32Reg newValue);

private:
    // Registradores de propósito geral (x0-x31)
    QVector<RV32Reg> gprs_;
    
    // Program Counter
    RV32Addr pc_;
    
    // Control and Status Registers (básicos)
    QHash<uint16_t, RV32Reg> csrs_;
    
    // Contador de ciclos (para mcycle)
    uint64_t cycleCount_;
    
    /**
     * @brief Aplica máscara de 32 bits a um valor
     */
    static RV32Reg maskTo32Bits(uint64_t value) {
        return static_cast<RV32Reg>(value & 0xFFFFFFFFu);
    }
    
    /**
     * @brief Verifica se um CSR é read-only
     */
    bool isReadOnlyCSR(CSRId csrId) const;
};

} // namespace riscv

#endif // RISCV_REGISTER_FILE_H
