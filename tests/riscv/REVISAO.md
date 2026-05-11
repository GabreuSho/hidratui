# Checklist de Refatoração - Integração RISC-V no Hidra

Este documento lista ajustes necessários na base de código existente do Hidra para integrar o núcleo RISC-V, ordenados por prioridade e impacto.

---

## 🔴 PRIORIDADE CRÍTICA (Bloqueadores)

### 1. CMakeLists.txt Principal
**Impacto:** Alto  
**Esforço:** Baixo  
**Local:** `/workspace/src/CMakeLists.txt` ou `/workspace/CMakeLists.txt`

**Ajuste necessário:**
```cmake
# Adicionar após outras máquinas
add_subdirectory(riscv)

# Linkar biblioteca RISC-V ao executável principal
target_link_libraries(${EXECUTABLE_NAME} PRIVATE hidrariscv)
```

**Risco:** Build falha sem isso.

---

### 2. Factory de Máquinas
**Impacto:** Alto  
**Esforço:** Médio  
**Local:** Provavelmente em `src/machines/machine_factory.{h,cpp}` ou similar

**Ajuste necessário:**
```cpp
// Incluir header RISC-V
#include "riscv/riscv_machine.h"

// Adicionar caso no factory
Machine* MachineFactory::createMachine(MachineType type) {
    switch (type) {
        // ... casos existentes
        case MachineType::RiscV:
            return new riscv::RiscVMachine(parent);
        default:
            return nullptr;
    }
}
```

**Risco:** UI não consegue instanciar máquina RISC-V.

---

### 3. Enum MachineType
**Impacto:** Alto  
**Esforço:** Baixo  
**Local:** `src/machines/machine_types.h` ou onde estiver definido

**Ajuste necessário:**
```cpp
enum class MachineType {
    Pitagoras,
    Neander,
    Ramses,
    Ahmes,
    Pericles,
    Cromag,
    Volta,
    Queops,
    Reg,
    RiscV,  // <- ADICIONAR
    Unknown
};
```

**Risco:** Type safety quebrada.

---

## 🟠 PRIORIDADE ALTA (Funcionalidade Básica)

### 4. UI - Selector de Máquinas
**Impacto:** Médio-Alto  
**Esforço:** Baixo  
**Local:** `src/ui/main_window.cpp` ou `machine_selector_dialog.cpp`

**Ajuste necessário:**
```cpp
// Adicionar RISC-V à lista de opções
ui->machineTypeCombo->addItem("RISC-V RV32I", 
                               static_cast<int>(MachineType::RiscV));
```

**Risco:** Usuário não pode selecionar RISC-V via UI.

---

### 5. Sistema de Assembler/Parser
**Impacto:** Médio-Alto  
**Esforço:** Alto  
**Local:** `src/assembler/` (se existir)

**Decisão necessária:**
- Opção A: Criar assembler RISC-V separado (recomendado)
- Opção B: Estender assembler existente (mais complexo)

**Recomendação:**
```cpp
// Criar src/riscv/riscv_assembler.{h,cpp}
// Não modificar assemblers legados
```

**Risco:** Sem assembler, usuários precisam carregar binários manualmente.

---

### 6. Sistema de Memória Unificado
**Impacto:** Médio  
**Esforço:** Médio  
**Local:** `src/memory/` ou interface da classe `Machine`

**Verificação necessária:**
```cpp
// Machine base deve ter métodos virtuais para memória
class Machine : public QObject {
    Q_OBJECT
public:
    virtual quint32 readMemoryWord(quint32 addr) = 0;
    virtual void writeMemoryWord(quint32 addr, quint32 value) = 0;
    virtual void loadProgram(const QByteArray& program) = 0;
    // ...
};
```

**Se não existir:** Adicionar na classe base ou usar composition.

**Risco:** RiscVMachine não integra com sistema de memória existente.

---

## 🟡 PRIORIDADE MÉDIA (Melhorias)

### 7. Signals/Slots de Debug
**Impacto:** Médio  
**Esforço:** Baixo-Médio  
**Local:** `src/ui/debug_widget.cpp` ou similar

**Ajuste necessário:**
```cpp
// Conectar signals do RISC-V aos widgets de debug
connect(riscvMachine, &riscv::RiscVMachine::registerChanged,
        debugWidget, &DebugWidget::updateRegister);

connect(riscvMachine, &riscv::RiscVMachine::instructionExecuted,
        logWidget, &LogWidget::appendInstruction);
```

**Risco:** Widgets de debug não atualizam para RISC-V.

---

### 8. Formato de Arquivo de Programa
**Impacto:** Médio  
**Esforço:** Médio  
**Local:** `src/file_formats/` ou parser de programas

**Ajuste necessário:**
```cpp
// Suportar formatos RISC-V
enum class ProgramFormat {
    Binary,
    IntelHex,
    MotorolaS,
    RiscVAssembly,  // <- ADICIONAR
    Elf             // <- OPCIONAL
};
```

**Risco:** Não carregar programas assembly/texto RISC-V.

---

### 9. Logging System
**Impacto:** Baixo-Médio  
**Esforço:** Baixo  
**Local:** `src/logging/` ou sistema de logs

**Ajuste opcional:**
```cpp
// Adicionar categoria para logs RISC-V
qputenv("QT_LOGGING_RULES", "riscv.*=true");

// Ou registrar filter rule
QLoggingCategory::setFilterRules("riscv.decoder=false\n"
                                 "riscv.alu=false\n"
                                 "riscv.memory=true");
```

**Risco:** Logs RISC-V não aparecem ou poluem output.

---

## 🟢 PRIORIDADE BAIXA (Opcionais/Nice-to-have)

### 10. Serialização/Save State
**Impacto:** Baixo  
**Esforço:** Médio  
**Local:** Sistema de save/load state

**Ajuste opcional:**
```cpp
// Se houver sistema de snapshot
void RiscVMachine::saveState(QDataStream& out) {
    out << regFile_.getPC();
    for (int i = 0; i < 32; ++i) {
        out << regFile_.readRegister(i);
    }
    // ... memory, csrs, etc.
}

void RiscVMachine::loadState(QDataStream& in) {
    quint32 pc;
    in >> pc;
    regFile_.setPC(pc);
    // ...
}
```

**Risco:** Não poder salvar/restaurar estado de máquinas RISC-V.

---

### 11. Performance Profiling
**Impacto:** Baixo  
**Esforço:** Baixo  
**Local:** Sistema de profiling (se existir)

**Ajuste opcional:**
```cpp
// Habilitar contadores de performance
class RiscVMachine : public Machine {
    // ...
    quint64 instructionCount_ = 0;
    quint64 cycleCount_ = 0;
    
    quint64 getInstructionCount() const override { 
        return instructionCount_; 
    }
    
    double getCPI() const {
        return static_cast<double>(cycleCount_) / instructionCount_;
    }
};
```

**Risco:** Sem métricas de performance para RISC-V.

---

### 12. Help/Documentation Integration
**Impacto:** Baixo  
**Esforço:** Baixo  
**Local:** Sistema de help da UI

**Ajuste opcional:**
```cpp
// Adicionar entrada no help
helpSystem->registerMachineDoc(
    MachineType::RiscV,
    ":/docs/riscv_help.html",
    "RISC-V RV32I Instruction Set"
);
```

**Risco:** Usuários sem documentação contextual.

---

## ⚪ PRIORIDADE MÍNIMA (Futuro)

### 13. Plugin System
**Impacto:** Mínimo  
**Esforço:** Alto  
**Local:** Arquitetura de plugins (se planejar)

**Consideração futura:**
```cpp
// Em vez de link estático, carregar como plugin
QObject* plugin = QPluginLoader::load("libhidrariscv.so");
Machine* machine = qobject_cast<Machine*>(plugin);
```

**Risco:** Nenhum imediato, mas limita extensibilidade futura.

---

### 14. Multi-Core Support
**Impacto:** Mínimo  
**Esforço:** Muito Alto  
**Local:** Arquitetura whole-system

**Consideração muito futura:**
```cpp
// Se quiser suportar múltiplos harts RISC-V
class RiscVMultiCoreMachine {
    QVector<RiscVMachine*> harts_;
    SharedMemory* sharedMem_;
    // ...
};
```

**Risco:** Nenhum. Totalmente opcional.

---

## 📊 Matriz de Impacto vs Esforço

| Item | Impacto | Esforço | Prioridade |
|------|---------|---------|------------|
| 1. CMakeLists | Alto | Baixo | 🔴 Crítica |
| 2. Factory | Alto | Médio | 🔴 Crítica |
| 3. MachineType | Alto | Baixo | 🔴 Crítica |
| 4. UI Selector | Médio-Alto | Baixo | 🟠 Alta |
| 5. Assembler | Médio-Alto | Alto | 🟠 Alta |
| 6. Memory Interface | Médio | Médio | 🟠 Alta |
| 7. Debug Signals | Médio | Baixo-Médio | 🟡 Média |
| 8. File Formats | Médio | Médio | 🟡 Média |
| 9. Logging | Baixo-Médio | Baixo | 🟡 Média |
| 10. Save State | Baixo | Médio | 🟢 Baixa |
| 11. Profiling | Baixo | Baixo | 🟢 Baixa |
| 12. Help Docs | Baixo | Baixo | 🟢 Baixa |
| 13. Plugins | Mínimo | Alto | ⚪ Futuro |
| 14. Multi-Core | Mínimo | Muito Alto | ⚪ Futuro |

---

## 🛠️ Plano de Ação Recomendado

### Sprint 1 (Essencial)
1. ✅ CMakeLists.txt
2. ✅ MachineType enum
3. ✅ Factory pattern
4. ✅ UI selector

**Critério de sucesso:** Compila e roda máquina RISC-V básica.

### Sprint 2 (Funcional)
5. ⬜ Assembler RISC-V
6. ⬜ Memory interface
7. ⬜ Debug signals

**Critério de sucesso:** Carrega assembly e debug via UI.

### Sprint 3 (Polimento)
8. ⬜ File formats
9. ⬜ Logging
10. ⬜ Save state

**Critério de sucesso:** Experiência de usuário completa.

### Backlog (Futuro)
- 11-14: Melhorias opcionais

---

## 🧪 Testes de Validação

Após cada ajuste, validar:

```bash
# Build
cd build && cmake .. && make -j$(nproc)

# Teste básico
./hidra --machine riscv --test

# Teste UI (manual)
./hidra
# 1. Nova Máquina → RISC-V
# 2. Carregar programa teste
# 3. Step through instruções
# 4. Verificar registradores/memória
```

---

## 📝 Notas de Implementação

### Código Legado
- **Não modificar** máquinas existentes (Pitagoras, Neander, etc.)
- Usar **composition** em vez de inheritance quando possível
- Manter namespace `riscv::` isolado

### Compatibilidade Retroativa
- Garantir que mudanças na base não quebrem máquinas legadas
- Usar `override` e `final` apropriadamente
- Testar todas as máquinas após mudanças

### Documentação
- Atualizar README principal
- Adicionar RISC-V ao changelog
- Documentar breaking changes (se houver)

---

**Status:** Pronto para implementação  
**Última atualização:** 2025-01-XX  
**Responsável:** Equipe de Desenvolvimento Hidra
