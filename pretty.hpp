
#pragma once

#include <iostream>

#include "visitor.hpp"

namespace llast {

    class PrettyPrinterVisitor : public ExpressionTreeVisitor {
    private:
        int indent_ = -1;
        std::ostream &out_;

        void writeTabs(int changeIndentBy = 0) {
            int finalTabCount = indent_ + changeIndentBy;
            for(int i = 0; i < indent_; ++i)
                out_ << "\t";
        }

    public:

        PrettyPrinterVisitor(std::ostream &out) : out_(out) { }
        ~PrettyPrinterVisitor() { }

        void cleanUp() {
            out_ << "\n";
        }

        void visitingNode(const Expr *expr) {
            indent_++;
            out_ << "\n";
            writeTabs();
        }

        void visitedNode(const Expr *expr) {
            indent_--;
        }

        void visitingBlock(const Block *expr) {
            out_ << "Block:";
        }

        void visitedBlock(const Block *expr) {
//            out_ << "\n";
//            writeTabs(-1);
//            out_ << "}";
        }

        void visitingBinary(const Binary *expr) {
            out_ << "Binary: " << to_string(expr->operation());
        }

        void visitLiteralInt32(const LiteralInt32 *expr) {
            out_ << "LiteralInt32: " << std::to_string(expr->value());
        }

        void visitingConditional(const Conditional *expr) {
            out_ << "Conditional:";
        }
    };
}
