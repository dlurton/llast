
#pragma once

#include "Expr.hpp"

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

        virtual void visitingReturn(const Return *expr) {}
        virtual void visitedReturn(const Return *expr) {}

        virtual void visitVariableRef(const VariableRef *expr) {}

        virtual void visitingAssignVariable(const AssignVariable *expr) {}

        virtual void visitedAssignVariable(const AssignVariable *expr) {}
    };

}