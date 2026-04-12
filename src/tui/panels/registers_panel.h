#pragma once
#include "core/machine.h"
#include "../machine_config.h"
#include <ftxui/dom/elements.hpp>
#include <vector>
#include <string>

namespace hidra::tui::panels {

struct RegistersPanelConfig {
    bool show_hex = false;
    std::string machine_type;
};

class RegistersPanel {
public:
    explicit RegistersPanel(const RegistersPanelConfig& config = RegistersPanelConfig{});
    
    void set_machine(Machine* machine) { machine_ = machine; }
    void set_config(const RegistersPanelConfig& config) { config_ = config; }
    void set_machine_type(const std::string& type) { config_.machine_type = type; }
    
    ftxui::Element render() const;
    
    void toggle_hex_mode() { config_.show_hex = !config_.show_hex; }
    bool is_hex_mode() const { return config_.show_hex; }

private:
    Machine* machine_ = nullptr;
    RegistersPanelConfig config_;
    
    ftxui::Element render_all_registers() const;
    ftxui::Element render_register(const std::string& name, int value, 
                                   const std::string& description = "") const;
    ftxui::Element render_flags() const;
    ftxui::Element render_counters() const;
    std::string format_value(int value) const;
    std::string get_register_description(const std::string& name) const;
    MachineConfig get_machine_config() const;
};

} // namespace hidra::tui::panels
