# CMakeLists.txt para o módulo RISC-V do Hidra
# Requer Qt5 Core e hidracore já configurados no CMakeLists.txt principal

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

# Habilita MOC do Qt para headers com Q_OBJECT
set_target_properties(hidrariscv PROPERTIES AUTOMOC ON)

# Dependências
target_link_libraries(hidrariscv 
    PUBLIC Qt5::Core
    PRIVATE hidracore
)

# Define include directories
target_include_directories(hidrariscv PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Configurações de compilação
target_compile_features(hidrariscv PUBLIC cxx_std_17)

# Flags de warning para melhor qualidade de código
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(hidrariscv PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Define macro para builds condicionais
target_compile_definitions(hidrariscv PUBLIC RISCV_ENABLED)
