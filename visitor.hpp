
#pragma once

#include "ast.hpp"

namespace llast {

    /** The base class providing virtual methods with empty default implementations for all visitors.
     * Node types which do not have children will only have an overload of visit(...);  Node types that do have
     * children will have overloads for visiting(...) and visited(...) member functions.
     */
    class ExpressionTreeVisitor {
    public:

        /** Executes before every node is visited. */
        virtual void visitingNode(const Expression *expr) { }

        /** Executes after every node is visited. */
        virtual void visitedNode(const Expression *expr) { }

        virtual void visiting(const BlockExpression *expr) { }
        virtual void visited(const BlockExpression *expr) { }

        virtual void visiting(const BinaryExpression *expr) { }
        virtual void visited(const BinaryExpression *expr) { }

        virtual void visit(const LiteralInt32Expression *expr) { }
    };

    /** Default tree walker, suitable for most purposes */
    class ExpressionTreeWalker {
        ExpressionTreeVisitor *visitor_;

    public:
        ExpressionTreeWalker(ExpressionTreeVisitor *visitor) : visitor_(visitor) {

        }

        virtual void walk(const Expression *expr) {
            visitor_->visitingNode(expr);

            switch (expr->expressionType()) {
                case ExpressionType::Block: {
                    auto blockExpr = (BlockExpression *) expr;
                    visitor_->visiting(blockExpr);
                    blockExpr->forEach([this](Expression *childExpr) { this->walk(childExpr); });
                    visitor_->visited(blockExpr);
                    break;
                }
                case ExpressionType::LiteralInt32: {
                    auto literalInt32 = (LiteralInt32Expression *) expr;
                    visitor_->visit(literalInt32);
                    break;
                }
                case ExpressionType::Binary: {
                    auto binaryExpr = (BinaryExpression*)expr;
                    visitor_->visiting(binaryExpr);
                    walk(binaryExpr->rValue());
                    walk(binaryExpr->lValue());
                    visitor_->visited(binaryExpr);
                    break;
                }
                default:
                    throw UnhandledSwitchCase();
            }

            visitor_->visitedNode(expr);
        }

    };

}