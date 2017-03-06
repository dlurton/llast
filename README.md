

Derived from these instructions:  http://llvm.org/docs/CMake.html

To get llvm installed on your linux box, execute the following commands:

    wget http://releases.llvm.org/3.9.1/llvm-3.9.1.src.tar.xz
    tar xvf llvm-3.9.1.src.tar.xz
    cd llvm-3.9.1.src/
    mkdir build
    cd build
    cmake ..
    cmake --build .  -- -j <num cpu cores>

Go make yourself some coffee, etc, blah blah.  Then, continue with:

    cmake --build . --target install

