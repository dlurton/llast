cmake_minimum_required(VERSION 3.6)
project(llast)

set(CMAKE_CXX_STANDARD 14)

link_directories(../externs/llvm/lib/)
file(GLOB LLVM_LIBS ../externs/llvm/lib/*.lib ../externs/llvm/lib/lib*)

# -fsanitize=address 
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra -ggdb -fno-omit-frame-pointer -O0 -fno-optimize-sibling-calls -fstack-protector-all")


set(SOURCE_FILES Exception.hpp AST.hpp ExpressionTreeVisitor.hpp PrettyPrinter.hpp AST.cpp ExprRunner.hpp ExprRunner.cpp tests.cpp ExprRunner.hpp)

add_library(llast ${SOURCE_FILES})

target_link_libraries(llast ${LLVM_LIBS})

add_executable(demo SigHandler.cpp SigHandler.hpp)

target_link_libraries(demo llast ${LLVM_LIBS})


enable_testing()

add_executable(tests tests.cpp SigHandler.cpp SigHandler.hpp)
target_link_libraries(tests llast)


include_directories(../externs/llvm/include)