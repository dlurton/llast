
#pragma once

#include "ast.hpp"


namespace llast {

    /** Serves as a factory for creating nodes and manages their lifetime.
     * When an instance of Context is destroyed, so is all of the expressions it has created.
     */
    class ExpressionTreeContext {
        std::vector<Expression*> ownedNodes_;

    private:

        template<typename E>
        E *addNode_(E *expr) {
            ownedNodes_.push_back(expr);
            return expr;
        }

    public:

        virtual ~ExpressionTreeContext() {
            for (auto node : ownedNodes_)
                delete node;
        }

        /** Creates new BlockExpression from an initializer list. */
        BlockExpression *newBlock(std::initializer_list<Expression *> expressions) {
            return addNode_(new BlockExpression(expressions));
        }

        /** Creates a new BlockExpression from a pair of iterators. */
        template<typename Iterator>
        BlockExpression *newBlock(Iterator begin, Iterator end) {
            std::vector<Expression*> expressions;

            for (; begin != end; ++begin)
                expressions.push_back(*begin);

            return addNode_(new BlockExpression(expressions));
        }

        LiteralInt32Expression *newLiteralInt32(const int value) {
            return addNode_(new LiteralInt32Expression(value));
        }

        BinaryExpression *newBinary(Expression *lValue, OperationKind operation, Expression *rValue) {
            return addNode_(new BinaryExpression(lValue, operation, rValue));
        }

        ConditionalExpression *newConditional(Expression *condition, Expression *truePart, Expression *falsePart) {
            return addNode_(new ConditionalExpression(condition, truePart, falsePart));
        }
   };
}