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
 */

namespace llast {

    enum class ExpressionKind {
        Binary,
        Invoke,
        VariableRef,
        Conditional,
        Switch,
        Block,
        LiteralInt32,
        AssignVariable
    };
    std::string to_string(ExpressionKind expressionType);

    enum class OperationKind {
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
        Pointer,
        Float,
        Double
    };
    std::string to_string(DataType dataType);

    /** Base class for all expressions. */
    class Expr {
    public:
        virtual ~Expr() { }
        virtual ExpressionKind expressionKind() const = 0;

        virtual DataType dataType() const { return DataType::Void; }
    };

    /** Represents an expression that is a literal 32 bit integer. */
    class LiteralInt32 : public Expr {
        int const value_;
    public:
        LiteralInt32(const int value) : value_(value) { }

        virtual ~LiteralInt32() { }

        ExpressionKind expressionKind() const {
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

        ExpressionKind expressionKind() const {
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

    /** Defines a variable or a variable reference. */
    class Variable {
        const std::string name_;
        const DataType dataType_;

    public:
        Variable(const std::string &name_, const DataType dataType) : name_(name_), dataType_(dataType) { }

        DataType dataType() const { return dataType_; }

        std::string name() const { return name_;}

        std::string toString() const {
            return name_ + ":" + to_string(dataType_);
        }

    };

    class VariableRef : public Expr {
        const Variable *variable_;
    public:
        VariableRef(const Variable * variable) : variable_{variable} {

        }

        ExpressionKind expressionKind() const { return ExpressionKind::VariableRef; }

        DataType dataType() const { return variable_->dataType(); }

        std::string name() const { return variable_->name(); }

        std::string toString() const {
            return variable_->name() + ":" + to_string(variable_->dataType());
        }
    };

    class AssignVariable : public Expr {
        const Variable *variable_;  //Note:  variables are owned by llast::Scope.
        const std::unique_ptr<const Expr> valueExpr_;
    public:
        AssignVariable(const Variable *variable_, const Expr *valueExpr)
                : variable_(variable_), valueExpr_(valueExpr) { }

        ExpressionKind expressionKind() const { return ExpressionKind::AssignVariable; }
        DataType dataType() const { return variable_->dataType(); }

        std::string name() const { return variable_->name(); }
        const Expr* valueExpr() const { return valueExpr_.get(); }
    };

    class Scope {
        const std::unordered_map<std::string, std::unique_ptr<const Variable>> variables_;
    public:
        Scope(std::unordered_map<std::string, std::unique_ptr<const Variable>> variables)
                : variables_(std::move(variables)) { }

        const Variable *findVariable(std::string &name) const {
            auto found = variables_.find(name);
            if(found == variables_.end()) {
                return nullptr;
            }
            return (*found).second.get();
        }

        std::vector<const Variable*> variables() const {
            std::vector<const Variable*> vars;

            for(auto &v : variables_) {
                vars.push_back(v.second.get());
            }

            return vars;
        }
    };

    /** Contains a series of expressions. */
    class Block : public Expr, public Scope {
        const std::vector<std::unique_ptr<const Expr>> expressions_;
    public:

        /** Note:  assumes ownership of the contents of the vector arguments. */
        Block(std::unordered_map<std::string, std::unique_ptr<const Variable>> &variables,
              std::vector<std::unique_ptr<const Expr>> &expressions)
                : Scope(std::move(variables)),
                  expressions_{std::move(expressions)} { }

        ExpressionKind expressionKind() const {
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
        std::unordered_map<std::string, std::unique_ptr<const Variable>> variables_;
    public:
        virtual ~BlockBuilder() {
        }

        BlockBuilder *addVariable(const Variable *varDecl) {
            variables_.emplace(varDecl->name(), varDecl);
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

        ExpressionKind expressionKind() const {
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
