
#pragma once

#include <iostream>

#include "visitor.hpp"

namespace llast {

    class PrettyPrinter : public ExpressionTreeVisitor {
    private:
        int indent_ = -1;
        std::string tabs_;
        std::ostream &out_;

        void writeTabs() {
            for(int i = 0; i < indent_; ++i)
                out_ << "\t";
        }


    public:

        PrettyPrinter(std::ostream &out) : out_(out) { }
        ~PrettyPrinter() { }


        void visitingNode(const Expression *expr) {
            indent_++;
            out_ << "\n";
            writeTabs();
        }


        void visitedNode(const Expression *expr) {
            indent_--;
        }


        void visitingBlock(const BlockExpression *expr) {
            out_ << "{";
        }

        void visitedBlock(const BlockExpression *expr) {

            out_ << "}";
        }


        void visitingBinary(const BinaryExpression *expr) {
            out_ << to_string(expr->operation());
        }

        void visitLiteralInt32(const LiteralInt32Expression *expr) {
            out_ << std::to_string(expr->value());
        }
    };
}
