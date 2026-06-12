#include "goto_popup.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

#include <ftxui/dom/canvas.hpp>

#include "../layout.h"

using namespace ftxui;

namespace {

const int TEXT_BASE = 0x400000;
const int DATA_BASE = 0x10000000;

bool is_hex_string(const std::string& s) {
    if (s.size() >= 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        return std::all_of(s.begin() + 2, s.end(),
            [](char c) { return std::isxdigit(c); });
    }
    return false;
}

bool is_number(const std::string& s) {
    return !s.empty() && std::all_of(s.begin(), s.end(),
        [](char c) { return std::isdigit(c); });
}

std::optional<int> parse_hex(const std::string& s) {
    try {
        size_t pos;
        int value = std::stoi(s, &pos, 16);
        if (pos == s.size()) return value;
    } catch (...) {}
    return std::nullopt;
}

std::optional<int> parse_decimal(const std::string& s) {
    try {
        size_t pos;
        int value = std::stoi(s, &pos, 10);
        if (pos == s.size()) return value;
    } catch (...) {}
    return std::nullopt;
}

std::string to_hex_string(int value) {
    std::ostringstream oss;
    oss << "0x" << std::uppercase << std::hex << value;
    return oss.str();
}

}  // namespace

namespace hidra::tui::panels {

//////////////////////////////////////////////////
// GotoPopup
//////////////////////////////////////////////////

GotoPopup::GotoPopup() {
    update_quick_jumps();
}

void GotoPopup::set_machine(Machine* machine) {
    machine_ = machine;
    update_quick_jumps();
    update_suggestions();
}

void GotoPopup::open() {
    active_ = true;
    input_.clear();
    suggestions_.clear();
    selected_suggestion_ = 0;
    selected_quickjump_ = -1;
    if (machine_) {
        update_quick_jumps();
    }
}

void GotoPopup::close() {
    active_ = false;
    input_.clear();
    suggestions_.clear();
}

void GotoPopup::update_quick_jumps() {
    quick_jumps_.clear();
    if (!machine_) return;

    auto add_jump = [this](const std::string& name, int addr) {
        quick_jumps_.push_back({name, addr});
    };

    add_jump("text", TEXT_BASE);
    add_jump("data", DATA_BASE);
    add_jump("pc", machine_->getPCValue());
    add_jump("sp", machine_->getRegisterValue("sp"));
    add_jump("gp", machine_->getRegisterValue("gp"));
    add_jump("ra", machine_->getRegisterValue("ra"));
}

void GotoPopup::update_suggestions() {
    suggestions_.clear();
    if (!machine_ || input_.empty()) return;

    QString input_q = QString::fromStdString(input_).toLower();
    QStringList all_labels = machine_->getAllLabels();

    for (const QString& label : all_labels) {
        if (label.toLower().startsWith(input_q)) {
            int addr = machine_->getLabelAddress(label);
            if (addr >= 0) {
                suggestions_.push_back({label, addr});
            }
        }
    }

    selected_suggestion_ = 0;
}

int GotoPopup::find_label(const QString& label) const {
    if (!machine_) return -1;
    return machine_->getLabelAddress(label);
}

std::optional<int> GotoPopup::parse_input() const {
    if (input_.empty()) return std::nullopt;

    QString input_q = QString::fromStdString(input_).toLower();

    // Special keywords
    if (input_q == ".text") return TEXT_BASE;
    if (input_q == ".data") return DATA_BASE;
    if (input_q == ".pc") return machine_ ? machine_->getPCValue() : 0;
    if (input_q == ".sp") return machine_ ? machine_->getRegisterValue("sp") : 0;
    if (input_q == ".gp") return machine_ ? machine_->getRegisterValue("gp") : 0;
    if (input_q == ".ra") return machine_ ? machine_->getRegisterValue("ra") : 0;

    // Label lookup
    int addr = find_label(input_q);
    if (addr >= 0) return addr;

    // Hex input (0x...)
    if (is_hex_string(input_)) {
        return parse_hex(input_.substr(2));
    }

    // Decimal input
    if (is_number(input_)) {
        return parse_decimal(input_);
    }

    return std::nullopt;
}

std::optional<int> GotoPopup::get_target_address() const {
    // If a suggestion is selected, use it
    if (selected_suggestion_ >= 0 &&
        selected_suggestion_ < static_cast<int>(suggestions_.size())) {
        return suggestions_[selected_suggestion_].second;
    }

    // If a quick jump is selected, use it
    if (selected_quickjump_ >= 0 &&
        selected_quickjump_ < static_cast<int>(quick_jumps_.size())) {
        return quick_jumps_[selected_quickjump_].second;
    }

    // Otherwise parse the input
    return parse_input();
}

void GotoPopup::handle_key(const ftxui::Event& event) {
    if (event == Event::Character('\n') || event == Event::Character('\r')) {
        // Enter - target will be retrieved by get_target_address()
        return;
    }

    if (event == Event::Escape) {
        close();
        return;
    }

    if (event == Event::ArrowUp) {
        if (!suggestions_.empty()) {
            selected_suggestion_ = (selected_suggestion_ - 1 + suggestions_.size()) % suggestions_.size();
        } else if (!quick_jumps_.empty()) {
            if (selected_quickjump_ < 0) selected_quickjump_ = quick_jumps_.size() - 1;
            else selected_quickjump_ = (selected_quickjump_ - 1 + quick_jumps_.size()) % quick_jumps_.size();
        }
        return;
    }

    if (event == Event::ArrowDown) {
        if (!suggestions_.empty()) {
            selected_suggestion_ = (selected_suggestion_ + 1) % suggestions_.size();
        } else if (!quick_jumps_.empty()) {
            if (selected_quickjump_ < 0) selected_quickjump_ = 0;
            else selected_quickjump_ = (selected_quickjump_ + 1) % quick_jumps_.size();
        }
        return;
    }

    if (event == Event::Tab) {
        if (!suggestions_.empty()) {
            selected_suggestion_ = (selected_suggestion_ + 1) % suggestions_.size();
        }
        return;
    }

    if (event == Event::Backspace) {
        if (!input_.empty()) {
            input_.pop_back();
            update_suggestions();
        }
        return;
    }

    if (event == Event::Delete) {
        input_.clear();
        update_suggestions();
        return;
    }

    // Regular character input
    if (event.is_character()) {
        input_ += event.character();
        update_suggestions();
        return;
    }
}

Element GotoPopup::render() const {
    auto input_display = input_.empty() ? "[Digite para buscar...]" : input_;

    Elements content;

    // Input field
    content.push_back(hbox({
        text(" Go to: ") | bold | color(Color::Cyan),
        text("[" + input_display + "_]") | color(Color::Yellow) | dim
    }) | border);

    // Suggestions
    if (!suggestions_.empty()) {
        Elements suggestion_rows;
        for (size_t i = 0; i < suggestions_.size() && i < 5; ++i) {
            const auto& [label, addr] = suggestions_[i];
            std::string addr_str = to_hex_string(addr);
            std::string row_text = "  " + label.toStdString() + "  [" + addr_str + "]";

            if (static_cast<int>(i) == selected_suggestion_) {
                suggestion_rows.push_back(
                    text(row_text) | inverted | bold
                );
            } else {
                suggestion_rows.push_back(
                    text(row_text) | color(Color::White)
                );
            }
        }
        content.push_back(vbox(suggestion_rows) | border);
    }

    // Quick jumps
    if (!quick_jumps_.empty()) {
        Elements jump_elements;
        for (size_t i = 0; i < quick_jumps_.size(); ++i) {
            const auto& [name, addr] = quick_jumps_[i];
            std::string addr_str = to_hex_string(addr);
            std::string label = "[" + name + ": " + addr_str + "]";

            if (static_cast<int>(i) == selected_quickjump_) {
                jump_elements.push_back(text(label) | inverted);
            } else {
                jump_elements.push_back(text(label) | color(Color::Green));
            }
        }
        content.push_back(hbox(jump_elements) | border);
    }

    // Help text
    content.push_back(text(" [Enter] Go   [Tab] Next   [Esc] Cancel ") | dim);

    return vbox(content) | border | bold | color(Color::Cyan);
}

}  // namespace hidra::tui::panels