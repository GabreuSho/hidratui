#pragma once

#include <string>
#include <vector>
#include <optional>

#include <ftxui/dom/elements.hpp>
#include <ftxui/component/event.hpp>

namespace ftxui {
class Event;
}

namespace hidra::tui::panels {

struct HelpSection {
    std::string title;
    std::vector<std::pair<std::string, std::string>> items;  // key, description
};

class HelpPopup {
public:
    explicit HelpPopup();

    void set_machine_type(const std::string& machine_type);
    void open();
    void close();
    bool is_active() const { return active_; }

    void handle_key(const ftxui::Event& event);
    ftxui::Element render() const;

private:
    std::string machine_type_;
    std::vector<HelpSection> sections_;
    int scroll_offset_ = 0;
    int selected_section_ = 0;
    bool active_ = false;

    void build_sections();
    static std::vector<HelpSection> build_neander_sections();
    static std::vector<HelpSection> build_ahmes_sections();
    static std::vector<HelpSection> build_ramses_sections();
    static std::vector<HelpSection> build_reg_sections();
    static std::vector<HelpSection> build_volta_sections();
    static std::vector<HelpSection> build_pitagoras_sections();
    static std::vector<HelpSection> build_cromag_sections();
    static std::vector<HelpSection> build_pericles_sections();
    static std::vector<HelpSection> build_queops_sections();
    static std::vector<HelpSection> build_rv32im_sections();
    static std::vector<HelpSection> build_generic_sections();
};

}  // namespace hidra::tui::panels