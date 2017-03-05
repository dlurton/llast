#include <iostream>
#include <vector>
#include <memory>

#include "visitor.hpp"
#include "pretty.hpp"

//https://github.com/IronLanguages/dlr/tree/master/Src/Microsoft.Scripting.Core/Ast
//http://www.stack.nl/~dimitri/doxygen/manual/docblocks.html

using namespace llast;

void constructSimpleAst() {

    //10 + 11
    Expr *expr1 = new Binary(new LiteralInt32(10), OperationKind::Add, new LiteralInt32(11));

    //20 - 21
    Expr *expr2 = new Binary(new LiteralInt32(20), OperationKind::Sub, new LiteralInt32(21));

    //1 ? 30 : 31
    Expr *expr3 = new Conditional(new LiteralInt32(1), new LiteralInt32(30), new LiteralInt32(31));

    // { all of the above }
    BlockBuilder bb;
    std::unique_ptr<Block const> blockExpr =
            bb.addExpression(expr1)
            ->addExpression(expr2)
            ->addExpression(expr3)
            ->build();

    // Pretty print the AST
    PrettyPrinterVisitor visitor(std::cout);
    ExpressionTreeWalker walker(&visitor);
    walker.walkTree(blockExpr.get());
}

int main() {
    try {
        constructSimpleAst();
    } catch(Exception &e) {
        e.dump();
    }

    return 0;
}
