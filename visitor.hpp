
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

        virtual void visitingBlock(const BlockExpression *expr) { }
        virtual void visitedBlock(const BlockExpression *expr) { }

        virtual void visitingConditional(const ConditionalExpression *expr) { }
        virtual void visitedConditional(const ConditionalExpression *expr) { }

        virtual void visitingBinary(const BinaryExpression *expr) { }
        virtual void visitedBinary(const BinaryExpression *expr) { }

        virtual void visitLiteralInt32(const LiteralInt32Expression *expr) { }

    };

    /** Default tree walker, suitable for most purposes */
    class ExpressionTreeWalker {
        ExpressionTreeVisitor *visitor_;

    public:
        ExpressionTreeWalker(ExpressionTreeVisitor *visitor) : visitor_(visitor) {

        }

        virtual ~ExpressionTreeWalker() {

        }


        virtual void walk(const Expression *expr) const {
            ARG_NOT_NULL(expr);
            switch (expr->expressionType()) {
                case ExpressionKind::LiteralInt32:
                    walkLiteralInt32((LiteralInt32Expression*)expr);
                    break;
                case ExpressionKind::Binary:
                    walkBinary((BinaryExpression*)expr);
                    break;
                case ExpressionKind::Block:
                    walkBlock((BlockExpression*)expr);
                    break;
                case ExpressionKind::Conditional:
                    walkConditional((ConditionalExpression*)expr);
                    break;
                default:
                    throw UnhandledSwitchCase();
            }
        }

        void walkBlock(const BlockExpression *blockExpr) const {
            ARG_NOT_NULL(blockExpr);
            visitor_->visitingNode(blockExpr);
            visitor_->visitingBlock(blockExpr);

            blockExpr->forEach([this](Expression *childExpr) { walk(childExpr); });

            visitor_->visitedBlock(blockExpr);
            visitor_->visitedNode(blockExpr);
        }

        void walkBinary(const BinaryExpression *binaryExpr) const {
            ARG_NOT_NULL(binaryExpr);
            visitor_->visitingNode(binaryExpr);
            visitor_->visitingBinary(binaryExpr);

            walk(binaryExpr->rValue());
            walk(binaryExpr->lValue());

            visitor_->visitedBinary(binaryExpr);
            visitor_->visitedNode(binaryExpr);
        }

        void walkConditional(ConditionalExpression *conditionalExpr) const {
            ARG_NOT_NULL(conditionalExpr);
            visitor_->visitingNode(conditionalExpr);
            visitor_->visitingConditional(conditionalExpr);

            walk(conditionalExpr->condition());
            if(conditionalExpr->truePart())
                walk(conditionalExpr->truePart());

            if(conditionalExpr->falsePart())
                walk(conditionalExpr->falsePart());

            visitor_->visitedConditional(conditionalExpr);
            visitor_->visitedNode(conditionalExpr);
        }


        void walkLiteralInt32(const LiteralInt32Expression *expr) const {
            ARG_NOT_NULL(expr);
            visitor_->visitingNode(expr);
            visitor_->visitLiteralInt32(expr);
            visitor_->visitedNode(expr);
        }
    };

}