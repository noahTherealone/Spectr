cmake_minimum_required(VERSION 3.20)

project(
    Spectr
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(spectr
    src/main.cpp
    src/compiler.cpp
    src/lexer.cpp
    src/type.cpp
    src/expression.cpp
    src/statement.cpp
    src/parser.cpp
    src/name_resolution.cpp
    src/type_checker.cpp
)
