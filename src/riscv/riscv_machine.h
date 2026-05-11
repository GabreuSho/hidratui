#ifndef RISCV_MACHINE_H
#define RISCV_MACHINE_H

#include <QObject>
#include <QString>
#include <QVector>
#include <cstdint>

#include "core/machine.h"
#include "riscv_alu.h"
#include "riscv_decoder.h"
#include "riscv_register_file.h"

/**
 * @brief Máquina RISC-V RV32I para o simulador Hidra
 *
 * Implementação single-cycle do conjunto de instruções RISC-V RV32I base.
 *
 * Características:
 * - 32 registradores de propósito geral (x0-x31)
 * - Memória de 32-bit word-addressable (ou byte-addressable conforme
 * configuração)
 * - Instruções de 32 bits fixas (sem suporte a compressed/C-extension neste
 * momento)
 * - Ciclo single-cycle: Fetch → Decode → Execute → Memory → Writeback
 * - Endianness configurável (padrão: little-endian conforme RISC-V spec)
 *
 * Integração com Hidra:
 * - Herda de Machine para compatibilidade com UI e framework existente
 * - Override dos métodos virtuais principais (decodeInstruction,
 * executeInstruction)
 * - RegisterFile e ALU isolados em componentes próprios
 */

namespace riscv {

class RiscVMachine : public Machine {
  Q_OBJECT

public:
  explicit RiscVMachine(QObject *parent = nullptr);
  ~RiscVMachine() override;

  //////////////////////////////////////////////////
  // Interface Machine (overrides)
  //////////////////////////////////////////////////

  /**
   * @brief Decodifica a instrução corrente
   *
   * Override do método virtual da classe Machine.
   * Usa o RiscVDecoder para parsear a instrução de 32 bits.
   */
  void decodeInstruction() override;

  /**
   * @brief Executa a instrução decodificada
   *
   * Override do método virtual da classe Machine.
   * Implementa o ciclo Execute → Memory → Writeback.
   */
  void executeInstruction() override;

  /**
   * @brief Realiza um passo de simulação completo
   *
   * Reimplementa o método da classe Machine para implementar o ciclo
   * single-cycle RISC-V:
   * 1. Fetch: Busca instrução da memória (4 bytes)
   * 2. Decode: Decodifica usando RiscVDecoder
   * 3. Execute: Executa na ALU
   * 4. Memory: Acessa memória se load/store
   * 5. Writeback: Escreve resultado no register file
   */
  void step(); // Reimplementação do método da base (não é virtual)

  /**
   * @brief Inicializa a máquina RISC-V
   *
   * Configura:
   * - 32 registradores GPR + PC
   * - Memória (tamanho configurável, padrão 4KB)
   * - Endianness (little-endian por padrão)
   * - Instruções (para compatibilidade com assembler existente)
   */
  void initialize();

  /**
   * @brief Reseta o estado da CPU
   *
   * Zera todos os registradores (exceto x0 que é sempre 0),
   * reseta PC para endereço inicial, limpa flags.
   */
  void reset(); // Reimplementação do método da base (não é virtual)

  /**
   * @brief Limpa todos os dados da máquina
   */
  void clear() override;

  //////////////////////////////////////////////////
  // Métodos específicos RISC-V
  //////////////////////////////////////////////////

  /**
   * @brief Obtém referência ao Register File
   */
  RiscVRegisterFile &getRegisterFile() { return regFile_; }
  const RiscVRegisterFile &getRegisterFile() const { return regFile_; }

  //////////////////////////////////////////////////
  // Convenience methods para testes
  //////////////////////////////////////////////////

  /**
   * @brief Lê valor de um registrador
   * @param regId ID do registrador (0-31)
   * @return Valor do registrador
   */
  uint32_t readRegister(int regId) { return regFile_.readRegister(regId); }

  /**
   * @brief Escreve valor em um registrador
   * @param regId ID do registrador (0-31)
   * @param value Valor a escrever
   * @return true se escrito com sucesso, false se falhar (ex: x0)
   */
  bool writeRegister(int regId, uint32_t value) {
    return regFile_.writeRegister(regId, value);
  }

  /**
   * @brief Lê palavra da memória
   * @param addr Endereço (deve ser alinhado a 4 bytes)
   * @return Valor lido
   */
  uint32_t readMemory(uint32_t addr) { return memory[addr / 4]; }

  /**
   * @brief Escreve palavra na memória
   * @param addr Endereço (deve ser alinhado a 4 bytes)
   * @param value Valor a escrever
   */
  void writeMemory(uint32_t addr, uint32_t value) {
    if (addr % 4 == 0 && addr < memory.size()) {
      memory[addr / 4] = value;
    }
  }

  /**
   * @brief Obtém a última instrução decodificada
   */
  const DecodedInstruction &getCurrentInstruction() const {
    return currentInstr_;
  }

  /**
   * @brief Verifica se há um hazard de controle pendente
   *
   * Usado para detecção básica de hazards em implementação single-cycle.
   * Em single-cycle puro, não há hazards, mas deixamos hook para futuro
   * pipeline.
   */
  bool hasControlHazard() const { return controlHazard_; }

  /**
   * @brief Define o endereço de início de execução
   * @param address Endereço inicial (deve ser alinhado a 4 bytes)
   */
  void setStartAddress(uint32_t address);

  //////////////////////////////////////////////////
  // Assembler (stubs para futura implementação)
  //////////////////////////////////////////////////

  /**
   * @brief Monta código assembly RISC-V
   *
   * Nota: Implementação completa do assembler RISC-V será feita
   * em componente separado (RiscVAssembler). Este método é stub
   * para compatibilidade com interface Machine.
   */
  void assemble(
      QString sourceCode); // Reimplementação do método da base (não é virtual)

signals:
  /**
   * @brief Signal emitido após cada ciclo de instrução
   * @param pc PC atual
   * @param instruction Instrução executada
   */
  void instructionExecuted(RV32Addr pc, const DecodedInstruction &instruction);

  /**
   * @brief Signal emitido quando ocorre uma exceção/trap
   * @param cause Código da causa da exceção
   * @param value Valor associado (ex: endereço que causou fault)
   */
  void trapOccurred(int cause, uint32_t value);

  /**
   * @brief Signal emitido em acesso inválido à memória
   * @param address Endereço acessado
   * @param isWrite true se era operação de escrita
   */
  void memoryAccessFault(uint32_t address, bool isWrite);

protected:
  //////////////////////////////////////////////////
  // Fases do ciclo single-cycle
  //////////////////////////////////////////////////

  /**
   * @brief Fase de Fetch
   *
   * Busca 4 bytes da memória no endereço do PC.
   * Combina bytes em palavra de 32 bits (considerando endianness).
   *
   * @return true se fetch foi bem-sucedido
   */
  bool fetch();

  /**
   * @brief Fase de Execute
   *
   * Executa operação na ALU baseada na instrução decodificada.
   * Calcula endereços efetivos para loads/stores.
   * Determina target de branches/jumps.
   */
  void execute();

  /**
   * @brief Fase de Memory
   *
   * Realiza acesso à memória para loads e stores.
   * Para loads: lê da memória e prepara para writeback.
   * Para stores: escreve na memória.
   */
  void memoryAccess();

  /**
   * @brief Fase de Writeback
   *
   * Escreve resultado no register file (rd).
   * Não escreve se rd == 0 (x0 é hardwired zero).
   */
  void writeBack();

  /**
   * @brief Atualiza o PC para próxima instrução
   *
   * Para branches: atualiza se condição for verdadeira.
   * Para jumps: atualiza incondicionalmente.
   * Caso contrário: PC += 4.
   */
  void updatePC();

  //////////////////////////////////////////////////
  // Helpers
  //////////////////////////////////////////////////

  /**
   * @brief Lê palavra de 32 bits da memória
   * @param address Endereço (deve ser alinhado a 4)
   * @param success Recebe true se leitura foi bem-sucedida
   * @return Palavra de 32 bits
   */
  uint32_t readWord(uint32_t address, bool &success);

  /**
   * @brief Escreve palavra de 32 bits na memória
   * @param address Endereço (deve ser alinhado a 4)
   * @param value Valor a escrever
   * @return true se escrita foi bem-sucedida
   */
  bool writeWord(uint32_t address, uint32_t value);

  /**
   * @brief Lê byte da memória
   */
  uint8_t readByte(uint32_t address, bool &success);

  /**
   * @brief Escreve byte na memória
   */
  bool writeByte(uint32_t address, uint8_t value);

  /**
   * @brief Lê halfword (16 bits) da memória
   */
  uint16_t readHalfword(uint32_t address, bool &success);

  /**
   * @brief Escreve halfword (16 bits) na memória
   */
  bool writeHalfword(uint32_t address, uint16_t value);

private:
  // Componentes principais
  RiscVRegisterFile regFile_; // Register file (x0-x31, PC, CSRs)
  RiscVDecoder decoder_;      // Instruction decoder
  RiscVALU alu_;              // Arithmetic Logic Unit

  // Estado da execução corrente
  DecodedInstruction currentInstr_; // Instrução decodificada atual
  uint32_t fetchedInstr_;           // Instrução bruta fetchada
  uint32_t aluResult_;              // Resultado da ALU
  uint32_t memoryData_;             // Dados lidos/escritos da memória
  uint32_t nextPC_;                 // Próximo valor do PC
  bool controlHazard_;              // Flag de hazard de controle

  // Estado de operações
  bool isLoad_;      // true se instrução corrente é load
  bool isStore_;     // true se instrução corrente é store
  bool isBranch_;    // true se instrução corrente é branch
  bool isJump_;      // true se instrução corrente é jump
  bool branchTaken_; // true se branch foi tomado

  // Configuração
  bool littleEndian_;     // Endianness da memória
  uint32_t startAddress_; // Endereço inicial de execução

  // Contadores para estatísticas
  uint64_t cycleCount_;        // Número de ciclos executados
  uint64_t instructionCount_;  // Número de instruções executadas
  uint64_t memoryAccessCount_; // Número de acessos à memória
};

} // namespace riscv

#endif // RISCV_MACHINE_H
