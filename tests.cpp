
#include "Expr.hpp"
#include "EmbryonicCompiler.hpp"

#include "gtest/gtest.h"

//https://github.com/google/googletest/blob/master/googletest/docs/Primer.md
using namespace llast;

namespace Int32Tests {

    class Int32ExprTest : public ::testing::Test { };

    int executeExpr(const Expr *expr) {
        auto retExpr = std::make_unique<Return const>(expr);
        return EmbryonicCompiler::compileAndExecuteInt32Expr(retExpr.get());
    }

    //LiteralInt32
    TEST_F(Int32ExprTest, CanReturnPositiveInt32) { ASSERT_EQ(1, executeExpr(new LiteralInt32(1))); }
    TEST_F(Int32ExprTest, CanReturnNegativeInt32) { ASSERT_EQ(-1, executeExpr(new LiteralInt32(-1))); }
    TEST_F(Int32ExprTest, CanReturnArbitraryPositiveInt32) { ASSERT_EQ(234944, executeExpr(new LiteralInt32(234944))); }
    TEST_F(Int32ExprTest, CanReturnArbitraryNegativeInt32) { ASSERT_EQ(-142344, executeExpr(new LiteralInt32(-142344))); }

    //Int32 Arithmetic
    TEST_F(Int32ExprTest, CanAddInt32) {
        ASSERT_EQ(3, executeExpr(new Binary(new LiteralInt32(1), OperationKind::Add, new LiteralInt32(2))));
    }
    TEST_F(Int32ExprTest, CanSubInt32) {
        ASSERT_EQ(5, executeExpr(new Binary(new LiteralInt32(6), OperationKind::Sub, new LiteralInt32(1))));
    }
    TEST_F(Int32ExprTest, CanMulInt32)  {
        ASSERT_EQ(15, executeExpr(new Binary(new LiteralInt32(5), OperationKind::Mul, new LiteralInt32(3))));
    }
    TEST_F(Int32ExprTest, CanDivInt32)  {
        ASSERT_EQ(8, executeExpr(new Binary(new LiteralInt32(16), OperationKind::Div, new LiteralInt32(2))));
    }

    //Int32 variable assignment
    TEST_F(Int32ExprTest, CanAssignAndReturnInt32Variable)  {
        Variable *var1 = new Variable("var1", DataType::Int32);
        BlockBuilder bb;
        std::unique_ptr<const Block> block {
                bb.addVariable(var1)
                        ->addExpression(new AssignVariable(var1, new LiteralInt32(123)))
                        ->addExpression(new Return(new VariableRef(var1)))
                        ->build()
        };
        ASSERT_EQ(123, EmbryonicCompiler::compileAndExecuteInt32Expr(block.get()));
    }

} //namespace Int32Tests

namespace FloatTests {

    class FloatExprTest : public ::testing::Test { };

    bool isEssentiallyEqual(float a, float b)
    {
        const float allowedVariance = 0.000001;
        const float diff = a - b;
        return diff >= -allowedVariance && diff <= allowedVariance;
    }

    float executeExpr(const Expr *expr) {
        auto retExpr = std::make_unique<Return const>(expr);
        return EmbryonicCompiler::compileAndExecuteFloatExpr(retExpr.get());
    }

    //LiteralFloat
    TEST_F(FloatExprTest, CanReturnPositiveFloat) { ASSERT_EQ(1.0f, executeExpr(new LiteralFloat(1.0f))); }
    TEST_F(FloatExprTest, CanReturnNegativeFloat) { ASSERT_EQ(-1.0f, executeExpr(new LiteralFloat(-1.0f))); }
    TEST_F(FloatExprTest, CanReturnArbitraryPositiveFloat) { ASSERT_EQ(234944.0f, executeExpr(new LiteralFloat(234944.0f))); }
    TEST_F(FloatExprTest, CanReturnArbitraryNegativeFloat) { ASSERT_EQ(-142344.0f, executeExpr(new LiteralFloat(-142344.0f))); }

    //Float Arithmetic
    TEST_F(FloatExprTest, CanAddFloat) {
        float result = executeExpr(new Binary(new LiteralFloat(1.1f), OperationKind::Add, new LiteralFloat(2.2)));
        ASSERT_TRUE(isEssentiallyEqual(3.3f, result));
    }
    TEST_F(FloatExprTest, CanSubFloat) {
        float result = executeExpr(new Binary(new LiteralFloat(6.2f), OperationKind::Sub, new LiteralFloat(1.1f)));
        ASSERT_TRUE(isEssentiallyEqual(5.1f, result));
    }
    TEST_F(FloatExprTest, CanMulFloat)  {
        float result = executeExpr(new Binary(new LiteralFloat(5.25f), OperationKind::Mul, new LiteralFloat(3.0f)));
        ASSERT_TRUE(isEssentiallyEqual(15.75f, result));
    }
    TEST_F(FloatExprTest, CanDivFloat)  {
        float result = executeExpr(new Binary(new LiteralFloat(7.0f), OperationKind::Div, new LiteralFloat(2.0f)));
        ASSERT_TRUE(isEssentiallyEqual(3.5f, result));
    }

    //Float variable assignment
    TEST_F(FloatExprTest, CanAssignAndReturnFloatVariable)  {
        Variable *var1 = new Variable("var1", DataType::Float);
        BlockBuilder bb;
        std::unique_ptr<const Block> block {
                bb.addVariable(var1)
                        ->addExpression(new AssignVariable(var1, new LiteralFloat(123.0f)))
                        ->addExpression(new Return(new VariableRef(var1)))
                        ->build()
        };
        ASSERT_EQ(123.0f, EmbryonicCompiler::compileAndExecuteFloatExpr(block.get()));
    }
} //namespace FloatTests

namespace ErrorTests {
    class ErrorTests : public ::testing::Test {  };

    void assertCompileError(CompileError expectedError, Expr *expr) {
        try {
            EmbryonicCompiler::compile(expr);
            FAIL() << "Compilation unexpectedly succeeded.";
        } catch(CompileException &e) {
            ASSERT_EQ(expectedError, e.error()) << "Compilation failed for an unexpected reason.";
        }
    }

    TEST_F(ErrorTests, MismatchedDataTypeThowsException) {
        assertCompileError(CompileError::BinaryExprDataTypeMismatch,
                           new Binary(new LiteralFloat(1.0f), OperationKind::Add, new LiteralInt32(1)));
    }

}//namespace ErrorTests