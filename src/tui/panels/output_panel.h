// src/tui/panels/output_panel.h
#pragma once
#include <deque>
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

namespace hidra::tui::panels {

class OutputPanel {
 public:
  OutputPanel();

  // Adicionar mensagens
  void add_message(const std::string& msg, bool is_error = false);
  void add_build_errors(const std::vector<std::string>& errors);
  void clear();

  // Configuração
  void set_max_lines(int max) { max_lines_ = max; }

  // Renderização
  ftxui::Element render() const;

  // Estado
  bool has_errors() const { return !errors_.empty(); }
  int get_message_count() const { return static_cast<int>(messages_.size()); }

 private:
  struct Message {
    std::string text;
    bool is_error;
  };

  std::deque<Message> messages_;
  std::vector<std::string> errors_;
  int max_lines_ = 6;

  ftxui::Color get_color(bool is_error) const;
};

}  // namespace hidra::tui::panels
