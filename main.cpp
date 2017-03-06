#include <iostream>
#include <vector>
#include <memory>

#include "visitor.hpp"
#include "pretty.hpp"
#include "compiler.hpp"

//https://github.com/IronLanguages/dlr/tree/master/Src/Microsoft.Scripting.Core/Ast
//http://www.stack.nl/~dimitri/doxygen/manual/docblocks.html

using namespace llast;

void constructSimpleAst() {

    auto var1 = new Variable("var1", DataType::Int32);
    auto var2 = new Variable("var2", DataType::Int32);

    BlockBuilder bb;
    std::unique_ptr<Block const> blockExpr{
            bb.addVariable(var1)
            ->addVariable(var2)
            ->addExpression(new AssignVariable(var1, new LiteralInt32(12)))
            ->addExpression(new AssignVariable(var2, new Binary(new VariableRef(var1), OperationKind::Mul, new LiteralInt32(6))))
            ->build()
    };

    // Pretty print the AST
    PrettyPrinterVisitor visitor(std::cout);
    ExpressionTreeWalker walker(&visitor);
    walker.walkTree(blockExpr.get());

    EmbryonicCompiler::compileEmbryonically(blockExpr.get());
}

int main() {
    try {
        constructSimpleAst();
    } catch(Exception &e) {
        e.dump();
    }

    return 0;
}
