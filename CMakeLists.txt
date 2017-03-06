cmake_minimum_required(VERSION 3.6)
project(llast)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS_DEBUG "-DLLAST_DEBUG")

# http://llvm.org/docs/CMake.html
# (See the heading "Embedding LLVM in your project")
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
# Find the libraries that correspond to the LLVM components that we wish to use
llvm_map_components_to_libnames(llvm_libs support core irreader)



set(SOURCE_FILES main.cpp exceptions.hpp ast.hpp visitor.hpp pretty.hpp ast.cpp compiler.hpp compiler.cpp)
add_executable(llast ${SOURCE_FILES})
target_link_libraries(llast ${llvm_libs})


