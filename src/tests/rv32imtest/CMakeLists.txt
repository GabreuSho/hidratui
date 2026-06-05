cmake_minimum_required(VERSION 3.16)
project(test_rv32im LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 5.15 REQUIRED COMPONENTS Core)

add_executable(test_rv32im tst_rv32im.cpp)
target_include_directories(test_rv32im PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_rv32im PRIVATE hidramachines hidracore Qt5::Core)
