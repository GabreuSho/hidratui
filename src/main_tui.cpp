// src/main_tui.cpp
#include <iostream>
#include <string>

#include "tui/tui_app.h"

void print_usage(const char* program) {
  std::cout << "Uso: " << program << " [máquina] [arquivo.asm]\n"
            << "Máquinas suportadas: neander, ahmes, ramses, reg, volta\n"
            << "Exemplo: " << program << " neander programa.ned\n";
}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    print_usage(argv[0]);
    return 1;
  }

  std::string machine_type = argv[1];
  std::string filepath = argv[2];

  // Converter para minúsculo
  for (auto& c : machine_type) c = std::tolower(c);

  try {
    TuiApp app(machine_type);
    app.load_file(filepath);
    app.run();
  } catch (const std::exception& e) {
    std::cerr << "Erro: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
