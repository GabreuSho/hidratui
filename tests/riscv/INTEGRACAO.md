# Guia de Integração RISC-V no Hidra

Este documento descreve como compilar, rodar, depurar e estender o núcleo RISC-V RV32I no simulador Hidra.

## 📋 Pré-requisitos

- CMake 3.16+
- Qt 5.15+ (ou Qt 6)
- Compilador C++17 ou superior (GCC 9+, Clang 10+, MSVC 2019+)
- Git

## 🔨 Compilação

### 1. Configurar Build

```bash
cd /workspace
mkdir build && cd build
cmake .. -DENABLE_RISCV=ON
```

### 2. Opções de CMake

```bash
# Habilitar RISC-V (default: ON)
-DENABLE_RISCV=ON

# Build type
-DCMAKE_BUILD_TYPE=Debug    # Com símbolos de debug
-DCMAKE_BUILD_TYPE=Release  # Otimizado

# Verbose output
-DCMAKE_VERBOSE_MAKEFILE=ON
```

### 3. Compilar

```bash
# Build completo
make -j$(nproc)

# Ou apenas módulo RISC-V
make hidrariscv
```

### 4. Rodar Testes Unitários

```bash
# Compilar testes
cd /workspace/tests/riscv
g++ -std=c++17 -I../../src/riscv \
    riscv_tests.cpp \
    ../../build/src/riscv/libhidrariscv.a \
    -lQt5Core -lQt5Test -o riscv_tests

# Executar testes
./riscv_tests
```

Ou com CMake:

```bash
cd /workspace/build
ctest -R riscv --verbose
```

## ▶️ Executando Programas RISC-V

### Via UI do Hidra

1. Abra o Hidra
2. Menu **Arquivo → Nova Máquina**
3. Selecione **RISC-V RV32I**
4. Carregue programa assembly ou binário
5. Use controles de execução:
   - **Step**: Executa uma instrução
   - **Run**: Executa continuamente
   - **Pause**: Interrompe execução
   - **Reset**: Reinicia CPU

### Via Linha de Comando (se suportado)

```bash
./hidra --machine riscv --program tests/riscv/programs/soma.s
```

### Carregando Assembly

Se houver assembler integrado:

```cpp
#include "riscv/riscv_assembler.h"

auto *machine = new riscv::RiscVMachine();
riscv::RiscVAssembler assembler;

assembler.loadFile("tests/riscv/programs/soma.s");
assembler.assemble(machine);

machine->run();
```

### Carregando Binário

```cpp
auto *machine = new riscv::RiscVMachine();

// Carrega binário bruto na memória
machine->loadBinary("program.bin", 0x0);

machine->run();
```

## 🐛 Debugging

### Inspecionando Estado da CPU

```cpp
#include "riscv/riscv_machine.h"

auto *cpu = new riscv::RiscVMachine();

// Ler registradores
for (int i = 0; i < 32; ++i) {
    qDebug() << "x" << i << "=" << cpu->readRegister(i);
}

// Ler PC
qDebug() << "PC =" << cpu->getPC();

// Ler memória
quint32 value = cpu->readMemoryWord(0x100);
qDebug() << "mem[0x100] =" << value;
```

### Hooks via Signals Qt

```cpp
// Conecta a signals para monitorar execução
connect(cpu, &riscv::RiscVMachine::instructionExecuted,
        [](quint32 pc, quint32 instr) {
            qDebug() << "Exec:" << hex << pc << ":" << instr;
        });

connect(cpu, &riscv::RiscVMachine::registerChanged,
        [](int regId, quint32 newValue) {
            qDebug() << "x" << regId << "<=" << newValue;
        });
```

### Logging de Instruções

Habilite logging verbose:

```cpp
// No código ou via environment variable
qputenv("RISCV_LOG", "1");

// Ou chame diretamente
cpu->setInstructionLogging(true);
```

Saída típica:

```
[0x000] ADDI x1, x0, 10    ; x1 <= 10
[0x004] ADDI x2, x0, 20    ; x2 <= 20
[0x008] ADD  x3, x1, x2    ; x3 <= 30
```

### Breakpoints

```cpp
// Set breakpoint em endereço
cpu->setBreakpoint(0x100);

// Set breakpoint condicional
cpu->setBreakpoint(0x200, []() {
    return cpu->readRegister(1) == 42;
});

// Remove breakpoint
cpu->removeBreakpoint(0x100);

// Clear all
cpu->clearBreakpoints();
```

### Watchpoints

```cpp
// Watch em região de memória
cpu->setMemoryWatch(0x1000, 0x100); // Watch 256 bytes a partir de 0x1000
```

## 📊 Visualizando Estado

### Dump de Registradores

```cpp
cpu->dumpRegisters();
```

Saída:
```
Register File:
  x0(zero): 0x00000000   x1(ra):   0x00000000
  x2(sp):   0x00001000   x3(gp):   0x00000000
  ...
  PC:       0x0000000C
```

### Dump de Memória

```cpp
cpu->dumpMemory(0x0, 0x100); // Dump 256 bytes a partir de 0x0
```

### Trace de Execução

```cpp
cpu->enableTrace(true);
cpu->step(); // Gera trace
cpu->printTrace();
```

## ⚙️ Configuração Avançada

### Memory Size

```cpp
// Default é 64KB, pode ser configurado
auto *cpu = new riscv::RiscVMachine(0x100000); // 1MB
```

### Endianness

RISC-V suporta ambos little-endian (default) e big-endian:

```cpp
// Forçar little-endian (default)
cpu->setEndianness(riscv::Endianness::Little);

// Big-endian (se implementado)
cpu->setEndianness(riscv::Endianness::Big);
```

### Cycle Counting

```cpp
// Habilita contador de ciclos
cpu->enableCycleCounter(true);

// Lê ciclos executados
quint64 cycles = cpu->getCycleCount();
qDebug() << "Cycles:" << cycles;

// CPI (Cycles Per Instruction)
double cpi = static_cast<double>(cycles) / cpu->getInstructionCount();
```

## 🧪 Testando com Programas Exemplo

Os programas de teste estão em `tests/riscv/programs/`:

| Programa | Descrição | Instruções Testadas |
|----------|-----------|---------------------|
| `soma.s` | Soma simples | ADDI, ADD, JAL |
| `loop.s` | Loop com contador | ADDI, BLT, JAL |
| `cond.s` | Branches condicionais | BEQ, BNE, BLT, BGE |
| `memory.s` | Load/Store | LW, SW, LH, SH, LB, SB |
| `stack.s` | Operações de stack | ADDI, SW, LW |

### Executando soma.s

```bash
# No Hidra UI
1. File → Load Program → tests/riscv/programs/soma.s
2. Step até instrução 3 (ADD x3, x1, x2)
3. Verifique x3 = 75 (0x4B)
```

### Executando memory.s

```bash
1. Load memory.s
2. Step através das instruções
3. Inspecione memória em 0x64 (100), 0x68 (104)
4. Verifique x3 = 42, x5 = 129
```

## 📝 Troubleshooting

### Problema: Instruções não decodificam corretamente

**Solução:** Verifique alinhamento de 4 bytes. RISC-V requer instruções alinhadas.

```cpp
if (!decoder.isAligned(pc)) {
    qWarning() << "Unaligned instruction at" << pc;
}
```

### Problema: Load/Store retorna valores incorretos

**Solução:** Verifique endianness e alinhamento de acesso.

```cpp
// Acessos a word devem ser alinhados a 4 bytes
// Acessos a halfword devem ser alinhados a 2 bytes
```

### Problema: Branch não toma caminho esperado

**Solução:** Verifique cálculo de offset. Offsets em branches são relativos ao PC.

```cpp
// Offset é calculado como: target = PC + imm
// Imm já está em unidades de 4 bytes
```

### Problema: x0 não é zero

**Solução:** Isso é um bug crítico. RegisterFile deve hardwired x0 para 0.

```cpp
// Em riscv_register_file.cpp
RV32Reg RiscVRegisterFile::readRegister(int regId) const {
    if (regId == 0) return 0; // Hardwired!
    return gprs_[regId];
}
```

## 🔗 Integração com Outras Máquinas

Para adicionar suporte a múltiplas arquiteturas:

```cpp
// Factory pattern
enum class MachineType {
    Pitagoras,
    Neander,
    Ramses,
    RiscV
};

Machine* createMachine(MachineType type) {
    switch (type) {
        case MachineType::RiscV:
            return new riscv::RiscVMachine();
        // ... outros casos
    }
}
```

## 📚 Próximos Passos

Após integração básica:

1. **Adicionar Assembler**: Parser de assembly RISC-V textual
2. **Suporte a Extensões**: M (multiplicação), C (compressão), F/D (float)
3. **Debugger GUI**: Interface gráfica para breakpoints, watchpoints
4. **Performance Counters**: Estatísticas detalhadas de execução
5. **Pipeline**: Implementar versão pipeline de 5 estágios

---

**Documentação Adicional:**
- Especificação RISC-V: https://riscv.org/specifications/
- README do módulo: `/workspace/src/riscv/README.md`
- Exemplos de uso: `/workspace/src/riscv/riscv_example.cpp`
