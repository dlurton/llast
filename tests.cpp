#include <llvm/Support/ManagedStatic.h>
#include "AST.hpp"
#include "ExprRunner.hpp"
#include "SigHandler.hpp"

//#include "gtest/gtest.h"
#define CATCH_CONFIG_RUNNER
#include "catch.hpp"


//https://github.com/google/googletest/blob/master/googletest/docs/Primer.md
using namespace llast;

//namespace Int32Tests {
//
//    class Int32ExprTest : public ::testing::Test { };

//    int executeExpr(unique_ptr<const Expr> expr) {
//
//        return ExprRunner::runInt32Expr(make_unique<Return>(move(expr)));
//    }
//
//    //LiteralInt32
//    TEST_F(Int32ExprTest, CanReturnPositiveInt32) { ASSERT_EQ(1, executeExpr(make_unique<LiteralInt32>(1))); }
//    TEST_F(Int32ExprTest, CanReturnNegativeInt32) { ASSERT_EQ(-1, executeExpr(make_unique<LiteralInt32>(-1))); }
//    TEST_F(Int32ExprTest, CanReturnArbitraryPositiveInt32) { ASSERT_EQ(234944, executeExpr(make_unique<LiteralInt32>(234944))); }
//    TEST_F(Int32ExprTest, CanReturnArbitraryNegativeInt32) { ASSERT_EQ(-142344, executeExpr(make_unique<LiteralInt32>(-142344))); }
//
//    //Int32 Arithmetic
//    TEST_F(Int32ExprTest, CanAddInt32) {
//        ASSERT_EQ(3, executeExpr(make_unique<Binary>(make_unique<LiteralInt32>(1), OperationKind::Add, make_unique<LiteralInt32>(2))));
//    }
//    TEST_F(Int32ExprTest, CanSubInt32) {
//        ASSERT_EQ(5, executeExpr(make_unique<Binary>(make_unique<LiteralInt32>(6), OperationKind::Sub, make_unique<LiteralInt32>(1))));
//    }
//    TEST_F(Int32ExprTest, CanMulInt32)  {
//        ASSERT_EQ(15, executeExpr(make_unique<Binary>(make_unique<LiteralInt32>(5), OperationKind::Mul, make_unique<LiteralInt32>(3))));
//    }
//    TEST_F(Int32ExprTest, CanDivInt32)  {
//        ASSERT_EQ(8, executeExpr(make_unique<Binary>(make_unique<LiteralInt32>(16), OperationKind::Div, make_unique<LiteralInt32>(2))));
//    }
//
//    //Int32 variable assignment
//    TEST_F(Int32ExprTest, CanAssignAndReturnInt32Variable)  {
//        auto var1 = make_shared<Variable>("var1", DataType::Int32);
//        BlockBuilder bb;
//        unique_ptr<const Block> block {
//                bb.addVariable(var1)
//                .addExpression(make_unique<AssignVariable>(var1, make_unique<LiteralInt32>(123)))
//                .addExpression(make_unique<Return>(make_unique<VariableRef>(var1)))
//                .build()
//        };
//
//        unique_ptr<const Expr> expr{move(block)};
//        ASSERT_EQ(123, ExprRunner::runInt32Expr(move(expr)));
//    }
//}
//namespace Int32Tests
//
//namespace FloatTests {
//
//    class FloatExprTest : public ::testing::Test { };
//
//    bool isEssentiallyEqual(float a, float b)
//    {
//        const float allowedVariance = 0.000001;
//        const float diff = a - b;
//        return diff >= -allowedVariance && diff <= allowedVariance;
//    }
//
//    float executeExpr(unique_ptr<const Expr> expr) {
//        return ExprRunner::runFloatExpr(make_unique<Return>(move(expr)));
//    }
//
//    //LiteralFloat
//    TEST_F(FloatExprTest, CanReturnPositiveFloat) { ASSERT_EQ(1.0f, executeExpr(make_unique<LiteralFloat>(1.0f))); }
//    TEST_F(FloatExprTest, CanReturnNegativeFloat) { ASSERT_EQ(-1.0f, executeExpr(make_unique<LiteralFloat>(-1.0f))); }
//    TEST_F(FloatExprTest, CanReturnArbitraryPositiveFloat) { ASSERT_EQ(234944.0f, executeExpr(make_unique<LiteralFloat>(234944.0f))); }
//    TEST_F(FloatExprTest, CanReturnArbitraryNegativeFloat) { ASSERT_EQ(-142344.0f, executeExpr(make_unique<LiteralFloat>(-142344.0f))); }
//
//    //Float Arithmetic
//    TEST_F(FloatExprTest, CanAddFloat) {
//        float result = executeExpr(make_unique<Binary>(make_unique<LiteralFloat>(1.1f), OperationKind::Add, make_unique<LiteralFloat>(2.2)));
//        ASSERT_TRUE(isEssentiallyEqual(3.3f, result));
//    }
//    TEST_F(FloatExprTest, CanSubFloat) {
//        float result = executeExpr(make_unique<Binary>(make_unique<LiteralFloat>(6.2f), OperationKind::Sub, make_unique<LiteralFloat>(1.1f)));
//        ASSERT_TRUE(isEssentiallyEqual(5.1f, result));
//    }
//    TEST_F(FloatExprTest, CanMulFloat)  {
//        float result = executeExpr(make_unique<Binary>(make_unique<LiteralFloat>(5.25f), OperationKind::Mul, make_unique<LiteralFloat>(3.0f)));
//        ASSERT_TRUE(isEssentiallyEqual(15.75f, result));
//    }
//    TEST_F(FloatExprTest, CanDivFloat)  {
//        float result = executeExpr(make_unique<Binary>(make_unique<LiteralFloat>(7.0f), OperationKind::Div, make_unique<LiteralFloat>(2.0f)));
//        ASSERT_TRUE(isEssentiallyEqual(3.5f, result));
//    }
//
//    //Float variable assignment
//    TEST_F(FloatExprTest, CanAssignAndReturnFloatVariable)  {
//        shared_ptr<Variable> var1 = make_shared<Variable>("var1", DataType::Float);
//        BlockBuilder bb;
//        unique_ptr<const Block> block {
//                bb.addVariable(var1)
//                .addExpression(make_unique<AssignVariable>(var1, make_unique<LiteralFloat>(123.0f)))
//                .addExpression(make_unique<Return>(make_unique<VariableRef>(var1)))
//                .build()
//        };
//
//        unique_ptr<const Expr> expr {move(block)};
//        ASSERT_EQ(123.0f, ExprRunner::runFloatExpr(move(expr)));
//    }
//} //namespace FloatTests

int executeExpr(unique_ptr<const Expr> expr) {
    return ExprRunner::runInt32Expr(make_unique<Return>(move(expr)));
}

bool tryCatchCompileError(CompileError expectedError, unique_ptr<const Expr> expr) {
    try {
        ExprRunner::compile(move(expr));
        return false;
    } catch(CompileException &e) {
        REQUIRE(expectedError == e.error());// << "Compilation must fail for the expected reason.";
        return true;
    }
}

TEST_CASE("Error case tests") {
    REQUIRE(executeExpr(make_unique<LiteralInt32>(1)) == 1);

    REQUIRE(tryCatchCompileError(CompileError::BinaryExprDataTypeMismatch,
                               make_unique<Binary>(make_unique<LiteralFloat>(1.0f),
                                                   OperationKind::Add,
                                                   make_unique<LiteralInt32>(1))));

}//namespace ErrorTests

int main(int argc, char **argv) {

    llast::ExprRunner::init();

    int result = Catch::Session().run( argc, argv );

    llvm::llvm_shutdown();

    return ( result < 0xff ? result : 0xff );

}