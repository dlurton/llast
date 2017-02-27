#include <iostream>
#include <vector>
#include <memory>

#include "context.hpp"
#include "visitor.hpp"
#include "pretty.hpp"

//https://github.com/IronLanguages/dlr/tree/master/Src/Microsoft.Scripting.Core/Ast
//http://www.stack.nl/~dimitri/doxygen/manual/docblocks.html

using namespace llast;

void demo() {

    {
        ExpressionTreeContext ctx;

        auto expr1 = ctx.newBinary(
                ctx.newLiteralInt32(10),
                OperationKind::Add,
                ctx.newLiteralInt32(11));

        auto expr2 = ctx.newBinary(
                ctx.newLiteralInt32(20),
                OperationKind::Sub,
                ctx.newLiteralInt32(21));

        auto expr3 = ctx.newConditional(
                ctx.newLiteralInt32(1),
                ctx.newLiteralInt32(30),
                ctx.newLiteralInt32(31));

        auto blockExpr = ctx.newBlock({expr1, expr2, expr3});

        PrettyPrinter visitor(std::cout);
        ExpressionTreeWalker walker(&visitor);

        walker.walk(blockExpr);
    }
}

int main() {

    demo();
    return 0;
}
