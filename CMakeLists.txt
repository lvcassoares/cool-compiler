cmake_minimum_required(VERSION 3.10)
project(coolc CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(coolc
    src/Lexer.cpp
    src/Parser.cpp
    src/SemanticAnalyzer.cpp
    src/main.cpp
)

target_include_directories(coolc PRIVATE include)

if(MSVC)
    target_compile_options(coolc PRIVATE /W4)
else()
    target_compile_options(coolc PRIVATE -Wall -Wextra)
endif()
