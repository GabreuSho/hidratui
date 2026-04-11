#include "tui/tui_app.h"
#include "tui/layout.h"
#include "tui/panels/controls_panel.h"
#include "tui/panels/memory_panel.h"
#include "tui/panels/output_panel.h"
#include "tui/panels/registers_panel.h"

// ⚠️ Headers das máquinas concretas (obrigatório para std::make_unique)
#include "machines/ahmesmachine.h"
#include "machines/neandermachine.h"
#include "machines/periclesmachine.h" // Se existir no seu projeto
#include "machines/ramsesmachine.h"
#include "machines/regmachine.h"
#include "machines/voltamachine.h"

#include <QString>
#include <chrono>
#include <fstream>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <sstream>
#include <thread>

using namespace ftxui;

TuiApp::TuiApp(const std::string &machine_type) : machine_type_(machine_type) {
  if (machine_type == "neander")
    machine_ = std::make_unique<NeanderMachine>();
  else if (machine_type == "ahmes")
    machine_ = std::make_unique<AhmesMachine>();
  else if (machine_type == "ramses")
    machine_ = std::make_unique<RamsesMachine>();
  else if (machine_type == "reg")
    machine_ = std::make_unique<RegMachine>();
  else if (machine_type == "volta")
    machine_ = std::make_unique<VoltaMachine>();
  else if (machine_type == "pericles")
    machine_ = std::make_unique<PericlesMachine>();
  else
    throw std::runtime_error("Máquina não suportada: " + machine_type);

  memory_panel_.set_machine(machine_.get());
  registers_panel_.set_machine(machine_.get());

  controls_panel_.add_action({"uild", 'b', [this]() { do_build(); }});
  controls_panel_.add_action({"tep", 's', [this]() { do_step(); }});
  controls_panel_.add_action({"un/Stop", 'r', [this]() {
                                if (running_)
                                  machine_->setRunning(false);
                                else
                                  do_run();
                              }});
  controls_panel_.add_action({"eset PC", 'p', [this]() { do_reset_pc(); }});
  controls_panel_.add_action({"eset All", 'x', [this]() { do_reset(); }});
  controls_panel_.add_action({"uit", 'q', []() {}, false});

  layout_config_.show_hex = false;
  layout_config_.follow_pc = true;
  layout_config_.registers_panel_width = 28;
  layout_config_.output_panel_height = 5;
}

TuiApp::~TuiApp() {
  if (file_monitor_)
    file_monitor_->stop();
}

void TuiApp::load_file(const std::string &filepath) {
  current_filepath_ = filepath;
  do_build();
  file_monitor_ = std::make_unique<FileMonitor>(filepath);
  file_monitor_->set_callback([this]() { on_file_changed(); });
  file_monitor_->start();
}

void TuiApp::on_file_changed() {
  file_modified_ = true;
  do_build();
}

void TuiApp::do_build() {
  std::ifstream file(current_filepath_);
  if (!file) {
    output_panel_.add_message("Arquivo não encontrado", true);
    return;
  }
  std::stringstream buffer;
  buffer << file.rdbuf();

  try {
    machine_->assemble(QString::fromStdString(buffer.str()));
    file_modified_ = false;
    output_panel_.clear();
    output_panel_.add_message("✓ Montagem OK", false);
  } catch (const QString &e) {
    output_panel_.clear();
    output_panel_.add_message("✗ Erro: " + e.toStdString(), true);
  } catch (const std::exception &e) {
    output_panel_.clear();
    output_panel_.add_message(std::string("✗ Erro: ") + e.what(), true);
  } catch (...) {
    output_panel_.clear();
    output_panel_.add_message("✗ Erro desconhecido na montagem", true);
  }
}

void TuiApp::do_step() {
  if (!machine_->getBuildSuccessful())
    return;
  try {
    machine_->step();
  } catch (const QString &e) {
    output_panel_.add_message("Erro: " + e.toStdString(), true);
  } catch (const std::exception &e) {
    output_panel_.add_message(std::string("Erro: ") + e.what(), true);
  } catch (...) {
    output_panel_.add_message("Erro desconhecido", true);
  }
}

void TuiApp::do_run() {
  if (!machine_->getBuildSuccessful())
    return;
  running_ = true;
  while (running_ && machine_->isRunning()) {
    try {
      machine_->step();
    } catch (...) {
      break; // Para execução contínua em qualquer erro
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
  }
  running_ = false;
}
void TuiApp::do_reset() {
  machine_->clear();
  running_ = false;
  output_panel_.add_message("Sistema resetado");
}

void TuiApp::do_reset_pc() {
  machine_->setPCValue(0);
  if (machine_->hasRegister(QString("SP")))
    machine_->setRegisterValue(QString("SP"), 0);
  output_panel_.add_message("PC resetado para 0");
}

void TuiApp::run() {
  auto renderer = Renderer([&] { return render(); });
  auto main_component = CatchEvent(renderer, [this](Event event) {
    if (event == Event::Character('q') ||
        event == Event::Character(static_cast<char>(3)))
      return false; // 3 = Ctrl+C
    if (event.is_character()) {
      char c = event.character()[0];
      switch (c) {
      case 'h':
        layout_config_.show_hex = !layout_config_.show_hex;
        return true;
      case 'f':
        layout_config_.follow_pc = !layout_config_.follow_pc;
        return true;
      case 'b':
        do_build();
        return true;
      case 's':
        do_step();
        return true;
      case 'r':
        if (running_)
          machine_->setRunning(false);
        else
          do_run();
        return true;
      case 'x':
        do_reset();
        return true;
      case 'p':
        do_reset_pc();
        return true;
      }
    }
    if (event == Event::Escape && running_) {
      machine_->setRunning(false);
      return true;
    }
    return false;
  });

  ScreenInteractive::Fullscreen().Loop(main_component);
}

Element TuiApp::render() {
  memory_panel_.set_config({.visible_rows = 16,
                            .show_hex = layout_config_.show_hex,
                            .show_decimal = true,
                            .show_disassembly = true,
                            .follow_pc = layout_config_.follow_pc});
  registers_panel_.set_config({.show_hex = layout_config_.show_hex});

  return vbox(
      Elements{render_header(),
               hbox(Elements{memory_panel_.render() | flex,
                             registers_panel_.render() |
                                 size(WIDTH, EQUAL,
                                      layout_config_.registers_panel_width)}) |
                   flex,
               output_panel_.render() |
                   size(HEIGHT, EQUAL, layout_config_.output_panel_height),
               controls_panel_.render(), render_status_bar()});
}

Element TuiApp::render_header() {
  std::string title = "HIDRA-TUI";
  std::string file_info =
      current_filepath_.empty() ? "[sem arquivo]" : current_filepath_;
  if (file_modified_)
    file_info += " *";
  if (running_)
    title += " [RODANDO]";

  return hidra::tui::compose_header(title, machine_type_, file_info, running_,
                                    file_modified_);
}

Element TuiApp::render_status_bar() {
  return hidra::tui::compose_status_bar(
      layout_config_.show_hex ? "HEX" : "DEC",
      file_modified_ ? "MODIFICADO (auto-recarregando)" : "sincronizado");
}
