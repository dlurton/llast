
#pragma once

#include "visitor.hpp"

#include <iostream>
#include <algorithm>

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
            writeScopeVariables(expr);
        }

        void writeScopeVariables(const Scope *scope) {
            out_ << "(";
            std::vector<const Variable*> variables = scope->variables();
            if(variables.size() == 0) {
                out_ << ")";
            } else if(variables.size() == 1) {
                out_ << variables.front()->toString() << ")";
            } else {
                std::sort(variables.begin(), variables.end(),
                          [](const Variable* a, const Variable *b) {
                              return a->name() < b->name();
                          });

                for(auto itr = variables.begin(); itr != variables.end() - 1; ++itr) {
                    out_ << (*itr)->toString() << ", ";
                }

                out_ << variables.back()->toString() << ")";
            }
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

        void visitVariableRef(const VariableRef *expr) {
            out_ << "VariableRef: " << expr->name();
        }

        void visitingConditional(const Conditional *expr) {
            out_ << "Conditional:";
        }

        virtual void visitingAssignVariable(const AssignVariable *expr) {
            out_ << "AssignVariable: " << expr->name();
        }

        virtual void visitedAssignVariable(const AssignVariable *expr) {

        }
    };
}
