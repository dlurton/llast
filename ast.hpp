#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <bits/unique_ptr.h>

#include "exceptions.hpp"

/** Rules for AST nodes:
 *      - Every node shall "own" its child nodes so that when a node is deleted, all of its children are also deleted.
 *      - Thou shalt not modify any AST node after it has been created.
 *      - More rules may come at another time.
 */

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
    std::string to_string(ExpressionKind expressionType);

    enum class OperationKind {
        Assign,
        Add,
        Sub,
        Mul,
        Div
    };
    std::string to_string(OperationKind type);

    enum class DataType {
        Void,
        Bool,
        Int32,
        Pointer
    };
    std::string to_string(DataType dataType);

    /** Base class for all expressions. */
    class Expr {
    public:
        virtual ~Expr() { }
        virtual ExpressionKind expressionType() const = 0;

        virtual DataType dataType() const { return DataType::Void; }

    };

    /** Represents an expression that is a literal 32 bit integer. */
    class LiteralInt32 : public Expr {
        int const value_;
    public:
        LiteralInt32(const int value) : value_(value) { }

        virtual ~LiteralInt32() { }

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

    /** Represents a binary expression, i.e. 1 + 2 or foo + bar */
    class Binary : public Expr {
        const std::unique_ptr<const Expr> lValue_;
        const OperationKind operation_;
        const std::unique_ptr<const Expr> rValue_;
    public:

        /** Constructs a new Binary expression.  Note: assumes ownership of lValue and rValue */
        Binary(const Expr *lValue, OperationKind operation, const Expr *rValue)
                : lValue_(lValue), operation_(operation), rValue_(rValue) {
        }

        virtual ~Binary() { }

        ExpressionKind expressionType() const {
            return ExpressionKind::Binary;
        }

        /** The data type of an rValue expression is always the same as the rValue's data type. */
        DataType dataType() const {
            return rValue_->dataType();
        }

        const Expr *lValue() const {
            return lValue_.get();
        }

        const Expr *rValue() const {
            return rValue_.get();
        }

        OperationKind operation() const {
            return operation_;
        }
    };

    /** Defines a variable. */
    class Variable {
        const std::string name_;
        const DataType dataType_;

    public:
        Variable(const std::string &name_, const DataType dataType) : name_(name_), dataType_(dataType) { }

        std::string name() const { return name_;}
        DataType dataType() const { return dataType_; }
    };

    /** Contains a series of expressions. */
    class Block : public Expr {
        const std::vector<std::unique_ptr<const Expr>> expressions_;
        const std::vector<std::unique_ptr<const Variable>> variableDeclarations;
    public:

        /** Note:  assumes ownership of the contents of the vector arguments. */
        Block(std::vector<std::unique_ptr<const Variable>> &variableDeclarations,
              std::vector<std::unique_ptr<const Expr>> &expressions)
                : variableDeclarations{std::move(variableDeclarations)},
                  expressions_{std::move(expressions)} { }

        ExpressionKind expressionType() const {
            return ExpressionKind::Block;
        }

        /** The data type of a block expression is always the data type of the last expression in the block. */
        DataType dataType() const {
            return expressions_.back()->dataType();
        }

        void forEach(std::function<void(const Expr*)> func) const {
            for(auto const &expr : expressions_) {
                func(expr.get());
            }
        }
    };

    /** Helper class which makes creating Block expression instances much easier. */
    class BlockBuilder {
        std::vector<std::unique_ptr<const Expr>> expressions_;
        std::vector<std::unique_ptr<const Variable>> variables_;
    public:
        virtual ~BlockBuilder() {
        }

        BlockBuilder *addVariable(const Variable *varDecl) {
            variables_.emplace_back(varDecl);
            return this;
        }

        BlockBuilder *addExpression(const Expr *newExpr) {
            expressions_.emplace_back(newExpr);
            return this;
        }

        std::unique_ptr<Block const> build() {
            return std::make_unique<Block>(variables_, expressions_);
        }
    };


    /** Can be the basis of an if-then-else or ternary operator. */
    class Conditional : public Expr {
        std::unique_ptr<const Expr> condition_;
        std::unique_ptr<const Expr> truePart_;
        std::unique_ptr<const Expr> falsePart_;
    public:

        /** Note:  assumes ownership of condition, truePart and falsePart.  */
        Conditional(const Expr * condition, const Expr * truePart, const Expr * falsePart)
                : condition_(condition), truePart_(truePart), falsePart_(falsePart)
        {
            ARG_NOT_NULL(condition_);
        }

        ExpressionKind expressionType() const {
            return ExpressionKind::Conditional;
        }

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

        const Expr *condition() const {
            return condition_.get();
        }

        const Expr *truePart() const {
            return truePart_.get();
        }

        const Expr *falsePart() const {
            return falsePart_.get();
        }
    };
}
