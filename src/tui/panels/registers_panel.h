// src/tui/panels/registers_panel.h
#pragma once
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

#include "core/machine.h"

namespace hidra::tui::panels {

struct RegistersPanelConfig {
  bool show_hex = false;
  bool show_signed = false;
  std::vector<std::string> flags_to_show = {"N", "Z", "C", "V"};
};

class RegistersPanel {
 public:
  explicit RegistersPanel(
      const RegistersPanelConfig& config = RegistersPanelConfig{});

  void set_machine(Machine* machine) { machine_ = machine; }
  void set_config(const RegistersPanelConfig& config) { config_ = config; }

  ftxui::Element render() const;

  // Toggle helpers
  void toggle_hex_mode() { config_.show_hex = !config_.show_hex; }
  bool is_hex_mode() const { return config_.show_hex; }

 private:
  Machine* machine_ = nullptr;
  RegistersPanelConfig config_;

  ftxui::Element render_register(const std::string& name, int value) const;
  ftxui::Element render_flags() const;
  ftxui::Element render_counters() const;
  std::string format_value(int value) const;
};

}  // namespace hidra::tui::panels
