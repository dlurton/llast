#include <iostream>
#include <vector>
#include <memory>

#include "context.hpp"
#include "visitor.hpp"
#include "pretty.hpp"

//https://github.com/IronLanguages/dlr/tree/master/Src/Microsoft.Scripting.Core/Ast
//http://www.stack.nl/~dimitri/doxygen/manual/docblocks.html

using namespace llast;

int main() {

    ExpressionTreeContext context;

    auto expr1 = context.newBinaryExpression(
            context.newLiteralInt32(10),
            OperationType::Add,
            context.newLiteralInt32(11));

    auto expr2 =  context.newBinaryExpression(
            context.newLiteralInt32(20),
            OperationType::Sub,
            context.newLiteralInt32(21));

    auto blockExpr = context.newBlock({expr1, expr2});

    PrettyPrinter visitor(std::cout);
    ExpressionTreeWalker walker(&visitor);

    walker.walk(blockExpr);

    return 0;
}
