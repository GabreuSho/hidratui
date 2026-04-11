#include "registers_panel.h"

#include <QString>
#include <ftxui/dom/elements.hpp>
#include <iomanip>
#include <sstream>

#include "layout.h"

using namespace ftxui;

namespace hidra::tui::panels {

RegistersPanel::RegistersPanel(const RegistersPanelConfig& config)
    : config_(config) {}

Element RegistersPanel::render() const {
  if (!machine_) return text("[sem máquina]") | border;

  Elements content;
  content.push_back(render_register("PC", machine_->getPCValue()));

  if (machine_->hasRegister("ACC")) {
    content.push_back(
        render_register("ACC", machine_->getRegisterValue("ACC")));
  }
  if (machine_->hasRegister("SP")) {
    content.push_back(render_register("SP", machine_->getRegisterValue("SP")));
  }

  auto flags = render_flags();
  if (flags) content.push_back(flags);

  content.push_back(render_counters());

  return vbox(Elements{text(" REGISTRADORES ") | bold, separator(),
                       vbox(std::move(content))}) |
         border;
}

Element RegistersPanel::render_register(const std::string& name,
                                        int value) const {
  return hbox(Elements{text(name + ": ") | dim,
                       text(format_value(value)) | bold | color(Color::Cyan)});
}

Element RegistersPanel::render_flags() const {
  Elements flag_elems;
  for (const auto& flag_name : config_.flags_to_show) {
    QString qflag = QString::fromStdString(flag_name);
    if (machine_->getFlagValue(qflag) != 0) {
      flag_elems.push_back(text(flag_name) | color(get_flag_color(true)) |
                           bold | size(WIDTH, EQUAL, 3));
    } else {
      flag_elems.push_back(text(flag_name) | color(get_flag_color(false)) |
                           size(WIDTH, EQUAL, 3));
    }
  }

  if (flag_elems.empty()) return nullptr;

  return hbox(Elements{text("FLAGS: ") | dim, hbox(flag_elems)});
}

Element RegistersPanel::render_counters() const {
  return hbox(Elements{
      text("Instr: ") | dim,
      text(std::to_string(machine_->getInstructionCount())) |
          color(Color::Yellow),
      text("  Mem: ") | dim,
      text(std::to_string(machine_->getAccessCount())) | color(Color::Yellow),
  });
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

}  // namespace hidra::tui::panels
