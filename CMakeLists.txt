cmake_minimum_required(VERSION 3.6)
project(llast)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra -ggdb -fno-omit-frame-pointer -O0 -fno-optimize-sibling-calls -fsanitize=address -fstack-protector-all")

#set(CMAKE_CXX_FLAGS_DEBUG "-DLLAST_DEBUG")

# http://llvm.org/docs/CMake.html

# (See the heading "Embedding LLVM in your project")
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
#TODO: make this work - is it even possible when linking with LLVM's .so?
#llvm_map_components_to_libnames(${llvm_libs} support core irreader mcjit codegen native nativecodegen xcore)

set(SOURCE_FILES Exception.hpp AST.hpp ExpressionTreeVisitor.hpp PrettyPrinter.hpp AST.cpp ExprRunner.hpp ExprRunner.cpp tests.cpp ExprRunner.hpp)
add_library(llast ${SOURCE_FILES})

target_link_libraries(llast LLVM-4.0)
#TODO: make this work - is it even possible when linking with LLVM's .so?
#target_link_libraries(llast ${llvm_libs})

add_executable(demo SigHandler.cpp SigHandler.hpp)

target_link_libraries(demo llast ${llvm_libs})


# https://cmake.org/cmake/help/v3.6/module/FindGTest.html

enable_testing()

add_executable(tests tests.cpp SigHandler.cpp SigHandler.hpp)
target_link_libraries(tests llast)