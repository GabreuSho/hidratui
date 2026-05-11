# CMakeLists.txt para Testes RISC-V

cmake_minimum_required(VERSION 3.16)
project(RiscVTests LANGUAGES CXX)

# Requer C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Encontrar Qt5
find_package(Qt5 COMPONENTS Core Test REQUIRED)

# Diretório de includes
include_directories(
    ${CMAKE_SOURCE_DIR}/../../src/riscv
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Test_INCLUDE_DIRS}
)

# Arquivos de teste
set(TEST_SOURCES
    riscv_tests.cpp
)

# Criar executável de testes
add_executable(riscv_tests ${TEST_SOURCES})

# Linkar bibliotecas
target_link_libraries(riscv_tests
    Qt5::Core
    Qt5::Test
    hidrariscv  # Biblioteca RISC-V do Hidra
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

# Opções de build
option(RISCV_TESTS_VERBOSE "Verbose test output" OFF)

if(RISCV_TESTS_VERBOSE)
    target_compile_definitions(riscv_tests PRIVATE QT_TESTLIB_VERBOSITY=2)
endif()

# Flags de compilação adicionais (opcionais)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(riscv_tests PRIVATE -g -O0)
endif()
