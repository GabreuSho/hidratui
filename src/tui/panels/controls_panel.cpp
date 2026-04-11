#include "controls_panel.h"

#include <cctype>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

namespace hidra::tui::panels {

ControlsPanel::ControlsPanel() = default;

void ControlsPanel::add_action(const ControlAction& action) {
  actions_.push_back(action);
  if (action.shortcut) {
    shortcut_map_[std::tolower(action.shortcut)] = actions_.size() - 1;
  }
}

void ControlsPanel::set_action_enabled(const std::string& label, bool enabled) {
  for (auto& action : actions_) {
    if (action.label == label) {
      action.enabled = enabled;
      break;
    }
  }
}

Element ControlsPanel::render() const {
  Elements buttons;
  for (const auto& action : actions_) {
    buttons.push_back(render_button(action, false));
  }
  return hbox(buttons) | border;
}

Element ControlsPanel::render_button(const ControlAction& action,
                                     bool focused) const {
  auto style = action.enabled
                   ? (focused ? bgcolor(Color::Blue) | color(Color::White)
                              : color(Color::Cyan))
                   : color(Color::White) | dim;

  std::string label = "[" + std::string(1, std::toupper(action.shortcut)) +
                      "]" + action.label.substr(1);
  return text(" " + label + " ") | style;
}

Component ControlsPanel::create_interactive() {
  return Renderer([this] { return render(); });
}

bool ControlsPanel::on_event(const Event& event) {
  if (!event.is_character()) return false;

  char c = std::tolower(event.character()[0]);
  auto it = shortcut_map_.find(c);
  if (it != shortcut_map_.end() && actions_[it->second].enabled) {
    actions_[it->second].callback();
    return true;
  }
  return false;
}

}  // namespace hidra::tui::panels
