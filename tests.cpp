#include "AST.hpp"
#include "ExprRunner.hpp"
#include "SigHandler.hpp"

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

//https://github.com/google/googletest/blob/master/googletest/docs/Primer.md
using namespace llast;

int execInt32Expr(unique_ptr<const Expr> expr) {
    return ExprRunner::runInt32Expr(make_unique<Return>(move(expr)));
}

int execInt32Binary(int lvalue, OperationKind opKind, int rvalue, int expectedResult) {
    std::unique_ptr<const Expr> binaryExpr = Binary::make(LiteralInt32::make(lvalue), opKind, LiteralInt32::make(rvalue));
    int result = execInt32Expr(move(binaryExpr));
    return result == expectedResult;
}

bool isEssentiallyEqual(float a, float b)
{
    const float allowedVariance = 0.000001;
    const float diff = a - b;
    return diff >= -allowedVariance && diff <= allowedVariance;
}

float execFloatExpr(unique_ptr<const Expr> expr) {
    return ExprRunner::runFloatExpr(make_unique<Return>(move(expr)));
}

float execFloatBinary(float lvalue, OperationKind opKind, float rvalue, float expectedResult) {
    std::unique_ptr<const Expr> binaryExpr = Binary::make(LiteralFloat::make(lvalue), opKind, LiteralFloat::make(rvalue));
    float result = execFloatExpr(move(binaryExpr));

    return isEssentiallyEqual(result, expectedResult);
}

bool assertCompileError(CompileError expectedError, unique_ptr<const Expr> expr) {
    try {
        ExprRunner::compile(move(expr));
        std::cerr << "Compilation unexpectedly succeeded.\n";
        return false;
    } catch(CompileException &e) {
        if(e.error() != expectedError) {
            std::cerr << "Compilation failed for an unexpected reason.\n";
            return false;
        }
        return true;
    }
}

TEST_CASE("Int32 tests") {
    //LiteralInt32
    REQUIRE(execInt32Expr(LiteralInt32::make(1)) == 1);
    REQUIRE(execInt32Expr(LiteralInt32::make(-1)) == -1);
    REQUIRE(execInt32Expr(LiteralInt32::make(INT32_MAX)) == INT32_MAX);
    REQUIRE(execInt32Expr(LiteralInt32::make(INT32_MIN)) == INT32_MIN);

    //Int32 Arithmetic
    REQUIRE(execInt32Binary(1, OperationKind::Add, 2, 3));
    REQUIRE(execInt32Binary(6, OperationKind::Sub, 1, 5));
    REQUIRE(execInt32Binary(5, OperationKind::Mul, 3, 15));
    REQUIRE(execInt32Binary(16, OperationKind::Div, 2, 8));
    REQUIRE(execInt32Binary(1, OperationKind::Add, 2, 3));

    SECTION("Int32 variable assignment") {
        auto var1 = make_shared<Variable>("var1", DataType::Int32);
        BlockBuilder bb;
        unique_ptr<const Block> block {
                bb.addVariable(var1)
                        .addExpression(make_unique<AssignVariable>(var1, LiteralInt32::make(123)))
                        .addExpression(make_unique<Return>(make_unique<VariableRef>(var1)))
                        .build()
        };
        unique_ptr<const Expr> expr{move(block)};
        REQUIRE(ExprRunner::runInt32Expr(move(expr)) == 123);
    }
}

TEST_CASE("Float tests") {
    REQUIRE(execFloatExpr(LiteralFloat::make(1.0f)) == 1.0f);
    REQUIRE(execFloatExpr(LiteralFloat::make(-1.0f)) == -1.0f);
    REQUIRE(execFloatExpr(LiteralFloat::make(234944.0f)) == 234944.0f);
    REQUIRE(execFloatExpr(LiteralFloat::make(-142344.0f)) == -142344.0f);

    REQUIRE(execFloatBinary(1.1f, OperationKind::Add, 2.2, 3.3f));
    REQUIRE(execFloatBinary(6.2f, OperationKind::Sub, 1.1, 5.1f));
    REQUIRE(execFloatBinary(5.25f, OperationKind::Mul, 3.0, 15.75f));
    REQUIRE(execFloatBinary(7.0f, OperationKind::Div, 2.0, 3.5f));

    SECTION("Float variable assignment") {
        shared_ptr<Variable> var1 = make_shared<Variable>("var1", DataType::Float);
        BlockBuilder bb;
        unique_ptr<const Block> block {
                bb.addVariable(var1)
                .addExpression(AssignVariable::make(var1, LiteralFloat::make(123.0f)))
                .addExpression(make_unique<Return>(make_unique<VariableRef>(var1)))
                .build()
        };

        unique_ptr<const Expr> expr {move(block)};
        REQUIRE(ExprRunner::runFloatExpr(move(expr)) == 123.0f);
    }
}

TEST_CASE("Error conditions") {

    REQUIRE(assertCompileError(
            CompileError::BinaryExprDataTypeMismatch,
            Binary::make(LiteralFloat::make(1.0f), OperationKind::Add, LiteralInt32::make(1))));

}

int main(int argc, char **argv) {
    initSigSegvHandler();
    //llast::ExprRunner::init();

    int result = Catch::Session().run( argc, argv );

    //TODO?
    //llvm::llvm_shutdown();

    return ( result < 0xff ? result : 0xff );

}