# CMakeLists.txt para Testes RISC-V do Hidra
# Requer que hidrariscv já tenha sido compilada

cmake_minimum_required(VERSION 3.16)

# Não criar novo projeto, usar o contexto do CMakeLists.txt principal
# project(RiscVTests LANGUAGES CXX)

# Encontrar Qt5 Core e Test
find_package(Qt5 COMPONENTS Core Test REQUIRED)

# Arquivos de teste
set(TEST_SOURCES
    riscv_tests.cpp
)

# Criar executável de testes
add_executable(riscv_tests ${TEST_SOURCES})

# Linkar bibliotecas
target_link_libraries(riscv_tests
    PRIVATE
        Qt5::Core
        Qt5::Test
        hidrariscv  # Biblioteca RISC-V do Hidra
)

# Include directories
target_include_directories(riscv_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/riscv
    ${CMAKE_SOURCE_DIR}/src
)

# Habilitar testes com CTest
enable_testing()

# Adicionar teste ao CTest
add_test(NAME RiscVRegisterFileTests 
         COMMAND riscv_tests RiscVRegisterFileTests)

add_test(NAME RiscVDecoderTests 
         COMMAND riscv_tests RiscVDecoderTests)

add_test(NAME RiscVALUTests 
         COMMAND riscv_tests RiscVALUTests)

add_test(NAME RiscVCPUTests 
         COMMAND riscv_tests RiscVCPUTests)

# Configurações de compilação
target_compile_features(riscv_tests PRIVATE cxx_std_17)

# Flags de warning
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(riscv_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Opção para output verboso
option(RISCV_TESTS_VERBOSE "Verbose test output" OFF)
if(RISCV_TESTS_VERBOSE)
    target_compile_definitions(riscv_tests PRIVATE QT_TESTLIB_VERBOSITY=2)
endif()

# Debug flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(riscv_tests PRIVATE -g -O0)
endif()
