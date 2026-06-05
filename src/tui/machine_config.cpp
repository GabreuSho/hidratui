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
    if (type == "rv32im" || type == "rvm")
        return createRV32IMConfig();

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

MachineConfig MachineConfigProvider::createRV32IMConfig() {
    MachineConfig config;
    config.name = "RV32IM";
    // 32 registers with ABI names + PC
    config.registers = {
        "zero", "ra", "sp", "gp", "tp",
        "t0", "t1", "t2", "s0/fp", "s1",
        "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
        "s2", "s3", "s4", "s5", "s6", "s7",
        "s8", "s9", "s10", "s11",
        "t3", "t4", "t5", "t6",
        "PC"
    };
    config.flags = {}; // RISC-V doesn't have explicit condition flags
    config.register_descriptions = {
        {"zero", "Hardwired Zero"},
        {"ra", "Return Address"},
        {"sp", "Stack Pointer"},
        {"gp", "Global Pointer"},
        {"tp", "Thread Pointer"},
        {"t0", "Temporary 0"},
        {"t1", "Temporary 1"},
        {"t2", "Temporary 2"},
        {"s0/fp", "Saved/Frame Pointer"},
        {"s1", "Saved 1"},
        {"a0", "Argument 0"},
        {"a1", "Argument 1"},
        {"a2", "Argument 2"},
        {"a3", "Argument 3"},
        {"a4", "Argument 4"},
        {"a5", "Argument 5"},
        {"a6", "Argument 6"},
        {"a7", "Argument 7"},
        {"s2", "Saved 2"},
        {"s3", "Saved 3"},
        {"s4", "Saved 4"},
        {"s5", "Saved 5"},
        {"s6", "Saved 6"},
        {"s7", "Saved 7"},
        {"s8", "Saved 8"},
        {"s9", "Saved 9"},
        {"s10", "Saved 10"},
        {"s11", "Saved 11"},
        {"t3", "Temporary 3"},
        {"t4", "Temporary 4"},
        {"t5", "Temporary 5"},
        {"t6", "Temporary 6"},
        {"PC", "Program Counter"}
    };
    return config;
}

} // namespace hidra::tui
