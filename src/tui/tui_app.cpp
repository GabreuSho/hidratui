#include "tui/tui_app.h"

#include <QString>
#include <atomic>
#include <chrono>
#include <fstream>
#include <ftxui/component/animation.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <sstream>
#include <thread>

#include "machines/ahmesmachine.h"
#include "machines/neandermachine.h"
#include "machines/ramsesmachine.h"
#include "machines/regmachine.h"
#include "machines/voltamachine.h"
#include "machines/rv32immachine.h"
#include "tui/layout.h"
#include "tui/panels/controls_panel.h"
#include "tui/panels/memory_panel.h"
#include "tui/panels/output_panel.h"
#include "tui/panels/registers_panel.h"

using namespace ftxui;
using namespace std::chrono;

TuiApp::TuiApp(const std::string& machine_type) : machine_type_(machine_type) {
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
    else if (machine_type == "rv32im")
        machine_ = std::make_unique<RV32IMMachine>();
  else
    throw std::runtime_error("Máquina não suportada: " + machine_type);

  memory_panel_.set_machine(machine_.get());
  registers_panel_.set_machine(machine_.get());
  registers_panel_.set_machine_type(machine_type);

  controls_panel_.add_action({"uild", 'b', [this]() { do_build(); }});
  controls_panel_.add_action({"tep", 's', [this]() { do_step(); }});
  controls_panel_.add_action({"un/Stop", 'r', [this]() {
                                if (running_) {
                                  running_ = false;
                                  machine_->setRunning(false);
                                } else
                                  do_run();
                              }});
  controls_panel_.add_action({"eset PC", 'p', [this]() { do_reset_pc(); }});
  controls_panel_.add_action({"eset All", 'x', [this]() { do_reset(); }});
  controls_panel_.add_action({"uit", 'q', []() {}, false});

  layout_config_.show_hex = false;
  layout_config_.follow_pc = true;
  layout_config_.registers_panel_width = 40;
  layout_config_.output_panel_height = 5;
}

TuiApp::~TuiApp() {
  if (file_monitor_) file_monitor_->stop();
}

void TuiApp::load_file(const std::string& filepath) {
  current_filepath_ = filepath;
  do_build();
  file_monitor_ = std::make_unique<FileMonitor>(filepath);
  file_monitor_->set_callback([this]() { on_file_changed(); });
  file_monitor_->start();
}

void TuiApp::on_file_changed() {
  file_modified_ = true;
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
    machine_->updateInstructionStrings();
    file_modified_ = false;
    output_panel_.clear();
    output_panel_.add_message("✓ Montagem OK", false);
  } catch (const QString& e) {
    output_panel_.clear();
    output_panel_.add_message("✗ Erro: " + e.toStdString(), true);
  } catch (const std::exception& e) {
    output_panel_.clear();
    output_panel_.add_message(std::string("✗ Erro: ") + e.what(), true);
  } catch (...) {
    output_panel_.clear();
    output_panel_.add_message("✗ Erro desconhecido", true);
  }
}

void TuiApp::do_step() {
  if (!machine_->getBuildSuccessful()) return;
  machine_->setRunning(true);  // Necessário para o core permitir execução
  try {
    machine_->step();
  } catch (const QString& e) {
    output_panel_.add_message("Erro: " + e.toStdString(), true);
  } catch (...) {
    output_panel_.add_message("Erro na execução", true);
  }
}

void TuiApp::do_run() {
  if (!machine_->getBuildSuccessful()) return;
  machine_->setRunning(true);
  running_ = true;
  last_step_time_ = steady_clock::now();
}

void TuiApp::do_reset() {
  machine_->clear();
  machine_->setRunning(false);
  running_ = false;
  output_panel_.add_message("Sistema resetado");
}

void TuiApp::do_reset_pc() {
  if (machine_->getMemorySize() >= 65536) {
    // RV32IM: reset to RARS-compatible defaults
    machine_->setPCValue(0x00400000);
    if (machine_->hasRegister(QString("SP")))
      machine_->setRegisterValue(QString("SP"), 0x7FFFFFFC);
    output_panel_.add_message("PC resetado para 0x00400000");
  } else {
    machine_->setPCValue(0);
    if (machine_->hasRegister(QString("SP")))
      machine_->setRegisterValue(QString("SP"), 0);
    output_panel_.add_message("PC resetado para 0");
  }
}

void TuiApp::run() {
  auto screen = ScreenInteractive::Fullscreen();
  last_step_time_ = steady_clock::now();

  // Flag atômica para controlar a thread de timer
  std::atomic<bool> timer_active{true};

  // ============================================================================
  // TIMER THREAD: Envia evento "dummy" a cada 50ms para forçar redraw da UI
  // PostEvent é thread-safe no FTXUI e acorda o loop principal
  // ============================================================================
  std::thread timer_thread([&screen, &timer_active]() {
    while (timer_active.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      // Envia evento espaço (' ') que será ignorado pelo handler, mas acorda o
      // loop
      screen.PostEvent(Event::Character(' '));
    }
  });

  // ============================================================================
  // RENDERER: Renderiza UI + Auto-Build + Simulação (thread principal)
  // ============================================================================
  auto renderer = Renderer([&] {
    // 1. AUTO-BUILD: Verifica mudança no arquivo e recompila
    if (file_modified_) {
      do_build();
    }

    // 2. SIMULAÇÃO: Executa 1 passo a cada step_interval_ (ex: 100ms)
    if (running_ && machine_ && machine_->isRunning() &&
        ((this->running_steps_ > 0) || (running_steps_ == -1))) {
      auto now = steady_clock::now();
      if ((now - last_step_time_ >= step_interval_)) {
        try {
          machine_->step();
          if (running_steps_ > 0) this->running_steps_ -= 1;
        } catch (const QString& e) {
          output_panel_.add_message("Erro: " + e.toStdString(), true);
          running_ = false;
          machine_->setRunning(false);
        } catch (const std::exception& e) {
          output_panel_.add_message(std::string("Erro: ") + e.what(), true);
          running_ = false;
          machine_->setRunning(false);
        } catch (...) {
          output_panel_.add_message("Erro desconhecido", true);
          running_ = false;
          machine_->setRunning(false);
        }
        last_step_time_ = now;
      }
    } else if (running_steps_ == 0) {
      running_ = false;
      machine_->setRunning(false);
      running_steps_ -= 1;
    }

    return render();
  });

  // ============================================================================
  // EVENT HANDLER: Captura teclas e controla o fluxo
  // ============================================================================
  auto main_component = CatchEvent(renderer, [this, &screen,
                                              &timer_active](Event event) {
    // Q ou ESC -> Para tudo e sai
    if (event == Event::Character('q') || event == Event::Escape) {
      timer_active.store(false);  // Para a thread de timer
      running_ = false;
      if (machine_) machine_->setRunning(false);
      screen.Exit();
      return true;
    }

    // Ignora o evento "dummy" do timer (espaço)
    if (event == Event::Character(' ')) {
      return true;
    }

    if (event.is_character()) {
      char c = event.character()[0];
      switch (c) {
        case '1':
          this->address_running_aux_ = (this->address_running_aux_ * 10) + 1;
          return true;
        case '2':
          this->address_running_aux_ = (this->address_running_aux_ * 10) + 2;
          return true;
        case '3':
          this->address_running_aux_ = (this->address_running_aux_ * 10) + 3;
          return true;
        case '4':
          this->address_running_aux_ = (this->address_running_aux_ * 10) + 4;
          return true;
        case '5':
          this->address_running_aux_ = (this->address_running_aux_ * 10) + 5;
          return true;
        case '6':
          this->address_running_aux_ = (this->address_running_aux_ * 10) + 6;
          return true;
        case '7':
          this->address_running_aux_ = (this->address_running_aux_ * 10) + 7;
          return true;
        case '8':
          this->address_running_aux_ = (this->address_running_aux_ * 10) + 8;
          return true;
        case '9':
          this->address_running_aux_ = (this->address_running_aux_ * 10) + 9;
          return true;
        case '0':
          this->address_running_aux_ = (this->address_running_aux_ * 10) + 0;
          return true;
        case 'r':  // Run/Stop toggle
          if (address_running_aux_ > 0) {
            this->running_steps_ = this->address_running_aux_;
            this->address_running_aux_ = 0;
          }
          running_ = !running_;
          if (running_ && machine_) {
            machine_->setRunning(true);
            last_step_time_ = steady_clock::now();
          } else if (machine_) {
            machine_->setRunning(false);
          }
          return true;
        case 's':  // Step único
          if (address_running_aux_ >= 0) {
            running_steps_ = address_running_aux_;
            address_running_aux_ = 0;
          }
          if (!running_ && machine_) {
            do {
              machine_->setRunning(true);
              do_step();
              machine_->setRunning(false);
              running_steps_ -= 1;
            } while (running_steps_ > 0);
          }
          return true;
        case 'b':  // Build manual + para execução
          running_ = false;
          if (machine_) machine_->setRunning(false);
          do_build();
          return true;
        case 'x':  // Reset completo
          running_ = false;
          if (machine_) machine_->setRunning(false);
          do_reset();
          return true;
        case 'p':  // Reset PC
          do_reset_pc();
          return true;
        case 'h':  // Toggle Hex/Decimal
          layout_config_.show_hex = !layout_config_.show_hex;
          return true;
        case 'f':  // Toggle Follow PC
          layout_config_.follow_pc = !layout_config_.follow_pc;
          return true;
        case 'k':
        case 'u':  // Scroll up
          memory_panel_.scroll_up(1);
          layout_config_.follow_pc = false;
          return true;
        case 'j':
        case 'd':  // Scroll down
          memory_panel_.scroll_down(1);
          layout_config_.follow_pc = false;
          return true;
        case 'g':
          this->address_go_ = this->address_running_aux_;
          this->address_running_aux_ = 0;

          if (this->address_go_ > 0 and
              this->address_go_ < machine_->getMemorySize()) {
            memory_panel_.set_base_address(this->address_go_);
          } else {
            memory_panel_.set_base_address(0);
          }
          this->address_go_ = 0;
          layout_config_.follow_pc = false;
          return true;
        case 'G':
          this->address_go_ = this->address_running_aux_;
          this->address_running_aux_ = 0;
          if (this->address_go_ > 0 and
              this->address_go_ < machine_->getMemorySize()) {
            memory_panel_.set_base_address(this->address_go_);
          } else {
            if (machine_)
              memory_panel_.set_base_address(machine_->getMemorySize() - 18);
          }
          this->address_go_ = 0;
          layout_config_.follow_pc = false;
          return true;
        case 'q':
          std::exit(0);
      }
    }

    // Setas do teclado
    if (event == Event::ArrowUp) {
      memory_panel_.scroll_up(1);
      layout_config_.follow_pc = false;
      return true;
    }
    if (event == Event::ArrowDown) {
      memory_panel_.scroll_down(1);
      layout_config_.follow_pc = false;
      return true;
    }

      // Ctrl+Arrow: scroll registradores
      if (event == Event::ArrowUpCtrl) {
        registers_panel_.scroll_up(1);
        return true;
      }
      if (event == Event::ArrowDownCtrl) {
        registers_panel_.scroll_down(1);
        return true;
      }
    return false;  // Evento não tratado
  });

  // ============================================================================
  // LOOP PRINCIPAL DO FTXUI
  // ============================================================================
  screen.Loop(main_component);

  // ============================================================================
  // CLEANUP: Garante que a thread de timer termine
  // ============================================================================
  timer_active.store(false);
  if (timer_thread.joinable()) {
    timer_thread.join();
  }
}

Element TuiApp::render() {
  memory_panel_.set_config({.visible_rows = 18,
                            .show_hex = layout_config_.show_hex,
                            .show_decimal = true,
                            .show_disassembly = true,
                            .follow_pc = layout_config_.follow_pc});
  registers_panel_.set_config({.show_hex = layout_config_.show_hex, .machine_type = machine_type_});
  registers_panel_.set_visible_rows(18);

  return vbox(Elements{
      render_header(),
      hbox(Elements{
          memory_panel_.render() | flex,
          registers_panel_.render() |
              size(WIDTH, EQUAL, layout_config_.registers_panel_width)}) |
          flex,
      output_panel_.render() |
          size(HEIGHT, EQUAL, layout_config_.output_panel_height),
      render_controls_panel(), render_status_bar()});
}

Element TuiApp::render_header() {
  std::string title = "HIDRA-TUI";
  std::string file_info =
      current_filepath_.empty() ? "[sem arquivo]" : current_filepath_;
  if (file_modified_) file_info += " *";
  if (running_) title += " [▶ RODANDO]";
  return hidra::tui::compose_header(title, machine_type_, file_info, running_,
                                    file_modified_);
}

Element TuiApp::render_status_bar() {
  return hidra::tui::compose_status_bar(
      layout_config_.show_hex ? "HEX" : "DEC",
      file_modified_ ? "MODIFICADO (auto-recarregando)" : "sincronizado");
}

Element TuiApp::render_controls_panel() {
  auto toggle_style = [](bool active) -> Decorator {
    return active ? color(Color::Green) : (color(Color::White) | dim);
  };

  return hbox(Elements{
             text("[B]uild ") | color(Color::Cyan),
             text("[S]tep ") | color(Color::Cyan),
             text("[R]un/Stop ") | color(Color::Cyan),
             text("[↑/↓]Scroll ") | color(Color::Yellow),
             text("[g/G]Top/Bot ") | color(Color::Yellow),
             text("[H]ex ") | toggle_style(layout_config_.show_hex),
             text("[F]ollowPC ") | toggle_style(layout_config_.follow_pc),
             text("[Q]uit") | color(Color::Red) | bold,
         }) |
         border;
}
