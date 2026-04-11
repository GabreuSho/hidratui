#include "layout.h"

#include <ftxui/dom/elements.hpp>

using namespace ftxui;

namespace hidra::tui {

Element compose_main_layout(Element memory, Element registers, Element output,
                            Element controls, const LayoutConfig& cfg) {
  auto content =
      hbox(Elements{memory | flex,
                    registers | size(WIDTH, EQUAL, cfg.registers_panel_width)});
  auto body = vbox(Elements{
      content | flex, output | size(HEIGHT, EQUAL, cfg.output_panel_height),
      controls});
  return body;
}

Element compose_header(const std::string& title, const std::string& machine,
                       const std::string& filepath, bool running,
                       bool modified) {
  std::string status = running ? " [▶ RODANDO]" : "";
  std::string mod = modified ? " *" : "";
  return hbox(Elements{
             text(title) | bold | color(Color::Cyan), text(" | ") | dim,
             text(machine) | color(Color::Yellow), text(" | ") | dim,
             text(filepath.empty() ? "[sem arquivo]" : filepath + mod) | dim,
             text(status) | color(running ? Color::Green : Color::White)}) |
         border;
}

Element compose_status_bar(const std::string& mode, const std::string& sync) {
  return text(" Modo: " + mode + " | " + sync) | inverted | align_right;
}

Color get_pc_highlight_color() { return Color::DarkBlue; }  // ✅ Corrigido
Color get_flag_color(bool value) { return value ? Color::Green : Color::White; }

}  // namespace hidra::tui
