#pragma once
#include <string>
#include <vector>
#include <map>

namespace hidra::tui {

struct MachineConfig {
    std::string name;
    std::vector<std::string> registers;
    std::vector<std::string> flags;
    std::map<std::string, std::string> register_descriptions;
};

class MachineConfigProvider {
public:
    static MachineConfig getConfig(const std::string& machine_type);
    
private:
    static MachineConfig createNeanderConfig();
    static MachineConfig createAhmesConfig();
    static MachineConfig createRamsesConfig();
    static MachineConfig createRegConfig();
    static MachineConfig createVoltaConfig();
    static MachineConfig createPericlesConfig();
    static MachineConfig createPitagorasConfig();
    static MachineConfig createCromagConfig();
    static MachineConfig createQueopsConfig();
static MachineConfig createRV32IMConfig();
};

} // namespace hidra::tui
