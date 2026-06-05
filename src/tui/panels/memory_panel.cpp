#include "memory_panel.h"

#include <ftxui/dom/elements.hpp>
#include <iomanip>
#include <sstream>

#include "../layout.h"

using namespace ftxui;

namespace hidra::tui::panels {

MemoryPanel::MemoryPanel(const MemoryPanelConfig& config)
    : config_(config) {}

bool MemoryPanel::is32BitMachine() const {
  if (!machine_) return false;
  return machine_->getMemorySize() >= 65536;
}

Element MemoryPanel::render() {
  if (!machine_) return text("[sem máquina]") | border;

  int mem_size = machine_->getMemorySize();
  int pc = machine_->getPCValue();
  bool large = is32BitMachine();

  int base = config_.follow_pc ? std::max(0, pc - config_.visible_rows / 2) : base_address_;
  if (large) {
    // Para máquinas 32-bit, o espaço de endereçamento é enorme — não limitar ao mem_size
    base = std::max(0, base);
  } else {
    base = std::max(0, std::min(base, mem_size - config_.visible_rows));
  }

  int addr_width = large ? 10 : 4;
  int instr_width = large ? 20 : 10;

  Elements rows;
  rows.push_back(
      hbox(Elements{text("END") | bold | size(WIDTH, EQUAL, addr_width),
                    text(" ") | dim,
                    text("VAL") | bold | size(WIDTH, EQUAL, 4),
                    text(" ") | dim,
                    text("INSTRUÇÃO") | bold | size(WIDTH, EQUAL, instr_width),
                    text("LABEL") | bold | color(Color::Cyan) | flex}) |
      color(Color::Cyan));
  rows.push_back(separator());

  for (int i = 0; i < config_.visible_rows; ++i) {
    int addr = base + i;
    if (!large && addr >= mem_size) break;
    bool is_pc = (addr == pc);
    rows.push_back(render_row(addr, is_pc));
  }

  if (large || base > 0 || base + config_.visible_rows < mem_size) {
    rows.push_back(separator());
    std::string info;
    if (large) {
      std::ostringstream info_ss;
      info_ss << "End: 0x" << std::hex << std::setw(8) << std::setfill('0')
              << (unsigned int)base << "-0x" << std::setw(8) << std::setfill('0')
              << (unsigned int)(base + config_.visible_rows - 1);
      info = info_ss.str();
    } else {
      info = "End: " + std::to_string(base) + "-" +
             std::to_string(base + config_.visible_rows - 1) +
             " / " + std::to_string(mem_size);
    }
    rows.push_back(text(info) | dim | align_right);
  }

  auto content = vbox(std::move(rows));
  return vbox(Elements{text(" MEMÓRIA ") | bold, separator(), content | flex}) | border;
}

Element MemoryPanel::render_row(int address, bool is_pc) {
  int value = machine_->getMemoryValue(address);
  std::string instr_text = get_instruction_text(address);
  std::string label_text = get_label_text(address);

  bool large = is32BitMachine();
  std::ostringstream addr_ss, val_ss;

  if (large) {
    addr_ss << "0x" << std::hex << std::setw(8) << std::setfill('0') << (unsigned int)address;
    val_ss << std::hex << std::setw(2) << std::setfill('0') << value;
  } else {
    addr_ss << std::setw(3) << std::setfill(' ') << address;
    val_ss << std::setw(3) << std::setfill(' ') << value;
  }

  auto addr_elem = text(addr_ss.str()) | color(Color::Green);
  auto val_elem = text(val_ss.str()) | color(Color::Yellow);

  Elements row_elems;
  int addr_width = large ? 10 : 4;
  int instr_width = large ? 20 : 10;

  row_elems.push_back(addr_elem | size(WIDTH, EQUAL, addr_width));
  row_elems.push_back(text(" ") | dim);
  row_elems.push_back(val_elem | size(WIDTH, EQUAL, 4));
  row_elems.push_back(text(" ") | dim);

  // Coluna Instrução (vazia se for operando/dado)
  if (!instr_text.empty()) {
    row_elems.push_back(text(instr_text) | color(Color::White) | size(WIDTH, EQUAL, instr_width));
  } else {
    row_elems.push_back(text("") | size(WIDTH, EQUAL, instr_width + 6));
  }

  // Coluna Label (à direita, flexível)
  if (!label_text.empty()) {
    row_elems.push_back(text(label_text) | color(Color::Cyan) | flex);
  } else {
    row_elems.push_back(text("") | flex);
  }

  auto row = hbox(row_elems);
  if (is_pc) {
    row = row | bgcolor(Color::Blue) | color(Color::White) | bold;
  }
  return row;
}

// ✅ Apenas consulta o desmontador oficial do Hidra. Se vazio, é dado/operando.
std::string MemoryPanel::get_instruction_text(int address) {
  if (!machine_) return "";
  try {
    QString instr = machine_->getInstructionString(address);
    if (!instr.isEmpty()) {
      return instr.toStdString();
    }
  } catch (...) {
  }
  return "";
}

std::string MemoryPanel::get_label_text(int address) const {
  if (!machine_) return "";
  try {
    QString label = machine_->getAddressCorrespondingLabel(address);
    if (!label.isEmpty()) return label.toStdString();
  } catch (...) {
  }
  return "";
}

void MemoryPanel::scroll_up(int lines) {
  if (is32BitMachine()) {
    // Para máquinas 32-bit, rola por palavras (4 bytes) alinhadas
    base_address_ = std::max(0, base_address_ - lines * 4);
  } else {
    base_address_ = std::max(0, base_address_ - lines);
  }
}

void MemoryPanel::scroll_down(int lines) {
  if (is32BitMachine()) {
    // Para máquinas 32-bit, rola por palavras (4 bytes) alinhadas sem limitar ao mem_size
    base_address_ = base_address_ + lines * 4;
  } else {
    if (machine_)
      base_address_ = std::min(machine_->getMemorySize() - config_.visible_rows, base_address_ + lines);
  }
}

void MemoryPanel::center_on_pc() {
  if (!machine_) return;
  if (is32BitMachine()) {
    // Alinha o PC ao limite de palavra (múltiplo de 4)
    int pc = machine_->getPCValue();
    base_address_ = std::max(0, (pc & ~3) - (config_.visible_rows / 2) * 4);
  } else {
    base_address_ = std::max(0, machine_->getPCValue() - config_.visible_rows / 2);
  }
}

}  // namespace hidra::tui::panels
