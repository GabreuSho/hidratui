#include "registers_panel.h"
#include "../layout.h"
#include <ftxui/dom/elements.hpp>
#include <iomanip>
#include <sstream>

using namespace ftxui;

namespace hidra::tui::panels {

RegistersPanel::RegistersPanel(const RegistersPanelConfig &config)
    : config_(config) {}

MachineConfig RegistersPanel::get_machine_config() const {
  return MachineConfigProvider::getConfig(config_.machine_type);
}

Element RegistersPanel::render() const {
  if (!machine_)
    return text("[sem máquina]") | border;

  Elements content;
  content.push_back(render_all_registers());

  auto flags = render_flags();
  if (flags) {
    content.push_back(separator());
    content.push_back(flags);
  }

  content.push_back(separator());
  content.push_back(render_counters());

  // ✅ FTXUI v5: usar vbox com Elements{}, não encadear | text()
  return vbox(Elements{text(" REGISTRADORES ") | bold, separator(),
                       vbox(std::move(content))}) |
         border;
}

Element RegistersPanel::render_all_registers() const {
  MachineConfig config = get_machine_config();
  Elements regs;

  if (!config.registers.empty()) {
    for (const auto &reg_name : config.registers) {
      try {
        int val = machine_->getRegisterValue(QString::fromStdString(reg_name));
        regs.push_back(
            render_register(reg_name, val, get_register_description(reg_name)));
      } catch (...) {
        // Registrador não disponível
      }
    }
  } else {
    std::vector<std::string> common_regs = {"PC", "ACC", "AC", "A",
                                            "B",  "X",   "Y",  "SP"};
    for (const auto &reg_name : common_regs) {
      if (machine_->hasRegister(QString::fromStdString(reg_name))) {
        try {
          int val =
              machine_->getRegisterValue(QString::fromStdString(reg_name));
          regs.push_back(render_register(reg_name, val,
                                         get_register_description(reg_name)));
        } catch (...) {
          continue;
        }
      }
    }
  }

  return vbox(std::move(regs));
}

Element RegistersPanel::render_register(const std::string &name, int value,
                                        const std::string &description) const {
  std::ostringstream oss;
  oss << std::setw(3) << std::setfill(' ') << value;

  // ✅ FTXUI v5: agrupar elementos em hbox com Elements{}
  Elements row;
  row.push_back(text(name + ": ") | dim | size(WIDTH, EQUAL, 6));
  row.push_back(text(oss.str()) | bold | color(Color::Cyan));

  if (!description.empty()) {
    row.push_back(text(" ") | dim);
    row.push_back(text("(" + description + ")") | dim);
  }

  return hbox(row);
}

Element RegistersPanel::render_flags() const {
  if (!machine_)
    return nullptr;

  MachineConfig config = get_machine_config();
  Elements flag_elems;
  bool has_any_flag = false;

  std::vector<std::string> flags_to_show = config.flags;
  if (flags_to_show.empty()) {
    flags_to_show = {"N", "Z", "C", "V", "H", "I"};
  }

  for (const auto &flag_name : flags_to_show) {
    QString qflag = QString::fromStdString(flag_name);
    try {
      int val = machine_->getFlagValue(qflag);
      flag_elems.push_back(hbox(Elements{
          text(flag_name + ":") | dim | size(WIDTH, EQUAL, 3),
          text(val != 0 ? "1" : "0") | color(get_flag_color(val != 0)) | bold |
              size(WIDTH, EQUAL, 2)}));
      has_any_flag = true;
    } catch (...) {
      continue;
    }
  }

  if (!has_any_flag)
    return nullptr;

  return vbox(
      Elements{text("FLAGS") | bold | color(Color::Yellow), hbox(flag_elems)});
}

Element RegistersPanel::render_counters() const {
  return vbox(Elements{
      text("CONTADORES") | bold | color(Color::Yellow),
      hbox(Elements{text("Instruções: ") | dim,
                    text(std::to_string(machine_->getInstructionCount())) |
                        color(Color::Green) | bold,
                    text("  ") | dim, text("Acessos: ") | dim,
                    text(std::to_string(machine_->getAccessCount())) |
                        color(Color::Magenta) | bold})});
}

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

std::string
RegistersPanel::get_register_description(const std::string &name) const {
  MachineConfig config = get_machine_config();
  auto it = config.register_descriptions.find(name);
  if (it != config.register_descriptions.end())
    return it->second;

  if (name == "PC")
    return "Program Counter";
  if (name == "ACC" || name == "AC" || name == "A")
    return "Acumulador";
  if (name == "SP")
    return "Stack Pointer";
  if (name == "X")
    return "Índice X";
  if (name == "Y")
    return "Índice Y";
  if (name == "B")
    return "Base";
  return "";
}

} // namespace hidra::tui::panels
