
#pragma once

#include "ExpressionTreeVisitor.hpp"

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

        void cleanUp() override {
            out_ << "\n";
        }

        void visitingNode(const Node *node) override {
            indent_++;
            out_ << "\n";
            writeTabs();
        }

        void visitedNode(const Node *node) override {
            indent_--;
        }

        void visitingBlock(const Block *expr) override {
            out_ << "Block:";
            writeScopeVariables(expr->scope());
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

        void visitedBlock(const Block *expr) override {
//            out_ << "\n";
//            writeTabs(-1);
//            out_ << "}";
        }

        void visitingBinary(const Binary *expr) override {
            out_ << "Binary: " << to_string(expr->operation());
        }

        void visitLiteralInt32(const LiteralInt32 *expr) override {
            out_ << "LiteralInt32: " << std::to_string(expr->value());
        }
        void visitLiteralFloat(const LiteralFloat *expr) override {
            out_ << "LiteralFloat: " << std::to_string(expr->value());
        }

        void visitVariableRef(const VariableRef *expr) override {
            out_ << "VariableRef: " << expr->name();
        }

        void visitingConditional(const Conditional *expr) override {
            out_ << "Conditional: ";
        }

        virtual void visitingAssignVariable(const AssignVariable *expr) override {
            out_ << "AssignVariable: " << expr->name();
        }

        virtual void visitingReturn(const Return *expr) override {
            out_ << "Return: ";
        }

        virtual void visitingFunction(const Function *func) override {
            out_ << "Function: " << func->name();
        }

        //virtual void visitedFunction(const Function *func) override {}

        virtual void visitingModule(const Module *module) override {
            out_ << "Module: " << module->name();
        }

        //virtual void visitedModule(const Module *module) override {}
    };
}
