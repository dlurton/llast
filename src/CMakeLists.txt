cmake_minimum_required(VERSION 3.6)

project(llast)
set(CMAKE_CXX_STANDARD 14)

# Yes to link with the LLVM dynamic lib otherwise link with static libs.
set(link_llvm_dylib yes)

link_directories(../externs/llvm/lib/)

# -fsanitize=address
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS1} -Wall -pedantic -Wextra -ggdb -fno-omit-frame-pointer -O0 -fno-optimize-sibling-calls -fstack-protector-all")
set(SOURCE_FILES Exception.hpp AST.hpp ExpressionTreeVisitor.hpp PrettyPrinter.hpp AST.cpp ExprRunner.hpp ExprRunner.cpp tests.cpp ExprRunner.hpp)

#LLVM stuff
set(LLVM_DIR ../externs/llvm/lib/cmake/llvm)
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

if(${link_llvm_dylib})
    set(llvm_libs LLVM-4.0)
else()
    llvm_map_components_to_libnames(llvm_libs all)
endif()

#LLAST targets:
add_library(llast ${SOURCE_FILES})
target_link_libraries(llast ${llvm_libs})

add_executable(demo SigHandler.cpp SigHandler.hpp)
target_link_libraries(demo llast ${llvm_libs})

enable_testing()
add_executable(tests tests.cpp SigHandler.cpp SigHandler.hpp)
target_link_libraries(tests llast)



