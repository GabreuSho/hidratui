#include "machine_config.h"

namespace hidra::tui {

MachineConfig
MachineConfigProvider::getConfig(const std::string &machine_type) {
  std::string type = machine_type;
  // Converter para minúsculo
  for (auto &c : type)
    c = std::tolower(c);

  if (type == "neander" || type == "ndr")
    return createNeanderConfig();
  if (type == "ahmes" || type == "ahm")
    return createAhmesConfig();
  if (type == "ramses" || type == "rms")
    return createRamsesConfig();
  if (type == "reg")
    return createRegConfig();
  if (type == "volta" || type == "vlt")
    return createVoltaConfig();
  if (type == "pericles" || type == "prc")
    return createPericlesConfig();
  if (type == "pitagoras" || type == "ptg")
    return createPitagorasConfig();
  if (type == "cromag" || type == "crm")
    return createCromagConfig();
  if (type == "queops" || type == "qps")
    return createQueopsConfig();

  // Configuração padrão vazia
  return MachineConfig{};
}

MachineConfig MachineConfigProvider::createNeanderConfig() {
  MachineConfig config;
  config.name = "Neander";
  config.registers = {"AC", "PC"};
  config.flags = {"N", "Z"};
  config.register_descriptions = {{"AC", "Acumulador"},
                                  {"PC", "Program Counter"}};
  return config;
}

MachineConfig MachineConfigProvider::createAhmesConfig() {
  MachineConfig config;
  config.name = "Ahmes";
  config.registers = {"AC", "PC"};
  config.flags = {"N", "Z", "V", "C", "B"};
  config.register_descriptions = {{"AC", "Acumulador"},
                                  {"PC", "Program Counter"}};
  return config;
}

MachineConfig MachineConfigProvider::createRamsesConfig() {
  MachineConfig config;
  config.name = "Ramses";
  config.registers = {"A", "B", "X", "PC"};
  config.flags = {"N", "Z", "C"};
  config.register_descriptions = {{"A", "Acumulador A"},
                                  {"B", "Acumulador B"},
                                  {"X", "Registrador Índice"},
                                  {"PC", "Program Counter"}};
  return config;
}

MachineConfig MachineConfigProvider::createRegConfig() {
  MachineConfig config;
  config.name = "REG";
  // R0 a R63
  for (int i = 0; i < 64; i++) {
    config.registers.push_back("R" + std::to_string(i));
  }
  config.registers.push_back("PC");
  config.flags = {}; // REG não tem flags
  config.register_descriptions = {{"PC", "Program Counter"}};
  // Adicionar descrições para R0-R63
  for (int i = 0; i < 64; i++) {
    config.register_descriptions["R" + std::to_string(i)] =
        "Registrador " + std::to_string(i);
  }
  return config;
}

MachineConfig MachineConfigProvider::createVoltaConfig() {
  MachineConfig config;
  config.name = "Volta";
  config.registers = {"PC", "SP"};
  config.flags = {}; // Volta não tem flags
  config.register_descriptions = {{"PC", "Program Counter"},
                                  {"SP", "Stack Pointer"}};
  return config;
}

MachineConfig MachineConfigProvider::createPericlesConfig() {
  MachineConfig config;
  config.name = "Pericles";
  config.registers = {"A", "B", "X", "PC"};
  config.flags = {"N", "Z", "C"};
  config.register_descriptions = {{"A", "Acumulador A"},
                                  {"B", "Acumulador B"},
                                  {"X", "Registrador Índice"},
                                  {"PC", "Program Counter (12 bits)"}};
  return config;
}

MachineConfig MachineConfigProvider::createPitagorasConfig() {
  MachineConfig config;
  config.name = "Pitagoras";
  config.registers = {"A", "PC"};
  config.flags = {"N", "Z", "C"};
  config.register_descriptions = {{"A", "Acumulador"},
                                  {"PC", "Program Counter"}};
  return config;
}

MachineConfig MachineConfigProvider::createCromagConfig() {
  MachineConfig config;
  config.name = "Cromag";
  config.registers = {"A", "PC"};
  config.flags = {"N", "Z", "C"};
  config.register_descriptions = {{"A", "Acumulador"},
                                  {"PC", "Program Counter"}};
  return config;
}

MachineConfig MachineConfigProvider::createQueopsConfig() {
  MachineConfig config;
  config.name = "Queops";
  config.registers = {"A", "PC"};
  config.flags = {"N", "Z", "C"};
  config.register_descriptions = {{"A", "Acumulador"},
                                  {"PC", "Program Counter"}};
  return config;
}

} // namespace hidra::tui
