#pragma once

#include "core/machine.h"
#include "../machine_config.h"
#include <ftxui/dom/elements.hpp>
#include <vector>
#include <string>

namespace hidra::tui::panels {

struct RegistersPanelConfig {
  bool show_hex = false;
  std::string machine_type;
};

class RegistersPanel {
 public:
  explicit RegistersPanel(const RegistersPanelConfig& config = RegistersPanelConfig{});

  void set_machine(Machine* machine) { machine_ = machine; }
  void set_config(const RegistersPanelConfig& config) { config_ = config; }
  void set_machine_type(const std::string& type) { config_.machine_type = type; }

  ftxui::Element render() const;

  void toggle_hex_mode() { config_.show_hex = !config_.show_hex; }
  bool is_hex_mode() const { return config_.show_hex; }

  // Scroll
  void scroll_up(int lines = 1);
  void scroll_down(int lines = 1);
  void set_visible_rows(int rows) { visible_rows_ = rows; }

 private:
  Machine* machine_ = nullptr;
  RegistersPanelConfig config_;
  int scroll_offset_ = 0;
  int visible_rows_ = 18;

  ftxui::Element render_pc_section() const;
  ftxui::Element render_registers_scrollable() const;
  ftxui::Element render_register(const std::string& name, int value,
                                  const std::string& description = "") const;
  ftxui::Element render_register_32bit(int index, const std::string& abi_name,
                                       int value) const;
  ftxui::Element render_scroll_indicator(int total, int visible, int offset) const;
  ftxui::Element render_flags() const;
  ftxui::Element render_counters() const;

  std::string format_value(int value) const;
  std::string get_register_description(const std::string& name) const;
  MachineConfig get_machine_config() const;
  bool is32BitMachine() const;
};

}  // namespace hidra::tui::panels
