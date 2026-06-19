#include "help_popup.h"

#include <algorithm>

using namespace ftxui;

namespace {

constexpr int VISIBLE_LINES = 18;
constexpr int COMPACT_SECTION_HEIGHT = 1;
const std::string BAR_SELECTED = "█";
const std::string BAR_AVAILABLE = "░";

std::vector<hidra::tui::panels::HelpSection> build_keybindings_section() {
    return {
        {"ATALHOS DO PROGRAMA", {
            {"?", "Abrir/Fechar esta ajuda"},
            {"b", "Montar código (Build)"},
            {"s", "Executar um passo (Step)"},
            {"r", "Executar/Parar (Run/Stop)"},
            {"5r", "Executar 5 passos (n+r)"},
            {"p", "Resetar PC para início"},
            {"x", "Resetar tudo (clear)"},
            {"q", "Sair do programa"},
            {"/", "Navegar para endereço/label"},
            {"g", "Ir para endereço digitado"},
            {"G", "Ir para endereço ou final da memória"},
            {"h", "Alternar Hexadecimal/Decimal"},
            {"f", "Alternar seguir PC"},
            {"k/u/↑", "Rolar memória para cima"},
            {"j/d/↓", "Rolar memória para baixo"},
            {"Ctrl+↑/↓", "Rolar registradores"},
            {"+", "Aumentar velocidade de execução"},
            {"-", "Diminuir velocidade de execução"},
        }},
        {"NAVEGAÇÃO (/popup)", {
            {"Enter", "Ir para endereço/label selecionado"},
            {"↑/↓", "Navegar sugestões/labels"},
            {"Tab", "Próxima sugestão"},
            {"Backspace", "Apagar último caractere"},
            {"Esc", "Fechar popup"},
        }},
    };
}

static std::vector<hidra::tui::panels::HelpSection> build_neander_sections() {
    return {
        {"NEANDER (NDR) - 8 bits", {
            {"Registradores", "AC (Acumulador), PC (Contador de Programa)"},
            {"Flags", "N (Negativo), Z (Zero)"},
            {"Memória", "256 bytes (endereços 0-255)"},
        }},
        {"INSTRUÇÕES", {
            {"NOP", "Nenhuma operação"},
            {"STA a", "Armazena AC no endereço 'a'"},
            {"LDA a", "Carrega de 'a' para AC"},
            {"ADD a", "Soma AC com valor de 'a'"},
            {"OR a", "OU lógico de AC com 'a'"},
            {"AND a", "E lógico de AC com 'a'"},
            {"NOT", "Inverte bits de AC"},
            {"JMP a", "Desvia para endereço 'a'"},
            {"JN a", "Desvia se N=1 (negativo)"},
            {"JZ a", "Desvia se Z=1 (zero)"},
            {"HLT", "Termina execução"},
        }},
        {"ENDEREÇAMENTO", {
            {"Direto", "sta 10 - acesso direto ao endereço 10"},
        }},
    };
}

static std::vector<hidra::tui::panels::HelpSection> build_ahmes_sections() {
    return {
        {"AHMES (AHM) - 8 bits", {
            {"Registradores", "AC (Acumulador), PC (Contador de Programa)"},
            {"Flags", "N (Negativo), Z (Zero), V (Overflow), C (Carry), B (Borrow)"},
            {"Memória", "256 bytes (endereços 0-255)"},
        }},
        {"INSTRUÇÕES", {
            {"NOP", "Nenhuma operação"},
            {"STA a", "Armazena AC no endereço 'a'"},
            {"LDA a", "Carrega de 'a' para AC"},
            {"ADD a", "Soma AC com valor de 'a'"},
            {"OR a", "OU lógico de AC com 'a'"},
            {"AND a", "E lógico de AC com 'a'"},
            {"NOT", "Inverte bits de AC"},
            {"SUB a", "Subtrai AC por valor de 'a'"},
            {"JMP a", "Desvia para endereço 'a'"},
            {"JN a", "Desvia se N=1 (negativo)"},
            {"JP a", "Desvia se N=0 (positivo)"},
            {"JV a", "Desvia se V=1 (overflow)"},
            {"JNV a", "Desvia se V=0 (sem overflow)"},
            {"JZ a", "Desvia se Z=1 (zero)"},
            {"JNZ a", "Desvia se Z=0 (não zero)"},
            {"JC a", "Desvia se C=1 (carry)"},
            {"JNC a", "Desvia se C=0 (sem carry)"},
            {"JB a", "Desvia se B=1 (borrow)"},
            {"JNB a", "Desvia se B=0 (sem borrow)"},
            {"SHR", "Shift right (desloca bits para direita)"},
            {"SHL", "Shift left (desloca bits para esquerda)"},
            {"ROR", "Rotate right (rotaciona bits para direita)"},
            {"ROL", "Rotate left (rotaciona bits para esquerda)"},
            {"HLT", "Termina execução"},
        }},
        {"ENDEREÇAMENTO", {
            {"Direto", "sta 10 - acesso direto ao endereço 10"},
        }},
    };
}

static std::vector<hidra::tui::panels::HelpSection> build_ramses_sections() {
    return {
        {"RAMSES (RMS) - 8 bits", {
            {"Registradores", "A, B, X (3 registradores), PC (Contador de Programa)"},
            {"Flags", "N (Negativo), Z (Zero), C (Carry)"},
            {"Memória", "256 bytes (endereços 0-255)"},
        }},
        {"INSTRUÇÕES", {
            {"NOP", "Nenhuma operação"},
            {"STR r a", "Armazena registrador r no endereço 'a'"},
            {"LDR r a", "Carrega de 'a' para registrador r"},
            {"ADD r a", "Soma registrador r com valor de 'a'"},
            {"OR r a", "OU lógico de r com 'a'"},
            {"AND r a", "E lógico de r com 'a'"},
            {"NOT r", "Inverte bits de registrador r"},
            {"SUB r a", "Subtrai registrador r por valor de 'a'"},
            {"JMP a", "Desvia para endereço 'a'"},
            {"JN a", "Desvia se N=1 (negativo)"},
            {"JZ a", "Desvia se Z=1 (zero)"},
            {"JC a", "Desvia se C=1 (carry)"},
            {"JSR a", "Salta para sub-rotina em 'a'"},
            {"NEG r", "Nega registrador r (complemento de 2)"},
            {"SHR r", "Shift right do registrador r"},
            {"HLT", "Termina execução"},
        }},
        {"MODOS DE ENDEREÇAMENTO", {
            {"Direto", "ldr a 10 - acessa endereço 10"},
            {"Indireto", "ldr a 10,i - usa valor em 10 como endereço"},
            {"Imediato", "ldr a #10 - usa 10 como valor imediato"},
            {"Indexado por X", "ldr a 10,x - endereço 10 + valor de X"},
        }},
    };
}

static std::vector<hidra::tui::panels::HelpSection> build_reg_sections() {
    return {
        {"REG (REG) - 8 bits", {
            {"Registradores", "R0-R63 (64 registradores genéricos), PC"},
            {"Memória", "256 bytes (endereços 0-255)"},
        }},
        {"INSTRUÇÕES", {
            {"INC r", "Incrementa registrador r"},
            {"DEC r", "Decrementa registrador r"},
            {"IF r a0 a1", "Se r≠0, desvia para a0; senão, para a1"},
            {"HLT", "Termina execução"},
        }},
    };
}

static std::vector<hidra::tui::panels::HelpSection> build_volta_sections() {
    return {
        {"VOLTA (VLT) - 8 bits (baseado em pilha)", {
            {"Registradores", "PC (Contador de Programa), SP (Stack Pointer)"},
            {"Memória", "256 bytes + stack de 64 posições"},
        }},
        {"ARITMÉTICA/LÓGICA (2 operandos)", {
            {"ADD", "Desempilha A e B, empilha B + A"},
            {"SUB", "Desempilha A e B, empilha B - A"},
            {"AND", "E lógico: desempilha A e B, empilha B & A"},
            {"OR", "OU lógico: desempilha A e B, empilha B | A"},
        }},
        {"ARITMÉTICA/LÓGICA (1 operando)", {
            {"CLR", "Zera o topo da pilha"},
            {"NOT", "Inverte bits do topo da pilha"},
            {"NEG", "Nega (complemento de 2) o topo da pilha"},
            {"INC", "Incrementa o topo da pilha"},
            {"DEC", "Decrementa o topo da pilha"},
            {"ASR", "Shift aritmético para direita"},
            {"ASL", "Shift aritmético para esquerda"},
            {"ROR", "Rotação para direita"},
            {"ROL", "Rotação para esquerda"},
        }},
        {"CONDIÇAIS (1 operando)", {
            {"SZ", "Skip if Zero (pula se topo=0)"},
            {"SNZ", "Skip if Not Zero (pula se topo≠0)"},
            {"SPL", "Skip if PLus (pula se topo>0)"},
            {"SMI", "Skip if MInus (pula se topo<0)"},
            {"SPZ", "Skip if Plus/Zero (pula se topo≥0)"},
            {"SMZ", "Skip if Minus/Zero (pula se topo≤0)"},
        }},
        {"CONDIÇAIS (2 operandos)", {
            {"SEQ", "Skip if EQual (pula se B=A)"},
            {"SNE", "Skip if Not Equal (pula se B≠A)"},
            {"SGR", "Skip if GReater (pula se B>A)"},
            {"SLS", "Skip if LeSS (pula se B<A)"},
            {"SGE", "Skip if Greater/Equal (pula se B≥A)"},
            {"SLE", "Skip if Less/Equal (pula se B≤A)"},
        }},
        {"CONTROLE DE FLUXO", {
            {"RTS", "Retorno de sub-rotina (desvia para topo da pilha)"},
            {"PSH a", "Empilha valor do endereço 'a'"},
            {"POP a", "Desempilha para endereço 'a'"},
            {"JMP a", "Desvia para endereço 'a'"},
            {"JSR a", "Chama sub-rotina em 'a'"},
            {"HLT", "Termina execução"},
        }},
    };
}

static std::vector<hidra::tui::panels::HelpSection> build_pitagoras_sections() {
    return {
        {"PITAGORAS (PTG) - 8 bits", {
            {"Registradores", "A (Acumulador), PC (Contador de Programa)"},
            {"Flags", "N (Negativo), Z (Zero), C (Carry)"},
            {"Memória", "256 bytes (endereços 0-255)"},
        }},
        {"INSTRUÇÕES", {
            {"NOP", "Nenhuma operação"},
            {"STA a", "Armazena A no endereço 'a'"},
            {"LDA a", "Carrega de 'a' para A"},
            {"ADD a", "Soma A com valor de 'a'"},
            {"OR a", "OU lógico de A com 'a'"},
            {"AND a", "E lógico de A com 'a'"},
            {"NOT", "Inverte bits de A"},
            {"SUB a", "Subtrai A por valor de 'a'"},
            {"JMP a", "Desvia para endereço 'a'"},
            {"JN a", "Desvia se N=1 (negativo)"},
            {"JP a", "Desvia se N=0 (positivo)"},
            {"JZ a", "Desvia se Z=1 (zero)"},
            {"JD a", "Desvia se Z=0 (diferente de zero)"},
            {"JC a", "Desvia se C=1 (carry)"},
            {"JB a", "Desvia se C=0 (não borrow)"},
            {"SHR", "Shift right (desloca bits para direita)"},
            {"SHL", "Shift left (desloca bits para esquerda)"},
            {"ROR", "Rotate right (rotaciona bits para direita)"},
            {"ROL", "Rotate left (rotaciona bits para esquerda)"},
            {"HLT", "Termina execução"},
        }},
    };
}

static std::vector<hidra::tui::panels::HelpSection> build_cromag_sections() {
    return {
        {"CRO MAG (CRM) - 8 bits", {
            {"Registradores", "A (Acumulador), PC (Contador de Programa)"},
            {"Flags", "N (Negativo), Z (Zero), C (Carry)"},
            {"Memória", "256 bytes (endereços 0-255)"},
        }},
        {"INSTRUÇÕES", {
            {"NOP", "Nenhuma operação"},
            {"STR a", "Armazena A no endereço 'a'"},
            {"LDR a", "Carrega de 'a' para A"},
            {"ADD a", "Soma A com valor de 'a'"},
            {"OR a", "OU lógico de A com 'a'"},
            {"AND a", "E lógico de A com 'a'"},
            {"NOT", "Inverte bits de A"},
            {"JMP a", "Desvia para endereço 'a'"},
            {"JN a", "Desvia se N=1 (negativo)"},
            {"JZ a", "Desvia se Z=1 (zero)"},
            {"JC a", "Desvia se C=1 (carry)"},
            {"SHR", "Shift right (desloca bits para direita)"},
            {"HLT", "Termina execução"},
        }},
        {"ENDEREÇAMENTO", {
            {"Direto", "str 10 - acesso direto ao endereço 10"},
            {"Indireto", "str 10,i - usa valor em 10 como endereço"},
        }},
    };
}

static std::vector<hidra::tui::panels::HelpSection> build_pericles_sections() {
    return {
        {"PERICLES (PRC) - 12 bits", {
            {"Registradores", "A, B, X (3 registradores), PC (12 bits)"},
            {"Flags", "N (Negativo), Z (Zero), C (Carry)"},
            {"Memória", "4096 bytes (endereços 0-4095)"},
        }},
        {"INSTRUÇÕES", {
            {"NOP", "Nenhuma operação"},
            {"STR r a", "Armazena registrador r no endereço 'a'"},
            {"LDR r a", "Carrega de 'a' para registrador r"},
            {"ADD r a", "Soma registrador r com valor de 'a'"},
            {"OR r a", "OU lógico de r com 'a'"},
            {"AND r a", "E lógico de r com 'a'"},
            {"NOT r", "Inverte bits de registrador r"},
            {"SUB r a", "Subtrai registrador r por valor de 'a'"},
            {"JMP a", "Desvia para endereço 'a'"},
            {"JN a", "Desvia se N=1 (negativo)"},
            {"JZ a", "Desvia se Z=1 (zero)"},
            {"JC a", "Desvia se C=1 (carry)"},
            {"JSR a", "Salta para sub-rotina em 'a'"},
            {"NEG r", "Nega registrador r (complemento de 2)"},
            {"SHR r", "Shift right do registrador r"},
            {"HLT", "Termina execução"},
        }},
        {"MODOS DE ENDEREÇAMENTO", {
            {"Direto", "ldr a 10 - acessa endereço 10"},
            {"Indireto", "ldr a 10,i - usa valor em 10 como endereço"},
            {"Imediato", "ldr a #10 - usa 10 como valor imediato"},
            {"Indexado por X", "ldr a 10,x - endereço 10 + valor de X"},
        }},
    };
}

static std::vector<hidra::tui::panels::HelpSection> build_queops_sections() {
    return {
        {"QUEOPS (QPS) - 8 bits", {
            {"Registradores", "A (Acumulador), PC (Contador de Programa)"},
            {"Flags", "N (Negativo), Z (Zero), C (Carry)"},
            {"Memória", "256 bytes (endereços 0-255)"},
        }},
        {"INSTRUÇÕES", {
            {"NOP", "Nenhuma operação"},
            {"STR a", "Armazena A no endereço 'a'"},
            {"LDR a", "Carrega de 'a' para A"},
            {"ADD a", "Soma A com valor de 'a'"},
            {"OR a", "OU lógico de A com 'a'"},
            {"AND a", "E lógico de A com 'a'"},
            {"NOT", "Inverte bits de A"},
            {"JMP a", "Desvia para endereço 'a'"},
            {"JN a", "Desvia se N=1 (negativo)"},
            {"JZ a", "Desvia se Z=1 (zero)"},
            {"JC a", "Desvia se C=1 (carry)"},
            {"HLT", "Termina execução"},
        }},
        {"MODOS DE ENDEREÇAMENTO", {
            {"Direto", "str 10 - acesso direto ao endereço 10"},
            {"Indireto", "str 10,i - usa valor em 10 como endereço"},
            {"Imediato", "str #10 - usa 10 como valor imediato"},
            {"Indexado por PC", "str 10,pc - usa PC + 10 como endereço"},
        }},
    };
}

static std::vector<hidra::tui::panels::HelpSection> build_rv32im_sections() {
    return {
        {"RISC-V RV32IM - 32 bits", {
            {"Registradores", "x0-x31 (32 registradores de 32 bits)"},
            {"x0/zero", "Sempre zero (hardwired)"},
            {"x1/ra", "Return address (sub-rotinas)"},
            {"x2/sp", "Stack Pointer (0x7FFFFFFC)"},
            {"x3/gp", "Global Pointer (0x10008000)"},
            {"x4/tp", "Thread Pointer"},
            {"x5-x7/t0-t2", "Temporários"},
            {"x8/s0/fp", "Saved register / Frame pointer"},
            {"x9/s1", "Saved register"},
            {"x10-x17/a0-a7", "Argumentos / Retorno"},
            {"x18-x27/s2-s11", "Saved registers"},
            {"x28-x31/t3-t6", "Temporários"},
            {"PC", "Program Counter (inicia em 0x40000000)"},
            {"Memória", "Esparsa (endereços 32-bit)"},
        }},
        {"DIRETIVAS DO MONTADOR", {
            {".text", "Seção de código"},
            {".data", "Seção de dados"},
            {".word val(s)", "Insere 32-bit: .word 0x1234 ou .word 1,2,3,4"},
            {".half val(s)", "Insere 16-bit: .half 0x1234 ou .half 1,2,3"},
            {".byte val(s)", "Insere 8-bit: .byte 0xFF ou .byte 1,2,3"},
            {".space N", "Reserva N bytes zerados"},
            {".asciz \"str\"", "Insere string com terminador nulo"},
            {".align N", "Alinha para 2^N bytes"},
            {".eqv NAME val", "Define constante: .eqv MY_CONST 42"},
        }},
        {"INSTRUÇÕES I-TYPE (Integer)", {
            {"ADD rd, rs1, rs2", "rd = rs1 + rs2"},
            {"SUB rd, rs1, rs2", "rd = rs1 - rs2"},
            {"XOR rd, rs1, rs2", "rd = rs1 ^ rs2"},
            {"OR rd, rs1, rs2", "rd = rs1 | rs2"},
            {"AND rd, rs1, rs2", "rd = rs1 & rs2"},
            {"SLL rd, rs1, rs2", "Shift left lógico"},
            {"SRL rd, rs1, rs2", "Shift right lógico"},
            {"SRA rd, rs1, rs2", "Shift right aritmético"},
            {"SLT rd, rs1, rs2", "rd = (rs1 < rs2) com sinal"},
            {"SLTU rd, rs1, rs2", "rd = (rs1 < rs2) sem sinal"},
            {"ADDI rd, rs1, imm", "rd = rs1 + imediato (12-bit)"},
            {"XORI rd, rs1, imm", "rd = rs1 ^ imediato"},
            {"ORI rd, rs1, imm", "rd = rs1 | imediato"},
            {"ANDI rd, rs1, imm", "rd = rs1 & imediato"},
            {"SLLI rd, rs1, imm", "Shift left por imediato (5 bits)"},
            {"SRLI rd, rs1, imm", "Shift right lógico por imediato"},
            {"SRAI rd, rs1, imm", "Shift right aritmético por imediato"},
            {"SLTI rd, rs1, imm", "rd = (rs1 < imm) com sinal"},
            {"SLTIU rd, rs1, imm", "rd = (rs1 < imm) sem sinal"},
        }},
        {"INSTRUÇÕES I-TYPE (Load)", {
            {"LW rd, offset(rs1)", "Carrega word: rd = mem[rs1+offset]"},
            {"LB rd, offset(rs1)", "Carrega byte com sinal"},
            {"LBU rd, offset(rs1)", "Carrega byte sem sinal (zero-extend)"},
            {"LHU rd, offset(rs1)", "Carrega halfword sem sinal"},
        }},
        {"INSTRUÇÕES U-TYPE", {
            {"LUI rd, imm", "Carrega imediato 20-bit nos 20 bits altos"},
            {"AUIPC rd, imm", "rd = PC + imediato 20-bit (shiftado)"},
        }},
        {"INSTRUÇÕES S-TYPE (Store)", {
            {"SW rs2, offset(rs1)", "Armazena word: mem[rs1+offset] = rs2"},
            {"SB rs2, offset(rs1)", "Armazena byte"},
            {"SH rs2, offset(rs1)", "Armazena halfword"},
        }},
        {"INSTRUÇÕES B-TYPE (Branch)", {
            {"BEQ rs1, rs2, label", "Branch se rs1 == rs2"},
            {"BNE rs1, rs2, label", "Branch se rs1 != rs2"},
            {"BLT rs1, rs2, label", "Branch se rs1 < rs2 (com sinal)"},
            {"BGE rs1, rs2, label", "Branch se rs1 >= rs2 (com sinal)"},
            {"BLTU rs1, rs2, label", "Branch se rs1 < rs2 (sem sinal)"},
            {"BGEU rs1, rs2, label", "Branch se rs1 >= rs2 (sem sinal)"},
        }},
        {"INSTRUÇÕES J-TYPE", {
            {"JAL rd, label", "Jump and Link: rd=PC+4, PC=label"},
            {"JALR rd, rs1, imm", "Jump and Link Register"},
        }},
        {"INSTRUÇÕES M (Multiply/Divide)", {
            {"MUL rd, rs1, rs2", "rd = (rs1 * rs2)[31:0]"},
            {"MULH rd, rs1, rs2", "rd = high 32-bit do produto (com sinal)"},
            {"MULHSU rd, rs1, rs2", "rd = high 32-bit (rs1 com sinal, rs2 sem)"},
            {"MULHU rd, rs1, rs2", "rd = high 32-bit (ambos sem sinal)"},
            {"DIV rd, rs1, rs2", "rd = rs1 / rs2 (com sinal)"},
            {"DIVU rd, rs1, rs2", "rd = rs1 / rs2 (sem sinal)"},
            {"REM rd, rs1, rs2", "rd = rs1 % rs2 (com sinal)"},
            {"REMU rd, rs1, rs2", "rd = rs1 % rs2 (sem sinal)"},
        }},
        {"PSEUDO-INSTRUÇÕES", {
            {"LI rd, imm", "Load Immediate: li a0, 42"},
            {"LA rd, label", "Load Address: la a0, my_var"},
            {"MV rd, rs", "Move: mv a0, t0 (addi rd, rs, 0)"},
            {"NOP", "No operation (addi x0, x0, 0)"},
            {"CALL label", "Chama sub-rotina"},
            {"RET", "Retorna de sub-rotina (jalr x0, ra, 0)"},
        }},
    };
}

static std::vector<hidra::tui::panels::HelpSection> build_generic_sections() {
    std::vector<hidra::tui::panels::HelpSection> result = build_keybindings_section();

    std::vector<hidra::tui::panels::HelpSection> rv32im = build_rv32im_sections();
    result.insert(result.end(), rv32im.begin(), rv32im.end());

    return result;
}

int get_section_display_height(const hidra::tui::panels::HelpSection& section, bool expanded) {
    if (expanded) {
        return 1 + static_cast<int>(section.items.size());
    }
    return COMPACT_SECTION_HEIGHT;
}

}  // anonymous namespace

namespace hidra::tui::panels {

HelpPopup::HelpPopup() {
    machine_type_ = "generic";
    build_sections();
}

void HelpPopup::set_machine_type(const std::string& machine_type) {
    machine_type_ = machine_type;
    scroll_offset_ = 0;
    selected_section_ = 0;
    build_sections();
}

void HelpPopup::open() {
    active_ = true;
    scroll_offset_ = 0;
    selected_section_ = 0;
}

void HelpPopup::close() {
    active_ = false;
}

void HelpPopup::build_sections() {
    sections_.clear();

    if (machine_type_ == "neander") {
        sections_ = ::build_neander_sections();
    } else if (machine_type_ == "ahmes") {
        sections_ = ::build_ahmes_sections();
    } else if (machine_type_ == "ramses") {
        sections_ = ::build_ramses_sections();
    } else if (machine_type_ == "reg") {
        sections_ = ::build_reg_sections();
    } else if (machine_type_ == "volta") {
        sections_ = ::build_volta_sections();
    } else if (machine_type_ == "pitagoras") {
        sections_ = ::build_pitagoras_sections();
    } else if (machine_type_ == "cromag") {
        sections_ = ::build_cromag_sections();
    } else if (machine_type_ == "pericles") {
        sections_ = ::build_pericles_sections();
    } else if (machine_type_ == "queops") {
        sections_ = ::build_queops_sections();
    } else if (machine_type_ == "rv32im") {
        sections_ = ::build_rv32im_sections();
    } else {
        sections_ = ::build_generic_sections();
    }
}

void HelpPopup::handle_key(const ftxui::Event& event) {
    if (event == Event::Escape || event == Event::Character('?')) {
        close();
        return;
    }

    if (event == Event::ArrowUp) {
        navigate_section(-1);
        return;
    }

    if (event == Event::ArrowDown) {
        navigate_section(1);
        return;
    }

    if (event == Event::PageUp) {
        navigate_section(-5);
        return;
    }

    if (event == Event::PageDown) {
        navigate_section(5);
        return;
    }
}

void HelpPopup::navigate_section(int delta) {
    if (sections_.empty()) return;

    int new_selected = selected_section_ + delta;

    if (new_selected < 0) {
        new_selected = 0;
    } else if (new_selected >= static_cast<int>(sections_.size())) {
        new_selected = sections_.size() - 1;
    }

    selected_section_ = new_selected;

    ensure_selection_visible();
}

void HelpPopup::ensure_selection_visible() {
    int current_line = 0;

    for (int i = 0; i < selected_section_; ++i) {
        current_line += get_section_display_height(sections_[i], false);
    }

    bool selected_expanded = true;
    int selected_height = get_section_display_height(sections_[selected_section_], selected_expanded);

    int content_height = current_line + selected_height;

    if (current_line < scroll_offset_) {
        scroll_offset_ = current_line;
    } else if (content_height > scroll_offset_ + VISIBLE_LINES) {
        scroll_offset_ = content_height - VISIBLE_LINES;
    }
}

Element HelpPopup::render() const {
    if (sections_.empty()) {
        return vbox({
            text(" Pressione ? ou ESC para fechar ") | bold | color(Color::Cyan),
            text(" Nenhum conteúdo de ajuda disponível ") | dim,
        }) | border;
    }

    Elements content;

    content.push_back(
        text(" AJUDA - ? ou ESC fecha ") | bold | color(Color::Cyan) | inverted
    );

    int current_line = 0;
    int display_line = 0;
    Elements visible_content;
    std::vector<std::string> bar_markers;

    for (int i = 0; i < static_cast<int>(sections_.size()) && display_line < VISIBLE_LINES; ++i) {
        bool is_selected = (i == selected_section_);

        int section_height = get_section_display_height(sections_[i], is_selected);

        bool starts_in_view = (current_line >= scroll_offset_);
        bool ends_in_view = (current_line + section_height <= scroll_offset_ + VISIBLE_LINES);

        bar_markers.push_back(is_selected ? BAR_SELECTED : BAR_AVAILABLE);

        if (!starts_in_view) {
            current_line += section_height;
            continue;
        }

        if (!ends_in_view && current_line + section_height > scroll_offset_ + VISIBLE_LINES) {
            int visible_lines = scroll_offset_ + VISIBLE_LINES - current_line;
            visible_content.push_back(render_section_compact(sections_[i], is_selected));
            display_line++;
            current_line += section_height;

            if (display_line >= VISIBLE_LINES) break;
            continue;
        }

        visible_content.push_back(render_section(sections_[i], is_selected));
        display_line++;
        current_line += section_height;
    }

    while (display_line < VISIBLE_LINES && bar_markers.size() < static_cast<size_t>(sections_.size())) {
        bar_markers.push_back(BAR_AVAILABLE);
        display_line++;
    }

    Element bar_element = text("");
    if (!bar_markers.empty()) {
        std::string bar_str;
        for (const auto& marker : bar_markers) {
            bar_str += marker + "\n";
        }
        bar_element = text(bar_str) | color(Color::GrayDark);
    }

    Element main_content = vbox(visible_content);

    content.push_back(hbox({
        main_content | flex,
        bar_element,
    }) | flex);

    std::string pos_str = std::to_string(selected_section_ + 1) + "/" + std::to_string(sections_.size());
    content.push_back(text(" [↑↓] Navegar  [PgUp/PgDn] Page  [?] ou [Esc] Fechar  " + pos_str) | dim);

    return vbox(content) | border | bold | color(Color::Cyan);
}

Element HelpPopup::render_section(const HelpSection& section, bool selected) const {
    Elements lines;

    if (selected) {
        lines.push_back(text("▶ " + section.title) | bold | color(Color::Yellow));
    } else {
        lines.push_back(text("  " + section.title) | bold | color(Color::Cyan));
    }

    if (selected) {
        for (const auto& [key, desc] : section.items) {
            std::string line = "   [";
            line += key;
            line += "] ";
            line += desc;
            lines.push_back(text(line) | color(Color::White));
        }
    }

    return vbox(lines);
}

Element HelpPopup::render_section_compact(const HelpSection& section, bool selected) const {
    if (selected) {
        return text("▶ " + section.title) | bold | color(Color::Yellow);
    } else {
        return text("  " + section.title) | bold | color(Color::Cyan);
    }
}

}  // namespace hidra::tui::panels