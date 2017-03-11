cmake_minimum_required(VERSION 3.6)
project(llast)

set(CMAKE_CXX_STANDARD 14)

#set(CMAKE_CXX_FLAGS_DEBUG "-DLLAST_DEBUG")

# http://llvm.org/docs/CMake.html

# (See the heading "Embedding LLVM in your project")
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Find the libraries that correspond to the LLVM components that we wish to use
# TODO:  don't link against all llvm libs.  execute command `llvm-config --components' to get list and somehow
# (trial and error?) whiddle down that list to nothing but the libs we really need
# llvm_map_components_to_libnames(llvm_libs all)

set(SOURCE_FILES Exception.hpp Expr.hpp ExpressionTreeVisitor.hpp PrettyPrinter.hpp Expr.cpp EmbryonicCompiler.hpp EmbryonicCompiler.cpp tests.cpp)
add_library(llast ${SOURCE_FILES} )
target_link_libraries(llast LLVM-3.9 ${llvm_libs})

add_executable(demo demo.cpp)

target_link_libraries(demo llast)



# https://cmake.org/cmake/help/v3.6/module/FindGTest.html

enable_testing()
find_package(GTest REQUIRED)

add_executable(tests tests.cpp)
target_link_libraries(tests llast gtest gtest_main)