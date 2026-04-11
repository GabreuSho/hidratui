#include "registers_panel.h"

#include <QString>
#include <ftxui/dom/elements.hpp>
#include <iomanip>
#include <sstream>

#include "layout.h"

using namespace ftxui;

namespace hidra::tui::panels {

bool safe_get_flag_value(const Machine *machine, const QString &flag_name,
                         int &out_value) {
  try {
    out_value = machine->getFlagValue(flag_name);
    return true;
  } catch (const QString &) {
    return false; // Flag não existe nesta máquina
  } catch (...) {
    return false;
  }
}

RegistersPanel::RegistersPanel(const RegistersPanelConfig &config)
    : config_(config) {}

Element RegistersPanel::render() const {
  if (!machine_)
    return text("[sem máquina]") | border;

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
  if (flags)
    content.push_back(flags);

  content.push_back(render_counters());

  return vbox(Elements{text(" REGISTRADORES ") | bold, separator(),
                       vbox(std::move(content))}) |
         border;
}

Element RegistersPanel::render_register(const std::string &name,
                                        int value) const {
  return hbox(Elements{text(name + ": ") | dim,
                       text(format_value(value)) | bold | color(Color::Cyan)});
}

Element RegistersPanel::render_flags() const {
  if (!machine_)
    return nullptr;

  Elements flag_elems;
  std::vector<std::string> flags_to_check = {"N", "Z", "C", "V", "H", "I"};
  if (!config_.flags_to_show.empty())
    flags_to_check = config_.flags_to_show;

  for (const auto &flag_name : flags_to_check) {
    QString qflag = QString::fromStdString(flag_name);
    int flag_value = 0;

    if (safe_get_flag_value(machine_, qflag, flag_value)) {
      flag_elems.push_back(text(flag_name) |
                           color(get_flag_color(flag_value != 0)) | bold |
                           size(WIDTH, EQUAL, 3));
    }
    // Se a flag não existir, simplesmente ignora (sem crash)
  }

  if (flag_elems.empty())
    return nullptr;

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

} // namespace hidra::tui::panels
