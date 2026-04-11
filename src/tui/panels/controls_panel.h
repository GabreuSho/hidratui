// src/tui/panels/controls_panel.h
#pragma once
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <map>
#include <string>

namespace hidra::tui::panels {

struct ControlAction {
  std::string label;
  char shortcut;
  std::function<void()> callback;
  bool enabled = true;
};

class ControlsPanel {
 public:
  ControlsPanel();

  // Registrar ações
  void add_action(const ControlAction& action);
  void set_action_enabled(const std::string& label, bool enabled);

  // Renderização
  ftxui::Element render() const;
  ftxui::Component create_interactive();

  // Processamento de eventos (para modo interativo)
  bool on_event(const ftxui::Event& event);

 private:
  std::vector<ControlAction> actions_;
  std::map<char, size_t> shortcut_map_;

  ftxui::Element render_button(const ControlAction& action, bool focused) const;
};

}  // namespace hidra::tui::panels
