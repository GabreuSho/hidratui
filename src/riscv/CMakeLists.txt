# CMakeLists.txt para o módulo RISC-V do Hidra
# Adicione este subdiretório ao CMakeLists.txt principal

# Coleta arquivos fonte RISC-V
set(RISCV_SOURCES
    riscv_register_file.cpp
    riscv_decoder.cpp
    riscv_alu.cpp
    riscv_machine.cpp
)

set(RISCV_HEADERS
    riscv_register_file.h
    riscv_decoder.h
    riscv_alu.h
    riscv_machine.h
)

# Cria biblioteca estática para o núcleo RISC-V
add_library(hidrariscv STATIC ${RISCV_SOURCES} ${RISCV_HEADERS})

# Dependências
target_link_libraries(hidrariscv 
    Qt5::Widgets
    hidracore
)

# Define include directories
target_include_directories(hidrariscv PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Configurações de compilação
target_compile_features(hidrariscv PUBLIC cxx_std_17)

# Opcional: define flag para habilitar/desabilitar suporte RISC-V
option(ENABLE_RISCV "Enable RISC-V support" ON)
if(ENABLE_RISCV)
    target_compile_definitions(hidrariscv PUBLIC RISCV_ENABLED)
endif()
