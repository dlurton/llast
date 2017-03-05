
#pragma once

#include "ast.hpp"

namespace llast {

    /** The base class providing virtual methods with empty default implementations for all visitors.
     * Node types which do not have children will only have an overload of visit(...);  Node types that do have
     * children will have overloads for visiting(...) and visited(...) member functions.
     */
    class ExpressionTreeVisitor {
    public:

        virtual void initialize() {}

        virtual void cleanUp() {}

        /** Executes before every node is visited. */
        virtual void visitingNode(const Expr *expr) {}

        /** Executes after every node is visited. */
        virtual void visitedNode(const Expr *expr) {}

        virtual void visitingBlock(const Block *expr) {}

        virtual void visitedBlock(const Block *expr) {}

        virtual void visitingConditional(const Conditional *expr) {}

        virtual void visitedConditional(const Conditional *expr) {}

        virtual void visitingBinary(const Binary *expr) {}

        virtual void visitedBinary(const Binary *expr) {}

        virtual void visitLiteralInt32(const LiteralInt32 *expr) {}

    };

    /** Default tree walker, suitable for most purposes */
    class ExpressionTreeWalker {
        ExpressionTreeVisitor *visitor_;

    public:
        ExpressionTreeWalker(ExpressionTreeVisitor *visitor) : visitor_(visitor) {

        }

        virtual ~ExpressionTreeWalker() {

        }

        void walkTree(const Expr *expr) {
            visitor_->initialize();

            this->walk(expr);

            visitor_->cleanUp();
        }

    protected:

        virtual void walk(const Expr *expr) const {
            ARG_NOT_NULL(expr);
            switch (expr->expressionType()) {
                case ExpressionKind::LiteralInt32:
                    walkLiteralInt32((LiteralInt32 *) expr);
                    break;
                case ExpressionKind::Binary:
                    walkBinary((Binary *) expr);
                    break;
                case ExpressionKind::Block:
                    walkBlock((Block *) expr);
                    break;
                case ExpressionKind::Conditional:
                    walkConditional((Conditional *) expr);
                    break;
                default:
                    throw UnhandledSwitchCase();
            }
        }

        void walkBlock(const Block *blockExpr) const {
            ARG_NOT_NULL(blockExpr);
            visitor_->visitingNode(blockExpr);
            visitor_->visitingBlock(blockExpr);

            blockExpr->forEach([this](const Expr *childExpr) { walk(childExpr); });

            visitor_->visitedBlock(blockExpr);
            visitor_->visitedNode(blockExpr);
        }

        void walkBinary(const Binary *binaryExpr) const {
            ARG_NOT_NULL(binaryExpr);
            visitor_->visitingNode(binaryExpr);
            visitor_->visitingBinary(binaryExpr);

            walk(binaryExpr->rValue());
            walk(binaryExpr->lValue());

            visitor_->visitedBinary(binaryExpr);
            visitor_->visitedNode(binaryExpr);
        }

        void walkConditional(Conditional *conditionalExpr) const {
            ARG_NOT_NULL(conditionalExpr);
            visitor_->visitingNode(conditionalExpr);
            visitor_->visitingConditional(conditionalExpr);

            walk(conditionalExpr->condition());
            if (conditionalExpr->truePart())
                walk(conditionalExpr->truePart());

            if (conditionalExpr->falsePart())
                walk(conditionalExpr->falsePart());

            visitor_->visitedConditional(conditionalExpr);
            visitor_->visitedNode(conditionalExpr);
        }


        void walkLiteralInt32(const LiteralInt32 *expr) const {
            ARG_NOT_NULL(expr);
            visitor_->visitingNode(expr);
            visitor_->visitLiteralInt32(expr);
            visitor_->visitedNode(expr);
        }
    };
}