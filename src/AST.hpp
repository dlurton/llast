#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

#include "Exception.hpp"

/** Rules for AST nodes:
 *      - Every node shall "own" its child nodes so that when a node is deleted, all of its children are also deleted.
 *      - Thou shalt not modify any AST node after it has been created.
 */

namespace llast {

    using std::make_unique;
    using std::unique_ptr;
    using std::make_shared;
    using std::shared_ptr;
    using std::move;
    using std::string;

    enum class NodeKind {
        Binary,
        Invoke,
        VariableRef,
        Conditional,
        Switch,
        Block,
        LiteralInt32,
        LiteralFloat,
        AssignVariable,
        Return,
        Module,
        Function
    };
    string to_string(NodeKind nodeKind);

    enum class OperationKind {
        Add,
        Sub,
        Mul,
        Div
    };
    string to_string(OperationKind type);

    enum class DataType {
        Void,
        Bool,
        Int32,
        Pointer,
        Float,
        Double
    };
    string to_string(DataType dataType);

    /** Base class for all nodes */
    class Node {
    public:
        virtual ~Node() { }
        virtual NodeKind nodeKind() const = 0;
    };

    /** Base class for all expressions. */
    class Expr : public Node {
    public:
        virtual ~Expr() { }

        virtual DataType dataType() const { return DataType::Void; }
    };

    /** Represents an expression that is a literal 32 bit integer. */
    class LiteralInt32 : public Expr {
        int const value_;
    public:
        LiteralInt32(const int value) : value_(value) { }

        virtual ~LiteralInt32() { }

        NodeKind nodeKind() const override { return NodeKind::LiteralInt32; }

        DataType dataType() const override {
            return DataType::Int32;
        }

        int value() const {
            return value_;
        }

        static std::unique_ptr<LiteralInt32> make(int value) {
            return std::make_unique<LiteralInt32>(value);
        }
    };


    /** Represents an expression that is a literal float. */
    class LiteralFloat : public Expr {
        float const value_;
    public:
        LiteralFloat(const float value) : value_(value) { }

        virtual ~LiteralFloat() { }

        NodeKind nodeKind() const override {
            return NodeKind::LiteralFloat;
        }

        DataType dataType() const override {
            return DataType::Float;
        }

        float value() const {
            return value_;
        }

        static std::unique_ptr<LiteralFloat> make(float value) {
            return std::make_unique<LiteralFloat>(value);
        }
    };

    /** Represents a binary expression, i.e. 1 + 2 or foo + bar */
    class Binary : public Expr {
        const unique_ptr<const Expr> lValue_;
        const OperationKind operation_;
        const unique_ptr<const Expr> rValue_;
    public:

        /** Constructs a new Binary expression.  Note: assumes ownership of lValue and rValue */
        Binary(unique_ptr<const Expr> lValue, OperationKind operation, unique_ptr<const Expr> rValue)
                : lValue_(move(lValue)), operation_(operation), rValue_(move(rValue)) {
        }

        virtual ~Binary() { }

        NodeKind nodeKind() const override {
            return NodeKind::Binary;
        }

        /** The data type of an rValue expression is always the same as the rValue's data type. */
        DataType dataType() const override {
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

        static std::unique_ptr<Binary> make(std::unique_ptr<const Expr> lvalue,
                                          OperationKind operation,
                                          std::unique_ptr<const Expr> rvalue) {

            return std::make_unique<Binary>(move(lvalue), operation, move(rvalue));
        }
    };


    /** Defines a variable or a variable reference. */
    class Variable {
        const string name_;
        const DataType dataType_;

    public:
        Variable(const string &name_, const DataType dataType) : name_(name_), dataType_(dataType) { }

        DataType dataType() const { return dataType_; }

        string name() const { return name_;}

        string toString() const {
            return name_ + ":" + to_string(dataType_);
        }
    };

    class VariableRef : public Expr {
        shared_ptr<const Variable> variable_;
    public:
        VariableRef(shared_ptr<const Variable> variable) : variable_{variable} {

        }

        NodeKind nodeKind() const override { return NodeKind::VariableRef; }

        DataType dataType() const override { return variable_->dataType(); }

        string name() const { return variable_->name(); }

        string toString() const {
            return variable_->name() + ":" + to_string(variable_->dataType());
        }
    };

    class AssignVariable : public Expr {
        shared_ptr<const Variable> variable_;  //Note:  variables are owned by llast::Scope.
        const unique_ptr<const Expr> valueExpr_;
    public:
        AssignVariable(shared_ptr<const Variable> variable, unique_ptr<const Expr> valueExpr)
                : variable_(variable), valueExpr_(move(valueExpr)) { }

        NodeKind nodeKind() const override { return NodeKind::AssignVariable; }
        DataType dataType() const override { return variable_->dataType(); }

        string name() const { return variable_->name(); }
        const Expr* valueExpr() const { return valueExpr_.get(); }

        static std::unique_ptr<AssignVariable> make(shared_ptr<const Variable> variable,
                                                    unique_ptr<const Expr> valueExpr) {

            return std::make_unique<AssignVariable>(move(variable), move(valueExpr));
        }
    };

    class Return : public Expr {
        const unique_ptr<const Expr> valueExpr_;
    public:
        Return(unique_ptr<const Expr> valueExpr) : valueExpr_(move(valueExpr)) { }

        NodeKind nodeKind() const override { return NodeKind::Return; }
        DataType dataType() const override { return valueExpr_->dataType(); }

        const Expr* valueExpr() const { return valueExpr_.get(); }

        static std::unique_ptr<Return> make(unique_ptr<const Expr> valueExpr) {
            return std::make_unique<Return>(move(valueExpr));
        }
    };

    class Scope {
        const std::unordered_map<string, shared_ptr<const Variable>> variables_;
    public:
        Scope(std::unordered_map<string, shared_ptr<const Variable>> variables)
                : variables_{move(variables)} { }

        virtual ~Scope() {}

        const Variable *findVariable(string &name) const {
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

    class ScopeBuilder {
        std::unordered_map<string, shared_ptr<const Variable>> variables_;
    public:

        ScopeBuilder &addVariable(shared_ptr<const Variable> varDecl) {
            variables_.emplace(varDecl->name(), varDecl);
            return *this;
        }

        unique_ptr<const Scope> build() {
            return make_unique<Scope>(move(variables_));
        }
    };

    /** Contains a series of expressions. */
    class Block : public Expr {
        const std::vector<unique_ptr<const Expr>> expressions_;
        unique_ptr<const Scope> scope_;
    public:
        virtual ~Block() {}

        /** Note:  assumes ownership of the contents of the vector arguments. */
        Block(unique_ptr<const Scope> scope, std::vector<unique_ptr<const Expr>> expressions)
                : expressions_{move(expressions)}, scope_{move(scope)} { }

        NodeKind nodeKind() const override { return NodeKind::Block; }

        /** The data type of a block expression is always the data type of the last expression in the block. */
        DataType dataType() const override {
            return expressions_.back()->dataType();
        }

        const Scope *scope() const { return scope_.get(); }

        void forEach(std::function<void(const Expr*)> func) const {
            for(auto const &expr : expressions_) {
                func(expr.get());
            }
        }
    };

    /** Helper class which makes creating Block expression instances much easier. */
    class BlockBuilder {
        std::vector<unique_ptr<const Expr>> expressions_;
        ScopeBuilder scopeBuilder_;
    public:
        virtual ~BlockBuilder() {
        }

        BlockBuilder &addVariable(shared_ptr<const Variable> variable) {
            scopeBuilder_.addVariable(variable);
            return *this;
        }

        BlockBuilder &addExpression(unique_ptr<const Expr> newExpr) {
            expressions_.emplace_back(move(newExpr));
            return *this;
        }

        unique_ptr<const Block> build() {
            return make_unique<const Block>(scopeBuilder_.build(), move(expressions_));
        }
    };


    /** Can be the basis of an if-then-else or ternary operator. */
    class Conditional : public Expr {
        unique_ptr<const Expr> condition_;
        unique_ptr<const Expr> truePart_;
        unique_ptr<const Expr> falsePart_;
    public:

        /** Note:  assumes ownership of condition, truePart and falsePart.  */
        Conditional(unique_ptr<const Expr> condition,
                    unique_ptr<const Expr> truePart,
                    unique_ptr<const Expr> falsePart)
                : condition_{move(condition)},
                  truePart_{move(truePart)},
                  falsePart_{move(falsePart)}
        {
            ARG_NOT_NULL(condition_);
        }

        NodeKind nodeKind() const override {
            return NodeKind::Conditional;
        }

        DataType dataType() const override {
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

    class Function : public Node {
        const string name_;
        const DataType returnType_;
        unique_ptr<const Scope> parameterScope_;
        unique_ptr<const Expr> body_;

    public:
        Function(string name,
                 DataType returnType,
                 unique_ptr<const Scope> parameterScope,
                 unique_ptr<const Expr> body)
                : name_{name},
                  returnType_{returnType},
                  parameterScope_{move(parameterScope)},
                  body_{move(body)} {
        }

        NodeKind nodeKind() const override { return NodeKind::Function; }

        string name() const { return name_; }

        DataType returnType() const { return returnType_; }

        const Scope *parameterScope() const { return parameterScope_.get(); };

        const Expr *body() const { return body_.get(); }

    };

    class FunctionBuilder {
        const string name_;
        const DataType returnType_;

        BlockBuilder blockBuilder_;
        ScopeBuilder parameterScopeBuilder_;
    public:
        FunctionBuilder(string name, DataType returnType)
            : name_{name}, returnType_{returnType} { }

        BlockBuilder &blockBuilder() {
            return blockBuilder_;
        }

        /** Assumes ownership of the variable. */
        FunctionBuilder &addParameter(unique_ptr<const Variable> variable) {
            parameterScopeBuilder_.addVariable(move(variable));
            return *this;
        };

        unique_ptr<const Function> build() {
            return make_unique<Function>(name_,
                                         returnType_,
                                         parameterScopeBuilder_.build(),
                                         blockBuilder_.build());
        }
    };

    class Module : public Node {
        const string name_;
        const std::vector<unique_ptr<const Function>> functions_;
    public:
        Module(string name, std::vector<unique_ptr<const Function>> functions)
             : name_{name}, functions_{move(functions)}  { }

        NodeKind nodeKind() const override { return NodeKind::Module; }

        string name() const { return name_; }

        void forEachFunction(std::function<void(const Function *)> func) const {
            for(const auto &f : functions_) {
                func(f.get());
            }
        }
    };

    class ModuleBuilder {
        const string name_;
        std::vector<unique_ptr<const Function>> functions_;
    public:
        ModuleBuilder(string name)
            : name_{name}
        { }

        ModuleBuilder &addFunction(unique_ptr<const Function> function) {
            functions_.emplace_back(move(function));
            return *this;
        }

        ModuleBuilder &addFunction(Function *function) {
            functions_.emplace_back(function);
            return *this;
        }

        unique_ptr<const Module> build() {
            return make_unique<const Module>(name_, move(functions_));
        }
    };
}
