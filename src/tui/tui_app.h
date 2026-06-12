#pragma once
#include <atomic>
#include <chrono>
#include <ftxui/dom/elements.hpp>
#include <memory>
#include <string>

#include "core/machine.h"
#include "tui/file_monitor.h"
#include "tui/layout.h"
#include "tui/panels/controls_panel.h"
#include "tui/panels/goto_popup.h"
#include "tui/panels/memory_panel.h"
#include "tui/panels/output_panel.h"
#include "tui/panels/registers_panel.h"

class TuiApp {
 public:
  explicit TuiApp(const std::string& machine_type);
  ~TuiApp();

  void load_file(const std::string& filepath);
  void run();

  void do_build();
  void do_step();
  void do_reset();
  void do_reset_pc();
  void do_run();

  bool is_running() const { return running_; }
  bool is_modified() const { return file_modified_; }

 private:
  std::unique_ptr<Machine> machine_;
  std::unique_ptr<FileMonitor> file_monitor_;
  std::string current_filepath_;
  std::string machine_type_;

  bool running_ = false;
  std::atomic<bool> file_modified_{false};
  int address_go_ = 0;
  int running_steps_ = -1;
  int address_running_aux_ = 0;

  // Controle de tempo para execução frame-a-frame
  std::chrono::steady_clock::time_point last_step_time_;
  std::chrono::milliseconds step_interval_{50};

  // Painéis da UI
  hidra::tui::panels::MemoryPanel memory_panel_;
  hidra::tui::panels::RegistersPanel registers_panel_;
  hidra::tui::panels::OutputPanel output_panel_;
  hidra::tui::panels::ControlsPanel controls_panel_;
  hidra::tui::panels::GotoPopup goto_popup_;
  hidra::tui::LayoutConfig layout_config_;

  void on_file_changed();
  ftxui::Element render();
  ftxui::Element render_header();
  ftxui::Element render_status_bar();
  ftxui::Element render_controls_panel();
};
