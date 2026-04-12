#include "memory_panel.h"
#include "layout.h"
#include <ftxui/dom/elements.hpp>
#include <iomanip>
#include <sstream>

using namespace ftxui;

namespace hidra::tui::panels {

MemoryPanel::MemoryPanel(const MemoryPanelConfig &config) : config_(config) {}

Element MemoryPanel::render() const {
  if (!machine_)
    return text("[sem máquina]") | border;

  int mem_size = machine_->getMemorySize();
  int pc = machine_->getPCValue();

  int base = config_.follow_pc ? std::max(0, pc - config_.visible_rows / 2)
                               : base_address_;
  base = std::max(0, std::min(base, mem_size - config_.visible_rows));

  Elements rows;
  for (int i = 0; i < config_.visible_rows && base + i < mem_size; ++i) {
    int addr = base + i;
    bool is_pc = (addr == pc && machine_->isRunning());
    rows.push_back(render_row(addr, is_pc));
  }

  return vbox(Elements{text(" MEMÓRIA ") | bold, separator(),
                       vbox(std::move(rows))}) |
         border;
}

Element MemoryPanel::render_row(int address, bool is_pc) const {
  int value = machine_->getMemoryValue(address);
  std::string instr = get_instruction_text(address);

  std::ostringstream addr_ss, val_ss;
  addr_ss << std::setw(3) << std::setfill(' ') << address;
  val_ss << std::setw(3) << std::setfill(' ') << value;

  auto addr_elem = text(addr_ss.str()) | color(Color::Green);
  auto val_elem = text(val_ss.str()) | color(Color::Yellow);

  std::string display_text = instr.empty() ? "(dado)" : instr;
  auto text_elem = text(display_text) | color(Color::White);

  auto row = hbox(Elements{addr_elem | size(WIDTH, EQUAL, 4), text(" ") | dim,
                           val_elem | size(WIDTH, EQUAL, 4), text("  ") | dim,
                           text_elem | flex});

  // ✅ HIGHLIGHT: Se for a linha do PC, inverte as cores e coloca negrito
  if (is_pc) {
    row = row | bgcolor(Color::Blue) | color(Color::White) | bold;
  }

  return row;
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

std::string MemoryPanel::get_instruction_text(int address) const {
  if (!machine_)
    return "...";
  try {
    QString instr = machine_->getInstructionString(address);
    if (instr.isEmpty())
      return "...";
    return instr.toStdString();
  } catch (...) {
    return "...";
  }
}
} // namespace hidra::tui::panels
