#include "memory_panel.h"

#include <ftxui/dom/elements.hpp>
#include <iomanip>
#include <sstream>

#include "layout.h"

using namespace ftxui;

namespace hidra::tui::panels {

MemoryPanel::MemoryPanel(const MemoryPanelConfig& config) : config_(config) {}

Element MemoryPanel::render() const {
  if (!machine_) return text("[sem máquina]") | border;

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

  auto content = vbox(std::move(rows));
  return vbox(Elements{text(" MEMÓRIA ") | bold, separator(), content}) |
         border;
}

Element MemoryPanel::render_row(int address, bool is_pc) const {
  int value = machine_->getMemoryValue(address);
  std::string instr = machine_->getInstructionString(address).toStdString();

  std::ostringstream addr_ss, hex_ss, dec_ss;
  addr_ss << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
          << address;
  hex_ss << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
         << value;
  dec_ss << std::dec << value;

  Elements row_elems;
  row_elems.push_back(text(addr_ss.str()) | color(Color::White));
  row_elems.push_back(text(" "));
  row_elems.push_back(config_.show_hex
                          ? (text(hex_ss.str()) | color(Color::Green) | bold)
                          : text("  "));
  row_elems.push_back(text(" "));
  row_elems.push_back(config_.show_decimal
                          ? (text(dec_ss.str()) | color(Color::White))
                          : text("  "));
  row_elems.push_back(text("  "));
  row_elems.push_back(config_.show_disassembly && !instr.empty()
                          ? (text(instr) | color(Color::Yellow))
                          : text("..."));

  auto row = hbox(row_elems);
  if (is_pc) {
    row = row | bgcolor(get_pc_highlight_color()) | color(Color::White);
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

}  // namespace hidra::tui::panels
