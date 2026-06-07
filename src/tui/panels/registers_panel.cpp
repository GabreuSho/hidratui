#include "registers_panel.h"
#include "../layout.h"
#include <ftxui/dom/elements.hpp>
#include <iomanip>
#include <sstream>

using namespace ftxui;

namespace hidra::tui::panels {

RegistersPanel::RegistersPanel(const RegistersPanelConfig& config)
    : config_(config) {}

MachineConfig RegistersPanel::get_machine_config() const {
  return MachineConfigProvider::getConfig(config_.machine_type);
}

bool RegistersPanel::is32BitMachine() const {
  if (!machine_) return false;
  return machine_->getMemorySize() >= 65536;
}

// ============================================================================
// Scroll
// ============================================================================

void RegistersPanel::scroll_up(int lines) {
  scroll_offset_ = std::max(0, scroll_offset_ - lines);
}

void RegistersPanel::scroll_down(int lines) {
  MachineConfig config = get_machine_config();
  int total = static_cast<int>(config.registers.size());
  // PC is shown separately, so exclude it from scrollable count
  if (is32BitMachine()) total -= 1;
  int max_offset = std::max(0, total - visible_rows_);
  scroll_offset_ = std::min(max_offset, scroll_offset_ + lines);
}

// ============================================================================
// Render — main layout: PC (fixed) | separator | scrollable regs | flags | counters
// ============================================================================

Element RegistersPanel::render() const {
  if (!machine_) return text("[sem máquina]") | border;

  Elements content;
  content.push_back(render_pc_section());
  content.push_back(separator());
  content.push_back(render_registers_scrollable());

  auto flags = render_flags();
  if (flags) {
    content.push_back(separator());
    content.push_back(flags);
  }

  content.push_back(separator());
  content.push_back(render_counters());

  return vbox(Elements{text(" REGISTRADORES ") | bold, separator(),
                       vbox(std::move(content))}) |
         border;
}

// ============================================================================
// PC section — always visible at the top
// ============================================================================

Element RegistersPanel::render_pc_section() const {
  if (!machine_) return text("PC: ?");

  int pc = machine_->getPCValue();
  bool large = is32BitMachine();

  std::ostringstream oss;
  if (large) {
    oss << "0x" << std::hex << std::setw(8) << std::setfill('0')
        << static_cast<unsigned int>(pc);
  } else {
    oss << std::dec << pc;
  }

  Elements row;
  row.push_back(text("PC: ") | bold | color(Color::Yellow));
  row.push_back(text(oss.str()) | bold | color(Color::Green));

  return hbox(row);
}

// ============================================================================
// Scrollable registers area
// ============================================================================

Element RegistersPanel::render_registers_scrollable() const {
  MachineConfig config = get_machine_config();
  bool large = is32BitMachine();
  Elements regs;

  if (!config.registers.empty()) {
    // Build list of register names excluding PC (shown separately above)
    std::vector<std::string> reg_names;
    for (const auto& name : config.registers) {
      if (name == "PC") continue;
      reg_names.push_back(name);
    }

    int total = static_cast<int>(reg_names.size());

    if (large) {
      // 32-bit: x0(zero) 0x00000000 format with scroll
      for (int i = scroll_offset_;
           i < std::min(scroll_offset_ + visible_rows_, total); ++i) {
        const auto& name = reg_names[i];
        try {
          int val =
              machine_->getRegisterValue(QString::fromStdString(name));
          regs.push_back(render_register_32bit(i, name, val));
        } catch (...) {
        }
      }
    } else {
      // 8-bit: compact format, no scroll needed (few registers)
      for (const auto& name : reg_names) {
        try {
          int val =
              machine_->getRegisterValue(QString::fromStdString(name));
          regs.push_back(
              render_register(name, val, get_register_description(name)));
        } catch (...) {
        }
      }
    }

    // Scroll indicator for 32-bit
    if (large && total > visible_rows_) {
      regs.push_back(render_scroll_indicator(total, visible_rows_, scroll_offset_));
    }
  } else {
    // Fallback for unknown machine types
    std::vector<std::string> common_regs = {"ACC", "AC", "A", "B", "X", "Y", "SP"};
    for (const auto& name : common_regs) {
      if (machine_->hasRegister(QString::fromStdString(name))) {
        try {
          int val =
              machine_->getRegisterValue(QString::fromStdString(name));
          regs.push_back(
              render_register(name, val, get_register_description(name)));
        } catch (...) {
          continue;
        }
      }
    }
  }

  return vbox(std::move(regs));
}

// ============================================================================
// 32-bit register: x0(zero) 0x00000000
// ============================================================================

Element RegistersPanel::render_register_32bit(int index,
                                               const std::string& abi_name,
                                               int value) const {
  // Numeric name: x0..x31
  std::string num_name = "x" + std::to_string(index);

  // Value
  std::ostringstream oss;
  if (config_.show_hex) {
    oss << "0x" << std::hex << std::setw(8) << std::setfill('0')
        << static_cast<unsigned int>(value);
  } else {
    oss << std::dec << value;
  }

  Elements row;
  // x0  — width 3
  row.push_back(text(num_name) | dim | size(WIDTH, EQUAL, 3));
  // (zero) — width 7
  row.push_back(text("(" + abi_name + ")") | color(Color::Cyan) |
                size(WIDTH, EQUAL, 8));
  // 0x00000000
  row.push_back(text(oss.str()) | bold | color(Color::White));

  return hbox(row);
}

// ============================================================================
// 8-bit register: AC: 42 (Accumulador)
// ============================================================================

Element RegistersPanel::render_register(const std::string& name, int value,
                                         const std::string& description) const {
  std::ostringstream oss;
  oss << std::setw(3) << std::setfill(' ') << value;

  Elements row;
  int name_width = 6;
  row.push_back(text(name + ": ") | dim | size(WIDTH, EQUAL, name_width));
  row.push_back(text(oss.str()) | bold | color(Color::Cyan));

  if (!description.empty()) {
    row.push_back(text(" ") | dim);
    row.push_back(text("(" + description + ")") | dim);
  }

  return hbox(row);
}

// ============================================================================
// Scroll indicator
// ============================================================================

Element RegistersPanel::render_scroll_indicator(int total, int visible,
                                                  int offset) const {
  int last_visible = std::min(offset + visible, total);
  std::ostringstream oss;
  oss << " " << (offset + 1) << "-" << last_visible << "/" << total
      << " Ctrl+\u2191\u2193";

  return text(oss.str()) | dim | align_right;
}

// ============================================================================
// Flags
// ============================================================================

Element RegistersPanel::render_flags() const {
  if (!machine_) return nullptr;

  MachineConfig config = get_machine_config();
  Elements flag_elems;
  bool has_any_flag = false;

  std::vector<std::string> flags_to_show = config.flags;
  if (flags_to_show.empty()) {
    flags_to_show = {"N", "Z", "C", "V", "H", "I"};
  }

  for (const auto& flag_name : flags_to_show) {
    QString qflag = QString::fromStdString(flag_name);
    try {
      int val = machine_->getFlagValue(qflag);
      flag_elems.push_back(hbox(Elements{
          text(flag_name + ":") | dim | size(WIDTH, EQUAL, 3),
          text(val != 0 ? "1" : "0") |
              color(get_flag_color(val != 0)) | bold |
              size(WIDTH, EQUAL, 2)}));
      has_any_flag = true;
    } catch (...) {
      continue;
    }
  }

  if (!has_any_flag) return nullptr;

  return vbox(Elements{text("FLAGS") | bold | color(Color::Yellow),
                       hbox(flag_elems)});
}

// ============================================================================
// Counters
// ============================================================================

Element RegistersPanel::render_counters() const {
  return vbox(Elements{
      text("CONTADORES") | bold | color(Color::Yellow),
      hbox(Elements{
          text("Instruções: ") | dim,
          text(std::to_string(machine_->getInstructionCount())) |
              color(Color::Green) | bold,
          text(" ") | dim,
          text("Acessos: ") | dim,
          text(std::to_string(machine_->getAccessCount())) |
              color(Color::Magenta) | bold})});
}

// ============================================================================
// Helpers
// ============================================================================

std::string RegistersPanel::format_value(int value) const {
  std::ostringstream oss;
  if (config_.show_hex) {
    oss << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
        << value;
  } else {
    oss << std::dec << value;
  }
  return oss.str();
}

std::string RegistersPanel::get_register_description(
    const std::string& name) const {
  MachineConfig config = get_machine_config();
  auto it = config.register_descriptions.find(name);
  if (it != config.register_descriptions.end()) return it->second;

  if (name == "PC") return "Program Counter";
  if (name == "ACC" || name == "AC" || name == "A") return "Acumulador";
  if (name == "SP") return "Stack Pointer";
  if (name == "X") return "Índice X";
  if (name == "Y") return "Índice Y";
  if (name == "B") return "Base";
  return "";
}

}  // namespace hidra::tui::panels
