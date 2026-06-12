#pragma once
#include <string>
#include <vector>
#include <optional>

#include <ftxui/dom/elements.hpp>
#include <ftxui/component/event.hpp>
#include <QString>
#include <QStringList>

#include "core/machine.h"

namespace ftxui {
class Event;
}

namespace hidra::tui::panels {

class GotoPopup {
public:
    explicit GotoPopup();

    void set_machine(Machine* machine);

    void open();
    void close();
    bool is_active() const { return active_; }

    void handle_key(const ftxui::Event& event);
    std::optional<int> get_target_address() const;
    ftxui::Element render() const;

private:
    void update_suggestions();
    std::optional<int> parse_input() const;
    int find_label(const QString& label) const;
    void update_quick_jumps();

    static int get_pc(Machine* m) { return m ? m->getPCValue() : 0; }
    static int get_sp(Machine* m) { return m ? m->getRegisterValue("sp") : 0; }
    static int get_gp(Machine* m) { return m ? m->getRegisterValue("gp") : 0; }
    static int get_ra(Machine* m) { return m ? m->getRegisterValue("ra") : 0; }

    Machine* machine_ = nullptr;
    bool active_ = false;
    std::string input_;
    std::vector<std::pair<QString, int>> suggestions_;  // label, address
    std::vector<std::pair<std::string, int>> quick_jumps_;  // name, address
    int selected_suggestion_ = 0;
    int selected_quickjump_ = -1;
};

}  // namespace hidra::tui::panels