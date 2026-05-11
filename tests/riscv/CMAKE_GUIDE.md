# Guia de Compilação e Configuração CMake - RISC-V no Hidra

## Visão Geral

Este documento descreve as modificações feitas nos arquivos CMake para integrar o módulo RISC-V ao projeto Hidra.

## Estrutura de Diretórios

```
/workspace/
├── CMakeLists.txt              # Principal (modificado)
├── src/
│   ├── CMakeLists.txt          # Build GUI (não modificado - legado)
│   ├── riscv/
│   │   └── CMakeLists.txt      # Módulo RISC-V (modificado)
│   └── tests/
│       ├── CMakeLists.txt      # Tests framework (modificado)
│       └── simulationtests/
│           ├── CMakeLists.txt  # Simulation tests (modificado)
│           └── RiscV/
│               └── CMakeLists.txt  # RISC-V integration tests (novo)
└── tests/
    └── riscv/
        └── CMakeLists.txt      # Unit tests RISC-V (modificado)
```

## Modificações Realizadas

### 1. `/workspace/CMakeLists.txt` (Principal)

**Adições:**
- `find_package(Qt5 ... Test)` - Adicionado componente Test para testes Qt
- `option(ENABLE_RISCV ...)` - Opção para habilitar/desabilitar RISC-V
- `add_subdirectory(src/riscv)` - Inclui módulo RISC-V
- `target_link_libraries(hidramachines PUBLIC hidrariscv)` - Link com RISC-V
- `enable_testing()` - Habilita framework de testes CTest
- `add_subdirectory(src/tests)` - Inclui testes existentes
- `add_subdirectory(tests/riscv)` - Inclui testes unitários RISC-V

**Por que:**
- Centraliza configuração do build
- Permite build condicional do módulo RISC-V
- Integra testes ao fluxo principal

### 2. `/workspace/src/riscv/CMakeLists.txt`

**Modificações:**
- Removido `option(ENABLE_RISCV ...)` (já definido no CMakeLists.txt principal)
- Alterado `Qt5::Widgets` para `Qt5::Core` (módulo não usa Widgets)
- Adicionado `set_target_properties(... AUTOMOC ON)` para suporte a Q_OBJECT
- Adicionados flags de warning (`-Wall -Wextra -Wpedantic`)
- Definido `RISCV_ENABLED` como definição pública

**Por que:**
- Evita duplicação de opção
- Usa dependência correta (Core, não Widgets)
- Melhora qualidade de código com warnings

### 3. `/workspace/tests/riscv/CMakeLists.txt`

**Modificações:**
- Removido `project(RiscVTests ...)` (usa contexto principal)
- Alterado `include_directories()` para `target_include_directories()` (mais moderno)
- Adicionado `target_compile_features(... cxx_std_17)`
- Adicionados flags de warning
- Melhorado paths de include usando variáveis CMake padrão

**Por que:**
- Evita conflito de projetos CMake aninhados
- Usa práticas modernas do CMake
- Garante consistência de padrão C++

### 4. `/workspace/src/tests/CMakeLists.txt`

**Adições:**
- Bloco condicional `if(ENABLE_RISCV)` para futuros testes de integração

**Por que:**
- Prepara infraestrutura para testes adicionais
- Mantém compatibilidade com builds sem RISC-V

### 5. `/workspace/src/tests/simulationtests/CMakeLists.txt`

**Adições:**
- `add_subdirectory(RiscV)` condicional

**Por que:**
- Integra testes RISC-V ao framework existente
- Mantém organização consistente com outras máquinas

### 6. `/workspace/src/tests/simulationtests/RiscV/` (Novo)

**Arquivos criados:**
- `CMakeLists.txt` - Configuração de build dos testes de integração
- `tst_riscv.cpp` - Testes exemplo usando SimulationTestCase

**Por que:**
- Valida integração com framework legado
- Fornece exemplos de uso da API

## Como Compilar

### Build Padrão (com RISC-V)

```bash
cd /workspace
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

### Build Sem RISC-V

```bash
cd /workspace
mkdir -p build && cd build
cmake .. -DENABLE_RISCV=OFF
make -j$(nproc)
```

### Build Debug com Verbose

```bash
cd /workspace
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DRISCV_TESTS_VERBOSE=ON
make -j$(nproc)
```

## Como Rodar Testes

### Todos os Testes

```bash
cd build
ctest
```

### Apenas Testes RISC-V

```bash
cd build
ctest -R RiscV --verbose
```

### Testes Específicos

```bash
# Register File
ctest -R RiscVRegisterFileTests --verbose

# Decoder
ctest -R RiscVDecoderTests --verbose

# ALU
ctest -R RiscVALUTests --verbose

# CPU completa
ctest -R RiscVCPUTests --verbose

# Integração (framework legado)
ctest -R TestRiscV --verbose
```

## Targets Gerados

| Target | Tipo | Descrição |
|--------|------|-----------|
| `hidracore` | Library | Core library do Hidra |
| `hidramachines` | Library | Máquinas existentes + link com RISC-V |
| `hidrariscv` | Library | **Novo** - Núcleo RISC-V |
| `hidratui` | Library | Interface TUI |
| `hidra-tui` | Executable | Executável principal |
| `riscv_tests` | Executable | **Novo** - Testes unitários RISC-V |
| `TestRiscV` | Executable | **Novo** - Testes de integração |

## Dependências

O módulo RISC-V depende de:
- Qt5 Core (obrigatório)
- Qt5 Test (para testes)
- hidracore (biblioteca core do Hidra)

## Solução de Problemas

### Erro: "Qt5::Test not found"

**Solução:** Instalar pacote de teste do Qt5
```bash
# Ubuntu/Debian
sudo apt-get install qtbase5-dev qttools5-dev

# Fedora
sudo dnf install qt5-qtbase-devel qt5-qttools-devel
```

### Erro: "hidrariscv not found"

**Causa:** ENABLE_RISCV está OFF

**Solução:**
```bash
cmake .. -DENABLE_RISCV=ON
```

### Erro: MOC não gerado para headers Qt

**Causa:** AUTOMOC não habilitado

**Solução:** Verificar se `set_target_properties(hidrariscv PROPERTIES AUTOMOC ON)` está presente

### Warnings de compilação

**Causa:** Flags `-Wall -Wextra -Wpedantic` ativos

**Solução:** Corrigir warnings ou desabilitar (não recomendado):
```cmake
target_compile_options(hidrariscv PRIVATE -w)
```

## Próximos Passos Sugeridos

1. **Validar build:** `cmake .. && make`
2. **Rodar testes:** `ctest -R RiscV --verbose`
3. **Integrar UI:** Adicionar RiscVMachine ao selector de máquinas na GUI
4. **Documentar API:** Gerar Doxygen para namespace `riscv`
5. **CI/CD:** Adicionar testes RISC-V ao pipeline de integração contínua

## Notas Importantes

- O CMakeLists.txt em `/workspace/src/CMakeLists.txt` é do build GUI (Qt Widgets) e **não foi modificado** para evitar quebrar build existente
- O build principal agora é via `/workspace/CMakeLists.txt` (TUI + RISC-V)
- Tests podem ser executados independentemente do executável principal
- A opção `ENABLE_RISCV` permite manter compatibilidade com sistemas sem suporte completo

## Referências

- [CMake Documentation](https://cmake.org/cmake/help/latest/)
- [Qt Testing Framework](https://doc.qt.io/qt-5/qtest.html)
- [RISC-V Specification](https://riscv.org/specifications/)
