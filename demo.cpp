#include <iostream>
#include <vector>
#include <memory>

#include "ExpressionTreeVisitor.hpp"
#include "PrettyPrinter.hpp"
#include "ExprRunner.hpp"
#include "ExpressionTreeWalker.hpp"

//TODO:  http://llvm.org/docs/tutorial/LangImpl05.html#if-then-else

//https://github.com/IronLanguages/dlr/tree/master/Src/Microsoft.Scripting.Core/Ast
//http://www.stack.nl/~dimitri/doxygen/manual/docblocks.html

using namespace llast;

void constructSimpleAst() {
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
                    ->addExpression(new Return(new VariableRef(var2)))
            ->build()
    };

    // Pretty print the AST
    PrettyPrinterVisitor visitor(std::cout);
    ExpressionTreeWalker walker(&visitor);
    walker.walkModule(blockExpr.get());

    ExprRunner::runInt32Expr(blockExpr.get());
}

int main() {
    try {
        constructSimpleAst();
    } catch(Exception &e) {
        e.dump();
    }

    return 0;
}
