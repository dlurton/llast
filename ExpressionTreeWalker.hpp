#pragma once

#include "ExpressionTreeVisitor.hpp"

namespace llast {
/** Default tree walker, suitable for most purposes */
    class ExpressionTreeWalker {
        ExpressionTreeVisitor *visitor_;

    public:
        ExpressionTreeWalker(ExpressionTreeVisitor *visitor) : visitor_{visitor} {

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
            switch (expr->expressionKind()) {
                case ExpressionKind::LiteralInt32:
                    walkLiteralInt32((LiteralInt32 *) expr);
                    break;
                case ExpressionKind::LiteralFloat:
                    walkLiteralFloat((LiteralFloat *) expr);
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
                case ExpressionKind::VariableRef:
                    walkVariableRef((VariableRef *) expr);
                    break;
                case ExpressionKind::AssignVariable:
                    walkAssignVariable((AssignVariable *) expr);
                    break;
                case ExpressionKind::Return:
                    walkReturn((Return *) expr);
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

        void walkVariableRef(const VariableRef *variableRefExpr) const {
            ARG_NOT_NULL(variableRefExpr);
            visitor_->visitingNode(variableRefExpr);
            visitor_->visitVariableRef(variableRefExpr);
            visitor_->visitedNode(variableRefExpr);
        }

        void walkAssignVariable(const AssignVariable *assignVariableExpr) const {

            ARG_NOT_NULL(assignVariableExpr);
            visitor_->visitingNode(assignVariableExpr);
            visitor_->visitingAssignVariable(assignVariableExpr);

            walk(assignVariableExpr->valueExpr());

            visitor_->visitedAssignVariable(assignVariableExpr);
            visitor_->visitedNode(assignVariableExpr);
        }

        void walkReturn(const Return *returnExpr) const {

            ARG_NOT_NULL(returnExpr);
            visitor_->visitingNode(returnExpr);
            visitor_->visitingReturn(returnExpr);

            walk(returnExpr->valueExpr());

            visitor_->visitedReturn(returnExpr);
            visitor_->visitedNode(returnExpr);
        }

        void walkBinary(const Binary *binaryExpr) const {
            ARG_NOT_NULL(binaryExpr);
            visitor_->visitingNode(binaryExpr);
            visitor_->visitingBinary(binaryExpr);

            walk(binaryExpr->lValue());
            walk(binaryExpr->rValue());

            visitor_->visitedBinary(binaryExpr);
            visitor_->visitedNode(binaryExpr);
        }

        void walkConditional(Conditional *conditionalExpr) const {
            ARG_NOT_NULL(conditionalExpr);
            visitor_->visitingNode(conditionalExpr);
            visitor_->visitingConditional(conditionalExpr);

            walk(conditionalExpr->condition());
            if (conditionalExpr->truePart()) {
                walk(conditionalExpr->truePart());
            }

            if (conditionalExpr->falsePart()) {
                walk(conditionalExpr->falsePart());
            }

            visitor_->visitedConditional(conditionalExpr);
            visitor_->visitedNode(conditionalExpr);
        }

        void walkLiteralInt32(const LiteralInt32 *expr) const {
            ARG_NOT_NULL(expr);
            visitor_->visitingNode(expr);
            visitor_->visitLiteralInt32(expr);
            visitor_->visitedNode(expr);
        }

        void walkLiteralFloat(const LiteralFloat *expr) const {
            ARG_NOT_NULL(expr);
            visitor_->visitingNode(expr);
            visitor_->visitLiteralFloat(expr);
            visitor_->visitedNode(expr);
        }
    };

}