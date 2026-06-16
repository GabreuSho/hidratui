# Hidra TUI

Simulador de máquinas teóricas para arquitetura de computadores com interface de terminal (TUI).

## Máquinas Suportadas

| Máquina      | Descrição                      | ISA            |
|--------------|--------------------------------|----------------|
| `neander`    | Acumulador 8-bit               | Neander        |
| `ahmes`      | Acumulador 8-bit               | Ahmes          |
| `ramses`     | Acumulador 8-bit               | Ramses         |
| `reg`        | Máquina de registradores 8-bit  | REG            |
| `volta`      | Máquina de pilha 8-bit          | Volta          |
| `pericles`   | 8-bit hipotética                | Pericles       |
| `pitagoras`  | 8-bit hipotética                | Pitagoras      |
| `cromag`     | 8-bit hipotética                | Cromag         |
| `queops`     | 8-bit hipotética                | Queops         |
| `rv32im`     | RISC-V 32-bit (I + M)          | RV32IM         |

Baseado no livro *Fundamentos de Arquitetura de Computadores* — Prof. Raul Fernando Weber (UFRGS).

## Construção

```bash
# Pré-requisitos
sudo apt install cmake g++ libftxui-dev qtbase5-dev

# Compilar
mkdir build && cd build
cmake ..
make -j$(nproc)

# Executar
./hidra-tui <máquina> <arquivo.asm>
```

## Uso

```bash
# Exemplo com RV32IM
./hidra-tui rv32im programa.s

# Exemplo com Ramses
./hidra-tui ramses programa.rad
```

## Controles

| Tecla | Ação                    |
|-------|-------------------------|
| `s`   | Step (executa 1 ciclo)  |
| `r`   | Run (execução contínua)  |
| `p`   | Pause                    |
| `c`   | Reset (clear)           |
| `/`   | Popup de navegação (Goto)|
| `q`   | Quit                     |

## Popup de Navegação (`/`)

Permite ir diretamente para:
- **Labels**: digite o nome para autocomplete (ex: `loop`, `entry`)
- **Endereços**: hex (`0x400000`) ou decimal
- **Quick jumps**: `.text`, `.data`, `.pc`, `.sp`, `.gp`, `.ra`

| Tecla | Ação                    |
|-------|-------------------------|
| `Enter` | Ir para endereço       |
| `Tab`  | Próxima sugestão        |
| `↑/↓`  | Navegar sugestões      |
| `Esc`  | Cancelar               |

## RV32IM (RISC-V)

Compatível com sintaxe RARS/MARS:
```asm
# Exemplo: soma de dois números
.eqv VALOR_A 5
.eqv VALOR_B 3

.text
_start:
    li t0, VALOR_A    # Carrega constante (t0 = 5)
    li t1, VALOR_B    # Carrega constante (t1 = 3)
    add t2, t0, t1     # Soma (t2 = 8)
    ebreak             # Encerra execução

.data
msg: .asciz "Resultado: "
```

### Suporte
- Instruções I (integer) + M (multiplication/division)
- Labels em `.text` e `.data`
- Diretivas: `.text`, `.data`, `.word`, `.byte`, `.half`, `.asciz`, `.space`, `.align`, `.eqv`
- Separadores: vírgulas e espaços (`add t0 t1 t2` ou `add t0, t1, t2`)
- Pseudo-instruções: `li`, `la`, `mv`, `nop`, `call`, `ret`, etc.

### .eqv (Equates)
Define constantes para reutilização:
```asm
.eqv BUFFER_SIZE 256
.eqv UART_BASE 0x40001000
```

## Estrutura do Projeto

```
src/
├── core/           # Máquina base, registradores, memória
├── machines/       # Implementações: neander, rv32im, etc.
├── tui/            # Interface de terminal (FTXUI)
│   └── panels/     # Painéis: memória, registradores, código, popup
└── main_tui.cpp    # Entry point
```

## Testes

```bash
cd build
ctest --output-on-failure

# Ou diretamente:
./src/tests/rv32imtest/test_rv32im
```

## Créditos

Originalmente desenvolvido pelo [PET Computação UFRGS](https://github.com/petcomputacaoufrgs).
Extensão TUI e suporte RV32IM adicionados.