#include "memory_panel.h"

#include <ftxui/dom/elements.hpp>
#include <iomanip>
#include <sstream>

#include "../layout.h"

using namespace ftxui;

namespace hidra::tui::panels {

MemoryPanel::MemoryPanel(const MemoryPanelConfig& config) : config_(config) {}

Element MemoryPanel::render() {
  if (!machine_) return text("[sem máquina]") | border;

  int mem_size = machine_->getMemorySize();
  int pc = machine_->getPCValue();

  int base = config_.follow_pc ? std::max(0, pc - config_.visible_rows / 2)
                               : base_address_;
  base = std::max(0, std::min(base, mem_size - config_.visible_rows));

  Elements rows;

  rows.push_back(
      hbox(Elements{text("END") | bold | size(WIDTH, EQUAL, 4), text(" ") | dim,
                    text("VAL") | bold | size(WIDTH, EQUAL, 4),
                    text("  ") | dim,
                    text("INSTRUÇÃO") | bold | size(WIDTH, EQUAL, 10),
                    text("LABEL") | bold | color(Color::Cyan) | flex}) |
      color(Color::Cyan));

  rows.push_back(separator());

  for (int i = 0; i < config_.visible_rows && base + i < mem_size; ++i) {
    int addr = base + i;
    bool is_pc = (addr == pc);
    rows.push_back(render_row(addr, is_pc));
  }

  if (base > 0 || base + config_.visible_rows < mem_size) {
    rows.push_back(separator());
    std::string info = "End: " + std::to_string(base) + "-" +
                       std::to_string(base + config_.visible_rows - 1) + " / " +
                       std::to_string(mem_size);
    rows.push_back(text(info) | dim | align_right);
  }

  auto content = vbox(std::move(rows));
  return vbox(Elements{text(" MEMÓRIA ") | bold, separator(), content | flex}) |
         border;
}

Element MemoryPanel::render_row(int address, bool is_pc) {
  int value = machine_->getMemoryValue(address);
  std::string instr_text = get_instruction_text(address);
  std::string label_text = get_label_text(address);

  std::ostringstream addr_ss, val_ss;
  addr_ss << std::setw(3) << std::setfill(' ') << address;
  val_ss << std::setw(3) << std::setfill(' ') << value;

  auto addr_elem = text(addr_ss.str()) | color(Color::Green);
  auto val_elem = text(val_ss.str()) | color(Color::Yellow);

  Elements row_elems;
  row_elems.push_back(addr_elem | size(WIDTH, EQUAL, 4));
  row_elems.push_back(text(" ") | dim);
  row_elems.push_back(val_elem | size(WIDTH, EQUAL, 4));
  row_elems.push_back(text("  ") | dim);

  // Coluna Instrução (vazia se for operando/dado)
  if (!instr_text.empty()) {
    row_elems.push_back(text(instr_text) | color(Color::White) |
                        size(WIDTH, EQUAL, 10));
  } else {
    row_elems.push_back(text("") | size(WIDTH, EQUAL, 16));
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
  base_address_ = std::max(0, base_address_ - lines);
}
void MemoryPanel::scroll_down(int lines) {
  if (machine_)
    base_address_ = std::min(machine_->getMemorySize() - config_.visible_rows,
                             base_address_ + lines);
}
void MemoryPanel::center_on_pc() {
  if (machine_)
    base_address_ =
        std::max(0, machine_->getPCValue() - config_.visible_rows / 2);
}

}  // namespace hidra::tui::panels
