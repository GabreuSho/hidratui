# CMakeLists.txt para Testes de Simulação RISC-V

find_package(Qt5Test REQUIRED)

add_executable(TestRiscV
    tst_riscv.cpp
    ../simulationtestcase.h
)

target_link_libraries(TestRiscV PRIVATE Qt5::Test)
target_link_libraries(TestRiscV PRIVATE hidramachines hidrariscv)

target_include_directories(
    TestRiscV
    PUBLIC ../../../gui
    PUBLIC ../../../core
    PUBLIC ../../../machines
    PUBLIC ../../../riscv
    PUBLIC ../../..
    PUBLIC ..
)

add_test(NAME TestRiscV COMMAND TestRiscV)
