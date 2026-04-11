// src/main_tui.cpp
#include "tui/tui_app.h"
#include <QCoreApplication> // Para inicializar Qt Core sem GUI
#include <QString>
#include <iostream>
#include <string>

void print_usage(const char *program) {
  std::cout << "Uso: " << program << " [máquina] [arquivo.asm]\n"
            << "Máquinas: neander, ahmes, ramses, reg, volta, pericles\n"
            << "Exemplo: " << program << " ramses programa.rad\n";
}

int main(int argc, char *argv[]) {
  // Inicializa Qt Core (necessário para QString, sem iniciar loop gráfico)
  QCoreApplication qt_app(argc, argv);

  if (argc < 3) {
    print_usage(argv[0]);
    return 1;
  }

  std::string machine_type = argv[1];
  std::string filepath = argv[2];
  for (auto &c : machine_type)
    c = std::tolower(c);

  try {
    TuiApp app(machine_type);
    app.load_file(filepath);
    app.run();
    return 0;

  } catch (const QString &e) {
    // ✅ Captura exceções Qt antes que abortem o programa
    std::cerr << "\n⚠️  Aviso: " << e.toStdString() << "\n";
    std::cerr << "Dica: Esta máquina pode não suportar todos os recursos.\n";
    return 1;

  } catch (const std::exception &e) {
    std::cerr << "Erro: " << e.what() << "\n";
    return 1;

  } catch (...) {
    std::cerr << "Erro desconhecido\n";
    return 1;
  }
}
