# Pontos de Extensão - Núcleo RISC-V Hidra

Este documento descreve como estender o núcleo RISC-V RV32I para suportar extensões futuras e funcionalidades avançadas.

## 📍 Mapa de Extensões

### 1. Extensão M (Multiplicação e Divisão)

**Localização:** `src/riscv/riscv_alu.{h,cpp}`

**O que adicionar:**

```cpp
// Em riscv_alu.h
enum class ALUOp {
    // ... existentes
    MUL,    // 0x02000033
    MULH,   // 0x02000033 (funct7=0x01)
    MULHSU, // 0x02000033 (funct7=0x02)
    MULHU,  // 0x02000033 (funct7=0x03)
    DIV,    // 0x02000033 (funct7=0x04)
    DIVU,   // 0x02000033 (funct7=0x05)
    REM,    // 0x02000033 (funct7=0x06)
    REMU,   // 0x02000033 (funct7=0x07)
};

// Em riscv_alu.cpp
ALUResult RiscVALU::execute(ALUOp op, uint32_t a, uint32_t b) {
    switch (op) {
        // ... casos existentes
        
        case ALUOp::MUL:
            result = static_cast<uint32_t>((static_cast<int64_t>(a) * static_cast<int64_t>(b)) & 0xFFFFFFFF);
            break;
            
        case ALUOp::MULH:
            result = static_cast<uint32_t>((static_cast<int64_t>(static_cast<int32_t>(a)) * 
                                           static_cast<int64_t>(static_cast<int32_t>(b))) >> 32);
            break;
            
        case ALUOp::DIV:
            if (b == 0) {
                result = 0xFFFFFFFF; // Divisão por zero: retorna -1 (all 1s)
            } else if (a == 0x80000000 && b == 0xFFFFFFFF) {
                result = 0x80000000; // Overflow caso especial
            } else {
                result = static_cast<uint32_t>(static_cast<int32_t>(a) / static_cast<int32_t>(b));
            }
            break;
            
        // ... implementar outros casos
    }
}
```

**Decoder:** Atualizar `riscv_decoder.cpp` para reconhecer funct7=0x01 para instruções M.

---

### 2. Extensão C (Instruções Comprimidas)

**Localização:** `src/riscv/riscv_decoder.{h,cpp}`

**Desafio:** Instruções de 16 bits misturadas com 32 bits.

**Implementação:**

```cpp
// Em riscv_decoder.h
enum class InstructionFormat {
    // ... existentes
    C_RVC,      // Formato comprimido (16 bits)
};

struct DecodedInstruction {
    // ... campos existentes
    bool isCompressed;  // Nova flag
    quint16 encoding16; // Para instruções C
};

// Em riscv_decoder.cpp
bool RiscVDecoder::decode(quint32 addr, quint32 instr_word, DecodedInstruction &instr) {
    // Verifica se é instrução comprimida (2 bits LSB != 0b11)
    if ((instr_word & 0x3) != 0x3) {
        return decodeCompressed(addr, static_cast<quint16>(instr_word), instr);
    }
    
    // Decoder normal 32-bit
    return decodeStandard(addr, instr_word, instr);
}

bool RiscVDecoder::decodeCompressed(quint32 addr, quint16 instr, DecodedInstruction &decoded) {
    // Extrai opcode de 2 bits
    quint8 c_opcode = instr & 0x3;
    
    switch (c_opcode) {
        case 0x0: // Quadrante 0
            // C.ADDI4SPN, C.LW, etc.
            break;
        case 0x1: // Quadrante 1
            // C.ADDI, C.JAL, C.LI, etc.
            break;
        case 0x2: // Quadrante 2
            // C.SW, C.BEQZ, etc.
            break;
    }
    
    decoded.isCompressed = true;
    decoded.encoding16 = instr;
    // Expandir para formato 32-bit equivalente
    return true;
}
```

**Memory Interface:** Ajustar fetch para lidar com alinhamento de 16 bits.

---

### 3. Extensão F/D (Ponto Flutuante)

**Localização:** Novos arquivos `riscv_fpu.{h,cpp}`

**Estrutura:**

```cpp
// riscv_fpu.h
#ifndef RISCV_FPU_H
#define RISCV_FPU_H

#include <QVector>
#include <cstdint>
#include <cmath>

namespace riscv {

using float32 = float;
using float64 = double;

/**
 * @brief Registradores de ponto flutuante (f0-f31)
 */
class RiscVFPU {
public:
    RiscVFPU();
    
    // Operações single-precision
    float32 readF(int regId) const;
    void writeF(int regId, float32 value);
    
    // Operações double-precision
    float64 readD(int regId) const;
    void writeD(int regId, float64 value);
    
    // Operações FPU
    float32 add(float32 a, float32 b);
    float32 sub(float32 a, float32 b);
    float32 mul(float32 a, float32 b);
    float32 div(float32 a, float32 b);
    float32 sqrt(float32 a);
    
    // Conversões
    int32_t ftoi(float32 value);
    float32 itof(int32_t value);
    
    // Flags
    uint32_t getFCSR() const;
    void setFCSR(uint32_t value);
    
private:
    QVector<float32> fprs_;
    uint32_t fcsr_; // Control and Status Register
    
    // Exception flags
    bool invalidOp_;
    bool divByZero_;
    bool overflow_;
    bool underflow_;
    bool inexact_;
};

} // namespace riscv
#endif
```

**Integração com RegisterFile:**

```cpp
// Em riscv_register_file.h
class RiscVRegisterFile : public QObject {
    // ... existente
    
    // Adicionar FPU
    RiscVFPU fpu_;
    
    float32 readFloatRegister(int regId) const;
    void writeFloatRegister(int regId, float32 value);
};
```

---

### 4. CSRs Avançados

**Localização:** `src/riscv/riscv_register_file.{h,cpp}`

**CSRs para implementar:**

```cpp
// Em riscv_register_file.h
enum class CSRId : uint16_t {
    // ... existentes (mstatus, mtvec, etc.)
    
    // Machine Trap Delegation
    medeleg   = 0x302,
    mideleg   = 0x303,
    
    // Machine Counter Setup
    mcounteren = 0x306,
    
    // Supervisor Mode (se implementar S-mode)
    sstatus   = 0x100,
    sie       = 0x104,
    stvec     = 0x105,
    sepc      = 0x141,
    scause    = 0x142,
    stval     = 0x143,
    sip       = 0x144,
    
    // User Mode (se implementar U-mode)
    ustatus   = 0x000,
    uepc      = 0x041,
    
    // Performance Counters
    hpmcounter3  = 0xB03,
    hpmcounter4  = 0xB04,
    // ... até hpmcounter31
    
    // Debug (se implementar debug mode)
    dcsr      = 0x7B0,
    dpc       = 0x7B1,
    dscratch  = 0x7B2,
};

// Implementação de leitura/escrita com permissões
bool RiscVRegisterFile::writeCSR(CSRId csrId, RV32Reg value) {
    // Verifica modo atual (M, S, U)
    RV32Reg mstatus = readCSR(CSRId::mstatus);
    int privMode = (mstatus >> 11) & 0x3; // MPP field
    
    // Verifica permissões baseado no CSR e modo
    if (!hasWritePermission(csrId, privMode)) {
        return false; // Write denied
    }
    
    // Aplica máscara de escrita (alguns bits são read-only)
    value &= getWriteMask(csrId);
    
    csrs_[static_cast<uint16_t>(csrId)] = value;
    emit csrWritten(static_cast<uint16_t>(csrId), value);
    
    return true;
}
```

---

### 5. Sistema de Interrupções e Exceções

**Localização:** `src/riscv/riscv_machine.{h,cpp}`

**Implementação:**

```cpp
// Em riscv_machine.h
enum class ExceptionCode : uint32_t {
    InstructionAddressMisaligned = 0,
    InstructionAccessFault       = 1,
    IllegalInstruction           = 2,
    Breakpoint                   = 3,
    LoadAddressMisaligned        = 4,
    LoadAccessFault              = 5,
    StoreAddressMisaligned       = 6,
    StoreAccessFault             = 7,
    EcallFromUMode               = 8,
    EcallFromSMode               = 9,
    EcallFromMMode               = 11,
    InstructionPageFault         = 12,
    LoadPageFault                = 13,
    StorePageFault               = 15,
};

enum class InterruptCode : uint32_t {
    UserSoftInterrupt    = 0,
    SupervisorSoftInterrupt = 1,
    MachineSoftInterrupt = 3,
    UserTimerInterrupt   = 4,
    SupervisorTimerInterrupt = 5,
    MachineTimerInterrupt = 7,
    UserExternalInterrupt = 8,
    SupervisorExternalInterrupt = 9,
    MachineExternalInterrupt = 11,
};

class RiscVMachine : public Machine {
    // ... existente
    
    // Trap handling
    void triggerException(ExceptionCode code, uint32_t tval = 0);
    void triggerInterrupt(InterruptCode code);
    void handleTrap();
    
    // Interrupt checking
    void checkInterrupts();
    bool interruptsEnabled() const;
    
    // ECALL/EBREAK
    void executeECALL();
    void executeEBREAK();
};

// Em riscv_machine.cpp
void RiscVMachine::triggerException(ExceptionCode code, uint32_t tval) {
    // Salva estado
    regFile_.writeCSR(CSRId::mepc, regFile_.getPC());
    regFile_.writeCSR(CSRId::mcause, static_cast<uint32_t>(code));
    regFile_.writeCSR(CSRId::mtval, tval);
    
    // Calcula vetor de trap
    uint32_t mtvec = regFile_.readCSR(CSRId::mtvec);
    uint32_t base = mtvec & 0xFFFFFFC0;
    uint32_t mode = mtvec & 0x3;
    
    uint32_t trapAddr = (mode == 1) ? base + (static_cast<uint32_t>(code) * 4) : base;
    
    // Jump para handler
    regFile_.setPC(trapAddr);
    
    emit exceptionTriggered(static_cast<uint32_t>(code));
}

void RiscVMachine::executeECALL() {
    // Determina modo atual e gera exceção apropriada
    triggerException(ExceptionCode::EcallFromMMode);
}

void RiscVMachine::executeEBREAK() {
    triggerException(ExceptionCode::Breakpoint);
    // Pode pausar execução se em modo debug
}
```

---

### 6. Pipeline de 5 Estágios (Futuro)

**Localização:** Novo arquivo `riscv_pipeline.{h,cpp}`

**Arquitetura:**

```cpp
// riscv_pipeline.h
enum class PipelineStage {
    IF, // Instruction Fetch
    ID, // Instruction Decode
    EX, // Execute
    MEM, // Memory Access
    WB  // Write Back
};

struct PipelineRegister {
    // Dados entre estágios
    bool valid = false;
    bool stall = false;
    bool bubble = false;
    
    // PC e instrução
    uint32_t pc;
    uint32_t instruction;
    
    // Decode
    DecodedInstruction decoded;
    
    // Execute
    uint32_t aluResult;
    uint32_t rs1Value;
    uint32_t rs2Value;
    
    // Memory
    uint32_t memAddress;
    uint32_t memReadData;
    uint32_t memWriteData;
    
    // Writeback
    int rd;
    uint32_t wbData;
    bool regWrite;
};

class RiscVPipelineCPU : public RiscVMachine {
public:
    RiscVPipelineCPU();
    
    void step() override;
    
    // Hazard detection
    bool detectDataHazard(const PipelineRegister& id, const PipelineRegister& ex);
    bool detectControlHazard(const PipelineRegister& id);
    
    // Forwarding
    uint32_t forwardData(const PipelineRegister& ex, const PipelineRegister& mem,
                        int rs1, int rs2);
    
    // Stall/bubble insertion
    void insertStall(PipelineStage stage);
    void insertBubble(PipelineStage stage);
    
private:
    PipelineRegister if_id_;
    PipelineRegister id_ex_;
    PipelineRegister ex_mem_;
    PipelineRegister mem_wb_;
    
    // Hazard units
    bool hazardDetected_ = false;
    bool forwardingEnabled_ = true;
};
```

---

### 7. MMU e Virtual Memory

**Localização:** Novo arquivo `riscv_mmu.{h,cpp}`

**Estrutura básica:**

```cpp
// riscv_mmu.h
class RiscVMMU {
public:
    RiscVMMU(RiscVRegisterFile& regFile);
    
    // Tradução de endereço virtual para físico
    uint32_t translateAddress(uint32_t vaddr, AccessType type);
    
    // Page table operations
    void loadPageTableBase(uint32_t satp);
    void invalidateTLB();
    
    // Protection checks
    bool checkPermissions(uint32_t pte, AccessType type, int privMode);
    
private:
    struct TLBEntry {
        uint32_t vpn;
        uint32_t ppn;
        bool valid;
        bool dirty;
        bool accessed;
        uint8_t permissions;
    };
    
    QVector<TLBEntry> tlb_;
    RiscVRegisterFile& regFile_;
    
    // Page size (4KB para Sv32)
    static constexpr uint32_t PAGE_SIZE = 4096;
};
```

---

## 📋 Checklist de Implementação

### Prioridade Alta (Core RV32I Completo)
- [ ] Todas as 40+ instruções base implementadas
- [ ] x0 hardwired para zero
- [ ] PC alignment validation
- [ ] Sign-extension correta para immediates
- [ ] Branch offset calculation

### Prioridade Média (Extensão M)
- [ ] MUL, MULH, MULHSU, MULHU
- [ ] DIV, DIVU
- [ ] REM, REMU
- [ ] Tratamento de divisão por zero

### Prioridade Baixa (Funcionalidades Avançadas)
- [ ] Extensão C (compressão)
- [ ] Extensão F/D (float)
- [ ] CSRs completos
- [ ] Sistema de interrupções
- [ ] MMU e virtual memory
- [ ] Pipeline de 5 estágios

---

## 🔧 Ferramentas de Desenvolvimento

### Simulador de Referência
Use o [Spike](https://github.com/riscv-software-src/riscv-isa-sim) como golden model:

```bash
# Instale Spike
git clone https://github.com/riscv-software-src/riscv-isa-sim
cd riscv-isa-sim && mkdir build && cd build
../configure --prefix=$RISCV
make && make install

# Compare saída
spike tests/riscv/programs/soma.bin
./hidra_riscv tests/riscv/programs/soma.bin
# Saídas devem ser idênticas
```

### Assemblers
- **GNU assembler (riscv64-unknown-elf-as)**: Oficial
- **LLVM MC**: Alternativa moderna

### Test Suites
- [riscv-tests](https://github.com/riscv-software-src/riscv-tests): Test suite oficial
- [riscv-arch-test](https://github.com/riscv-non-isa/riscv-arch-test): Tests de arquitetura

---

## 📚 Recursos

- [RISC-V Spec](https://riscv.org/specifications/)
- [RISC-V Reader](http://www.riscvbook.com/)
- [Patterson & Hennessy Appendix](https://github.com/gh-bier/cs205/blob/master/RISC-V-Green-Cards.pdf)
