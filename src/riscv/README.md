# Módulo RISC-V para o Simulador Hidra

Este diretório contém a implementação do núcleo RISC-V RV32I para o simulador Hidra.

## Estrutura de Arquivos

```
riscv/
├── CMakeLists.txt          # Configuração de build
├── riscv_register_file.h   # Register File (x0-x31, PC, CSRs)
├── riscv_register_file.cpp
├── riscv_decoder.h         # Instruction Decoder (formatos R,I,S,B,U,J)
├── riscv_decoder.cpp
├── riscv_alu.h             # Arithmetic Logic Unit
├── riscv_alu.cpp
├── riscv_machine.h         # CPU principal (single-cycle)
└── riscv_machine.cpp
```

## Componentes

### 1. RiscVRegisterFile (`riscv_register_file.h`)
- 32 registradores de propósito geral (x0-x31)
- x0 é hardwired para zero (imutável)
- PC (Program Counter) separado
- CSRs básicos (mstatus, mtvec, mepc, etc.)
- Hooks via Qt signals para debugging

### 2. RiscVDecoder (`riscv_decoder.h`)
- Decode dos 6 formatos RISC-V: R, I, S, B, U, J
- Extração de opcode, funct3, funct7
- Validação de alinhamento (4 bytes)
- Mapeamento para operações ALU internas

### 3. RiscVALU (`riscv_alu.h`)
- Operações aritméticas: ADD, SUB
- Operações lógicas: AND, OR, XOR
- Shifts: SLL, SRL, SRA
- Comparações: SLT, SLTU
- Detecção de overflow para operações signed

### 4. RiscVMachine (`riscv_machine.h`)
- Implementação single-cycle completa
- Ciclo: Fetch → Decode → Execute → Memory → Writeback
- Integração com classe base `Machine` do Hidra
- Suporte a loads/stores (LB, LH, LW, SB, SH, SW)
- Branches condicionais e jumps
- Endianness configurável (padrão: little-endian)

## Instruções Suportadas (RV32I Base)

### Aritméticas/Lógicas (R-type)
- ADD, SUB, AND, OR, XOR
- SLL, SRL, SRA
- SLT, SLTU

### Imediatas (I-type)
- ADDI, ANDI, ORI, XORI
- SLLI, SRLI, SRAI
- SLTI, SLTIU

### Loads/Stores
- LB, LH, LW (loads com sign/zero extend)
- SB, SH, SW (stores)

### Controle de Fluxo
- BEQ, BNE, BLT, BGE, BLTU, BGEU (branches)
- JAL, JALR (jumps)

### Upper Immediate
- LUI (Load Upper Immediate)
- AUIPC (Add Upper Immediate to PC)

### Sistema
- ECALL, EBREAK
- CSRRW, CSRRS, CSRRC (acesso a CSRs)

## Uso Básico

```cpp
#include "riscv/riscv_machine.h"

using namespace riscv;

// Criar instância da máquina
RiscVMachine *machine = new RiscVMachine();

// Configurar endereço inicial
machine->setStartAddress(0x00000000);

// Carregar programa na memória (exemplo manual)
machine->setMemoryValue(0x00, 0x13);  // addi x0, x0, 0 (nop)
machine->setMemoryValue(0x01, 0x00);
machine->setMemoryValue(0x02, 0x00);
machine->setMemoryValue(0x03, 0x00);
// ... carregar resto do programa

// Iniciar simulação
machine->setRunning(true);
machine->step();  // Executa uma instrução

// Acessar registradores
uint32_t value = machine->getRegisterFile().readRegister(10);  // x10 (a0)

// Conectar signals para debugging
connect(machine, &RiscVMachine::instructionExecuted,
        [](RV32Addr pc, const DecodedInstruction &instr) {
            qDebug() << "PC:" << QString("0x%1").arg(pc, 8, 16, QChar('0'))
                     << "Instr:" << RiscVDecoder::formatAssembly(instr);
        });
```

## Integração com Hidra Existente

### No CMakeLists.txt principal (`src/CMakeLists.txt`):

```cmake
# Adicionar subdiretório RISC-V
add_subdirectory(riscv)

# Linkar biblioteca RISC-V ao executável principal
target_link_libraries(${EXECUTABLE_NAME} hidrariscv)
```

### Na UI/Frontend:

```cpp
#include "riscv/riscv_machine.h"

// Criar seletor de arquitetura
if (selectedArchitecture == "RISC-V RV32I") {
    currentMachine = new riscv::RiscVMachine(this);
} else {
    // Máquinas existentes...
}
```

## Decisões de Projeto

### Single-Cycle vs Pipeline
- **Escolha**: Single-cycle
- **Justificativa**: 
  - Alinhado com objetivo pedagógico do Hidra
  - Mais simples de depurar e entender
  - Compatível com arquitetura atual do projeto
  - Pipeline pode ser implementado como máquina separada no futuro

### Isolamento do Código Legado
- Namespace `riscv` próprio para todos os componentes
- Herança da classe `Machine` apenas para interface UI
- RegisterFile, ALU e Decoder são componentes independentes
- Zero modificações no core existente do Hidra

### Endianness
- Padrão: Little-endian (conforme especificação RISC-V)
- Suporte configurável para big-endian (para fins educacionais)

## Próximos Passos / Extensões Futuras

1. **Assembler RISC-V**: Implementar parser de assembly texto → machine code
2. **Extensão M**: Multiplicação e divisão (MUL, DIV, REM, etc.)
3. **Extensão C**: Instruções comprimidas (16-bit)
4. **Pipeline 5 estágios**: Como máquina alternativa
5. **Interrupções/Exceptions**: Tratamento completo de traps
6. **Debugger Integration**: Breakpoints, watchpoints, step-through

## Referências

- RISC-V ISA Specification: https://riscv.org/specifications/
- RISC-V Assembly Programmer's Guide
- Green Card RISC-V: https://github.com/johnwinans/rvalp

## License

Mesma licença do projeto Hidra original.
