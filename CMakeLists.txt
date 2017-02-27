cmake_minimum_required(VERSION 3.6)
project(llast)

set(CMAKE_CXX_STANDARD 14)

set(SOURCE_FILES main.cpp exceptions.hpp ast.hpp visitor.hpp context.hpp pretty.hpp)
add_executable(llast ${SOURCE_FILES})