#pragma once
#include "core/machine.h"
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

namespace hidra::tui::panels {

struct MemoryPanelConfig {
    int visible_rows = 16;
    bool show_hex = false;  // Padrão: decimal
    bool show_decimal = true;
    bool show_disassembly = true;
    bool follow_pc = true;
    bool show_source = true;  // Mostrar código fonte
    int base_address = 0;
};

class MemoryPanel {
public:
    explicit MemoryPanel(const MemoryPanelConfig& config = MemoryPanelConfig{});
    
    void set_machine(Machine* machine) { machine_ = machine; }
    void set_config(const MemoryPanelConfig& config) { config_ = config; }
    
    ftxui::Element render() const;
    
    void scroll_up(int lines = 1);
    void scroll_down(int lines = 1);
    void center_on_pc();
    
    int get_base_address() const { return base_address_; }
    void set_base_address(int addr) { base_address_ = addr; }
    
    // Carregar código fonte
    void set_source_lines(const std::vector<std::string>& lines) { source_lines_ = lines; }

private:
    Machine* machine_ = nullptr;
    MemoryPanelConfig config_;
    int base_address_ = 0;
    std::vector<std::string> source_lines_;  // Linhas do código fonte
    
    ftxui::Element render_row(int address, bool is_pc) const;
    std::string format_value(int value, bool hex) const;
    std::string get_instruction_text(int address) const;
    std::string get_source_line(int address) const;
};

} // namespace hidra::tui::panels
