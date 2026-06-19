#pragma once

#include <string>
#include <vector>

#include <ftxui/dom/elements.hpp>
#include <ftxui/component/event.hpp>

namespace ftxui {
class Event;
}

namespace hidra::tui::panels {

struct HelpSection {
    std::string title;
    std::vector<std::pair<std::string, std::string>> items;
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
    void navigate_section(int delta);
    void ensure_selection_visible();
    ftxui::Element render_section(const HelpSection& section, bool selected) const;
    ftxui::Element render_section_compact(const HelpSection& section, bool selected) const;
};

}  // namespace hidra::tui::panels