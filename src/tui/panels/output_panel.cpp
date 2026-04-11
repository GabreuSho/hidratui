#include "output_panel.h"

#include <ftxui/dom/elements.hpp>

using namespace ftxui;

namespace hidra::tui::panels {

OutputPanel::OutputPanel() = default;

void OutputPanel::add_message(const std::string& msg, bool is_error) {
  messages_.push_back({msg, is_error});
  if (is_error) errors_.push_back(msg);
  while (static_cast<int>(messages_.size()) > max_lines_) messages_.pop_front();
}

void OutputPanel::add_build_errors(const std::vector<std::string>& errors) {
  errors_ = errors;
  for (const auto& e : errors) add_message("✗ " + e, true);
}

void OutputPanel::clear() {
  messages_.clear();
  errors_.clear();
}

Element OutputPanel::render() const {
  if (messages_.empty()) {
    return text("  [aguardando execução]") | dim | border;
  }

  Elements lines;
  for (const auto& msg : messages_) {
    lines.push_back(text("  " + msg.text) |
                    color(msg.is_error ? Color::Red : Color::White));
  }

  return vbox(Elements{text(" SAÍDA ") | bold, separator(),
                       vbox(std::move(lines))}) |
         border;
}

}  // namespace hidra::tui::panels
