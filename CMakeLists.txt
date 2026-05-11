cmake_minimum_required(VERSION 3.16)
project(hidra-tui VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============ QT5 CORE (obrigatório para QRegExp) ============
set(CMAKE_AUTOMOC ON)
find_package(Qt5 5.15 REQUIRED COMPONENTS Core Test)
set(QT_TARGET Qt5::Core)

# ============ CORE LIBRARY ============
file(GLOB CORE_SOURCES src/core/*.cpp)
add_library(hidracore STATIC ${CORE_SOURCES})
target_include_directories(hidracore PUBLIC src)
target_link_libraries(hidracore PUBLIC ${QT_TARGET})

# ============ MACHINES LIBRARY ============
file(GLOB MACHINES_SOURCES src/machines/*.cpp)
add_library(hidramachines STATIC ${MACHINES_SOURCES})
target_include_directories(hidramachines PUBLIC src)
target_link_libraries(hidramachines PUBLIC hidracore ${QT_TARGET})

# ============ RISC-V MODULE ============
option(ENABLE_RISCV "Enable RISC-V support" ON)
if(ENABLE_RISCV)
    add_subdirectory(src/riscv)
    # hidramachines precisa acessar classes RISC-V se houver integração
    target_link_libraries(hidramachines PUBLIC hidrariscv)
endif()

# ============ FTXUI (opcional para TUI) ============
find_package(ftxui COMPONENTS screen dom component QUIET)
if(NOT ftxui_FOUND)
    message(STATUS "FTXUI not found - building core library and tests only")
endif()

# ============ TUI LIBRARY (se FTXUI disponível) ============
if(ftxui_FOUND)
    file(GLOB TUI_SOURCES src/tui/*.cpp src/tui/panels/*.cpp)
    add_library(hidratui STATIC ${TUI_SOURCES})
    target_include_directories(hidratui PUBLIC src src/tui)
    target_link_libraries(hidratui PUBLIC 
        hidramachines 
        ftxui::screen
        ftxui::dom
        ftxui::component
    )
    
    # Adiciona pthread no Linux
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_libraries(hidratui PRIVATE pthread)
    endif()
    
    # ============ EXECUTABLE ============
    add_executable(hidra-tui src/main_tui.cpp)
    target_link_libraries(hidra-tui PRIVATE hidratui)
else()
    message(STATUS "Skipping TUI executable - FTXUI not available")
endif()

# ============ TESTES ============
enable_testing()
add_subdirectory(src/tests)

# Testes RISC-V (se habilitado)
if(ENABLE_RISCV)
    add_subdirectory(tests/riscv)
endif()
