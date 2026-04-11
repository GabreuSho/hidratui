// src/tui/layout.h
#pragma once
#include <ftxui/dom/elements.hpp>
#include <string>

namespace hidra::tui {

// Configurações do layout
struct LayoutConfig {
  int memory_panel_width = 0;  // 0 = flex
  int registers_panel_width = 30;
  int output_panel_height = 8;
  bool show_header = true;
  bool show_status = true;
  bool follow_pc = true;
  bool show_hex = false;
};

// Funções de composição do layout
ftxui::Element compose_main_layout(ftxui::Element memory,
                                   ftxui::Element registers,
                                   ftxui::Element output,
                                   ftxui::Element controls,
                                   const LayoutConfig& config = LayoutConfig{});

ftxui::Element compose_header(const std::string& title,
                              const std::string& machine,
                              const std::string& filepath, bool running,
                              bool modified);

ftxui::Element compose_status_bar(const std::string& mode,
                                  const std::string& sync_status);

// Helpers de estilo
ftxui::Color get_pc_highlight_color();
ftxui::Color get_flag_color(bool value);
ftxui::Decorator dim_if(bool condition);

}  // namespace hidra::tui
