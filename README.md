Currently, `llast` only has a very limited set of AST nodes and it can only pretty print them, and generate basic 
LLVM IL.  Eventually, this will be expanded to a complete set of generic AST nodes suitable for generating code
for many different languages.

`llast` should be thought of as a vastly simplified wrapper around LLVM (eventually) allowing run-time code generation 
and JIT compilation.  


This code snippet:

    auto var1 = new Variable("var1", DataType::Int32);
    auto var2 = new Variable("var2", DataType::Int32);

    BlockBuilder bb;
    std::unique_ptr<Block const> blockExpr{
            //int var1;
            bb.addVariable(var1)
            //int var2;
            ->addVariable(var2)
            //var1 = 12;
            ->addExpression(new AssignVariable(var1, new LiteralInt32(12)))
            //var2 = var1 * 6;
            ->addExpression(new AssignVariable(var2, new Binary(new VariableRef(var1), OperationKind::Mul, new LiteralInt32(6))))
            ->build()
    };

    // Pretty print the AST
    PrettyPrinterVisitor visitor(std::cout);
    ExpressionTreeWalker walker(&visitor);
    walker.walkModule(blockExpr.get());

    //Generate LLVM IL and dump to stdout
    ExprRunner::compileEmbryonically(blockExpr.get());

Example output:
     
    Block:(var1:Int32, var2:Int32)
        AssignVariable: var1
            LiteralInt32: 12
        AssignVariable: var2
            Binary: Mul
                LiteralInt32: 6
                VariableRef: var1
    LLVM IL:
    ; ModuleID = 'EmbryonicModule'
    source_filename = "EmbryonicModule"
    
    define void @someFunc() {
    someEntry:
      %var2 = alloca i32
      %var1 = alloca i32
      store i32 12, i32* %var1
      %0 = load i32, i32* %var1
      %1 = mul i32 6, %0
      store i32 %1, i32* %var2
    }

To get llvm installed on your linux box, execute the following commands:

    wget http://releases.llvm.org/4.0.0/llvm-4.0.0.src.tar.xz
    tar xvf llvm-4.0.0.src.tar.xz
    cd llvm-4.0.0.src/
    mkdir build
    cd build
    cmake .. -DLLVM_BUILD_LLVM_DYLIB:BOOL=ON -DLLVM_ENABLE_RTTI:BOOL=ON -DLLVM_ENABLE_EH:BOOL=ON -DLLVM_USE_SANITIZER:STRING=Address -DLLVM_ENABLE_ASSERTIONS:BOOL=ON -DLLVM_ENABLE_EXPENSIVE_CHECKS:BOOL=ON -DLLVM_BUILD_TOOLS:BOOL=OFF -DLLVM_BUILD_EXAMPLES:BOOL=OFF
    cmake .. -DLLVM_BUILD_LLVM_DYLIB:BOOL=ON -DLLVM_ENABLE_RTTI:BOOL=ON -DLLVM_ENABLE_EH:BOOL=ON
     

    cmake --build .  -- -j <num cpu cores>

Go make yourself some coffee, etc, blah blah.  Then, continue with:

    cmake --build . --target install

Also see: http://llvm.org/docs/CMake.html

#### TO DO:

 - Attempting to return a return causes SIGSEV.  This should either be an error or succeed.
 
 #### Facts and other notes that should one day be a part of the documentation:
 
  - Destroying any node also destroys all of its descendants.
  - Can't find libLLVM-4.0.so?  http://stackoverflow.com/questions/17889799/libraries-in-usr-local-lib-not-found
  