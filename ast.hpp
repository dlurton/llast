
#pragma once

#include <string>
#include <vector>

#include "exceptions.hpp"

namespace llast {

    enum class ExpressionType {
        Binary,
        Invoke,
        VarRef,
        Conditional,
        Switch,
        Block,
        LiteralInt32,
    };


    std::string to_string(ExpressionType expressionType) {
        switch (expressionType) {
            case ExpressionType::Binary:
                return "Binary";
            case ExpressionType::Invoke:
                return "Invoke";
            case ExpressionType::VarRef:
                return "VarRef";
            case ExpressionType::Conditional:
                return "Conditional";
            case ExpressionType::Switch:
                return "Switch";
            case ExpressionType::Block:
                return "Block";
            case ExpressionType::LiteralInt32:
                return "LiteralInt";
            default:
                throw UnhandledSwitchCase();
        }
    }


    enum class OperationType {
        Assign,
        Add,
        Sub,
        Mul,
        Div
    };


    std::string to_string(OperationType type) {
        switch (type) {
            case OperationType::Assign:
                return "Assign";
            case OperationType::Add:
                return "Add";
            case OperationType::Sub:
                return "Sub";
            case OperationType::Mul:
                return "Mul";
            case OperationType::Div:
                return "Div";
        }
    }


    enum class DataType {
        Void,
        Bool,
        Int32,
        Pointer
    };


    std::string to_string(DataType dataType) {
        switch (dataType) {
            case DataType::Void:
                return "void";
            case DataType::Bool:
                return "Bool";
            case DataType::Int32:
                return "Int32";
            case DataType::Pointer:
                return "Pointer";
            default:
                throw UnhandledSwitchCase();
        }
    }

    /** Base class for all expressions. */
    class Expression {
    public:
        virtual ExpressionType expressionType() const = 0;

        virtual DataType dataType() const { return DataType::Void; }
    };


    class LiteralInt32Expression : public Expression {
        const int value_;
    public:
        ExpressionType expressionType() const {
            return ExpressionType::LiteralInt32;
        }

        DataType dataType() const {
            return DataType::Int32;
        }

        LiteralInt32Expression(const int value) : value_(value) {

        }

        const int value() const {
            return value_;
        }
    };

    class BinaryExpression : public Expression {
        const Expression *lValue_;
        const OperationType operation_;
        const Expression *rValue_;

    public:
        ExpressionType expressionType() const {
            return ExpressionType::Binary;
        }

        /** The data type of an rValue expression is always the same as the rValue's data type. */
        DataType dataType() const {
            return rValue_->dataType();
        }

        BinaryExpression(Expression *lValue, OperationType operation, Expression *rValue)
                : lValue_(lValue), operation_(operation), rValue_(rValue) {
        }

        const Expression *lValue() const {
            return lValue_;
        }

        const Expression *rValue() const {
            return rValue_;
        }

        OperationType operation() const {
            return operation_;
        }
    };


    /** Contains a series of expressions, such as a method body, or one of the if-then-else parts. */
    class BlockExpression : public Expression {
        const std::vector<Expression *> expressions_;

    public:
        ExpressionType expressionType() const {
            return ExpressionType::Block;
        }

        /** The data type of a block expression is always the data type of the last expression in the block. */
        DataType dataType() const {
            return expressions_.back()->dataType();
        }

        BlockExpression(std::vector<Expression *> expressions) : expressions_(expressions) {

        }

        void forEach(std::function<void(Expression*)> func) {
            for(Expression *expr : expressions_)
                func(expr);
        }
    };


}