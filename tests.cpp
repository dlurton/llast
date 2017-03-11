
#include "Expr.hpp"
#include "EmbryonicCompiler.hpp"

#include "gtest/gtest.h"
#include "ExpressionTreeVisitor.hpp"

//https://github.com/google/googletest/blob/master/googletest/docs/Primer.md
using namespace llast;

namespace {

    class ExprTest : public ::testing::Test {
    protected:
        // You can remove any or all of the following functions if its body
        // is empty.

        ExprTest() {
            // You can do set-up work for each test here.
        }

        virtual ~ExprTest() {
            // You can do clean-up work that doesn't throw exceptions here.
        }

        // If the constructor and destructor are not enough for setting up
        // and cleaning up each test, you can define the following methods:

        virtual void SetUp() {
            // Code here will be called immediately after the constructor (right
            // before each test).
        }

        virtual void TearDown() {
            // Code here will be called immediately after each test (right
            // before the destructor).
        }

        // Objects declared here can be used by all tests in the test case for Foo.
    };

    int executeExpr(const Expr *expr) {
        auto retExpr = std::make_unique<Return const>(expr);
        return EmbryonicCompiler::compileAndExecute(retExpr.get());
    }

    //LiteralInt32
    TEST_F(ExprTest, CanReturnPositiveInt) { EXPECT_EQ(1, executeExpr(new LiteralInt32(1))); }
    TEST_F(ExprTest, CanReturnNegativeInt) { EXPECT_EQ(-1, executeExpr(new LiteralInt32(-1))); }
    TEST_F(ExprTest, CanReturnArbitraryPositiveInt) { EXPECT_EQ(234944, executeExpr(new LiteralInt32(234944))); }
    TEST_F(ExprTest, CanReturnArbitraryNegativeInt) { EXPECT_EQ(-142344, executeExpr(new LiteralInt32(-142344))); }

    //Int32 Arithmetic
    TEST_F(ExprTest, CanAddInt32) {
        EXPECT_EQ(3, executeExpr(new Binary(new LiteralInt32(1), OperationKind::Add, new LiteralInt32(2))));
    }
    TEST_F(ExprTest, CanSubInt32) {
        EXPECT_EQ(5, executeExpr(new Binary(new LiteralInt32(6), OperationKind::Sub, new LiteralInt32(1))));
    }
    TEST_F(ExprTest, CanMulInt32)  {
        EXPECT_EQ(15, executeExpr(new Binary(new LiteralInt32(5), OperationKind::Mul, new LiteralInt32(3))));
    }
    TEST_F(ExprTest, CanDivInt32)  {
        EXPECT_EQ(8, executeExpr(new Binary(new LiteralInt32(16), OperationKind::Div, new LiteralInt32(2))));
    }

    //Int32 variable assignment
    TEST_F(ExprTest, CanAssignAndReturnVariable)  {
        Variable *var1 = new Variable("var1", DataType::Int32);
        BlockBuilder bb;
        std::unique_ptr<const Block> block {
                bb.addVariable(var1)
                        ->addExpression(new AssignVariable(var1, new LiteralInt32(123)))
                        ->addExpression(new Return(new VariableRef(var1)))
                        ->build()
        };
        EXPECT_EQ(123, EmbryonicCompiler::compileAndExecute(block.get()));
    }
}