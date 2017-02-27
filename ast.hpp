
#pragma once

#include <string>
#include <vector>

#include "exceptions.hpp"

namespace llast {

    enum class ExpressionKind {
        Binary,
        Invoke,
        VarRef,
        Conditional,
        Switch,
        Block,
        LiteralInt32,
    };


    std::string to_string(ExpressionKind expressionType) {
        switch (expressionType) {
            case ExpressionKind::Binary:
                return "Binary";
            case ExpressionKind::Invoke:
                return "Invoke";
            case ExpressionKind::VarRef:
                return "VarRef";
            case ExpressionKind::Conditional:
                return "Conditional";
            case ExpressionKind::Switch:
                return "Switch";
            case ExpressionKind::Block:
                return "Block";
            case ExpressionKind::LiteralInt32:
                return "LiteralInt";
            default:
                throw UnhandledSwitchCase();
        }
    }


    enum class OperationKind {
        Assign,
        Add,
        Sub,
        Mul,
        Div
    };


    std::string to_string(OperationKind type) {
        switch (type) {
            case OperationKind::Assign:
                return "Assign";
            case OperationKind::Add:
                return "Add";
            case OperationKind::Sub:
                return "Sub";
            case OperationKind::Mul:
                return "Mul";
            case OperationKind::Div:
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

        virtual ~Expression() { }
        virtual ExpressionKind expressionType() const = 0;

        virtual DataType dataType() const { return DataType::Void; }
    };


    class LiteralInt32Expression : public Expression {
        const int value_;
    public:

        LiteralInt32Expression(const int value) : value_(value) {

        }

        virtual ~LiteralInt32Expression() {

        }

        ExpressionKind expressionType() const {
            return ExpressionKind::LiteralInt32;
        }

        DataType dataType() const {
            return DataType::Int32;
        }

        const int value() const {
            return value_;
        }
    };

    class BinaryExpression : public Expression {
        const Expression *lValue_;
        const OperationKind operation_;
        const Expression *rValue_;

    public:
        BinaryExpression(Expression *lValue, OperationKind operation, Expression *rValue)
                : lValue_(lValue), operation_(operation), rValue_(rValue) {
        }

        virtual ~BinaryExpression() {

        }

        ExpressionKind expressionType() const {
            return ExpressionKind::Binary;
        }

        /** The data type of an rValue expression is always the same as the rValue's data type. */
        DataType dataType() const {
            return rValue_->dataType();
        }

        const Expression *lValue() const {
            return lValue_;
        }

        const Expression *rValue() const {
            return rValue_;
        }

        OperationKind operation() const {
            return operation_;
        }
    };

    /** Contains a series of expressions, such as a method body, or one of the if-then-else parts. */
    class BlockExpression : public Expression {
        const std::vector<Expression *> expressions_;
    public:
        BlockExpression(std::vector<Expression *> expressions) : expressions_(expressions) {

        }

        virtual ~BlockExpression() {

        }

        ExpressionKind expressionType() const {
            return ExpressionKind::Block;
        }

        /** The data type of a block expression is always the data type of the last expression in the block. */
        DataType dataType() const {
            return expressions_.back()->dataType();
        }

        void forEach(std::function<void(Expression*)> func) const {
            for(Expression *expr : expressions_)
                func(expr);
        }
    };

    class ConditionalExpression : public Expression {
        Expression *condition_;
        Expression *truePart_;
        Expression *falsePart_;

    public:
        ConditionalExpression(Expression *condition, Expression *truePart, Expression *falsePart)
                : condition_(condition), truePart_(truePart), falsePart_(falsePart) {

            ARG_NOT_NULL(condition);
        }

        ExpressionKind expressionType() const {
            return ExpressionKind::Conditional;
        }

        /** The data type of a block expression is always the data type of the last expression in the block. */
        DataType dataType() const {
            if(truePart_ != nullptr) {
                return truePart_->dataType();
            } else {
                if(falsePart_ == nullptr) {
                    return DataType::Void;
                } else {
                    return falsePart_->dataType();
                }
            }
        }

        Expression *condition() {
            return condition_;
        }

        Expression *truePart() {
            return truePart_;
        }

        Expression *falsePart() {
            return falsePart_;
        }
    };

}