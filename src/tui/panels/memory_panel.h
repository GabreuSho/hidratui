// src/tui/panels/memory_panel.h
#pragma once
#include <ftxui/dom/elements.hpp>
#include <string>

#include "core/machine.h"

namespace hidra::tui::panels {

struct MemoryPanelConfig {
  int visible_rows = 16;
  bool show_hex = true;
  bool show_decimal = true;
  bool show_disassembly = true;
  bool follow_pc = true;
  int base_address = 0;  // -1 para auto (follow PC)
};

class MemoryPanel {
 public:
  explicit MemoryPanel(const MemoryPanelConfig& config = MemoryPanelConfig{});

  void set_machine(Machine* machine) { machine_ = machine; }
  void set_config(const MemoryPanelConfig& config) { config_ = config; }

  // Gera o elemento FTXUI para renderização
  ftxui::Element render();

  // Navegação
  void scroll_up(int lines = 1);
  void scroll_down(int lines = 1);
  void center_on_pc();

  // Estado
  int get_base_address() const { return base_address_; }
  void set_base_address(int addr) { base_address_ = addr; }
  std::string get_instruction_text(int address);
  std::string get_label_text(int address) const;

 private:
  Machine* machine_ = nullptr;
  MemoryPanelConfig config_;
  int base_address_ = 0;

  ftxui::Element render_row(int address, bool is_pc);
  std::string format_cell(int value, bool hex, bool decimal) const;
  bool is32BitMachine() const;
};

}  // namespace hidra::tui::panels
