// https://github.com/llvm-mirror/llvm/tree/master/examples

#include "ExprRunner.hpp"
#include "ExpressionTreeVisitor.hpp"
#include "ExpressionTreeWalker.hpp"
#include "PrettyPrinter.hpp"

#include <map>

#include <stack>

#include "llvm/Analysis/Passes.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/ObjectCache.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/ExecutionEngine/GenericValue.h"

namespace llast {

    class CodeGenVisitor : public ExpressionTreeVisitor {
        llvm::LLVMContext &context_;
        llvm::IRBuilder<> irBuilder_;
        std::unique_ptr<llvm::Module> module_;
        llvm::Function* function_;
        llvm::BasicBlock *block_;

        typedef std::unordered_map<std::string, llvm::AllocaInst*> AllocaScope;

        //this is a deque and not an actual std::stack because we need the ability to iterate over it's contents
        std::deque<AllocaScope> allocaScopeStack_;
        std::stack<llvm::Value*> valueStack_;
        std::stack<const Node*> ancestryStack_;

    public:
        CodeGenVisitor(llvm::LLVMContext &context)
                : context_(context), irBuilder_{context} { }

        virtual void visitingModule(const Module *module) override {
            module_ = llvm::make_unique<llvm::Module>(module->name(), context_);
        }

        virtual void visitingFunction(const Function *func) override {
            std::vector<llvm::Type*> argTypes;

            function_ = llvm::cast<llvm::Function>(
                    module_->getOrInsertFunction(func->name(),
                    getType(func->returnType()),
                    nullptr));

            block_ = llvm::BasicBlock::Create(context_, "functionBody", function_);
            irBuilder_.SetInsertPoint(block_);


        }

        virtual void initialize() override {
            llvm::InitializeNativeTarget();
            llvm::InitializeNativeTargetAsmPrinter();
            llvm::InitializeNativeTargetAsmParser();
        }

        virtual void cleanUp() override {
            //DEBUG_ASSERT(scopeStack_.size() == 0, "When compilation complete, no scopes should remain.");
            DEBUG_ASSERT(valueStack_.size() == 0, "When compilation complete, no values should remain.");
        }

        void dumpIL() {
            std::cout << "LLVM IL:\n";
            module_->print(llvm::outs(), nullptr);
        }

    public:

        std::unique_ptr<llvm::Module> releaseLlvmModuleOwnership() {
            return std::move(module_);
        }


        virtual void visitingNode(const Node *expr) override {
            ancestryStack_.push(expr);
        }

        virtual void visitedNode(const Node *expr) override {
            DEBUG_ASSERT(ancestryStack_.top() == expr, "Top node of ancestryStack_ should be the current node.");
            UNUSED(expr);
            ancestryStack_.pop();

            //If the parent node of expr is a BlockExpr, the value left behind on valueStack_ is extraneous and
            //should be removed.  (This is a consequence of "everything is an expression.")
            if(ancestryStack_.size() >= 1
               && ancestryStack_.top()->nodeKind() == NodeKind::Block
               &&  valueStack_.size() > 0) {
                valueStack_.pop();
            }
        }

        llvm::Value *lookupVariable(const std::string &name) {
            for(auto scope = allocaScopeStack_.rbegin(); scope != allocaScopeStack_.rend(); ++scope) {
                auto foundValue = scope->find(name);
                if(foundValue != scope->end()) {
                    return foundValue->second;
                }
            }

            throw InvalidStateException(std::string("Variable '") + name + std::string("' was not defined."));
        }

        virtual void visitingBlock(const Block *expr) override {
            allocaScopeStack_.emplace_back();
            AllocaScope &topScope = allocaScopeStack_.back();

            for(auto var : expr->scope()->variables()) {
                 if(topScope.find(var->name()) != topScope .end()) {
                    throw InvalidStateException("More than one variable named '" + var->name() +
                                                        "' was defined in the current scope.");
                }

                llvm::Type *type{getType(var->dataType())};
                llvm::AllocaInst *allocaInst = irBuilder_.CreateAlloca(type, nullptr, var->name());
                topScope [var->name()] = allocaInst;
            }
        }

        llvm::Type *getType(DataType type)
        {
            switch(type) {
                case DataType::Void:
                    return llvm::Type::getVoidTy(context_);
                case DataType::Bool:
                    return llvm::Type::getInt8Ty(context_);
                case DataType::Int32:
                    return llvm::Type::getInt32Ty(context_);
                case DataType::Float:
                    return llvm::Type::getFloatTy(context_);
                case DataType::Double:
                    return llvm::Type::getDoubleTy(context_);
                default:
                    throw UnhandledSwitchCase();
            }
        }

        virtual void visitedBlock(const Block *) override {
            allocaScopeStack_.pop_back();
        }

        virtual void visitedAssignVariable(const AssignVariable *expr) override {
            llvm::Value *inst = lookupVariable(expr->name());

            llvm::Value *value = valueStack_.top();
            valueStack_.pop();

            value = irBuilder_.CreateStore(value, inst);
            valueStack_.push(value);
        }

        virtual void visitedBinary(const Binary *expr) override {

            if(expr->lValue()->dataType() != expr->rValue()->dataType()) {
                throw CompileException(CompileError::BinaryExprDataTypeMismatch,
                                       "Data types of lvalue and rvalue in binary expression do not match");
            }

            llvm::Value *rValue = valueStack_.top();
            valueStack_.pop();

            llvm::Value *lValue = valueStack_.top();
            valueStack_.pop();

            llvm::Value *result = createOperation(lValue, rValue, expr->operation(), expr->dataType());
            valueStack_.push(result);
        }


        llvm::Value *createOperation(llvm::Value *lValue, llvm::Value *rValue, OperationKind op, DataType dataType) {
            switch(dataType) {
                case DataType::Int32:
                    switch(op) {
                        case OperationKind::Add: return irBuilder_.CreateAdd(lValue, rValue);
                        case OperationKind::Sub: return irBuilder_.CreateSub(lValue, rValue);
                        case OperationKind::Mul: return irBuilder_.CreateMul(lValue, rValue);
                        case OperationKind::Div: return irBuilder_.CreateSDiv(lValue, rValue);
#pragma clang diagnostic push
#pragma ide diagnostic ignored "DuplicateSwitchCase"
                        default:
                            throw UnhandledSwitchCase();
#pragma clang diagnostic pop
                    }
                case DataType::Float:
                    switch(op) {
                        case OperationKind::Add: return irBuilder_.CreateFAdd(lValue, rValue);
                        case OperationKind::Sub: return irBuilder_.CreateFSub(lValue, rValue);
                        case OperationKind::Mul: return irBuilder_.CreateFMul(lValue, rValue);
                        case OperationKind::Div: return irBuilder_.CreateFDiv(lValue, rValue);
#pragma clang diagnostic push
#pragma ide diagnostic ignored "DuplicateSwitchCase"
                        default:
                            throw UnhandledSwitchCase();
#pragma clang diagnostic pop
                    }
                default:
                    throw UnhandledSwitchCase();
            }
        }

        virtual void visitLiteralInt32(const LiteralInt32 *expr) override {
            valueStack_.push(getConstantInt32(expr->value()));
        }

        virtual void visitLiteralFloat(const LiteralFloat *expr) override {
            valueStack_.push(getConstantFloat(expr->value()));
        }

        llvm::Value *getConstantInt32(int value) {
            return llvm::ConstantInt::get(context_, llvm::APInt(32, (uint64_t)value, true));
        }

        llvm::Value *getConstantFloat(float value) {
            return llvm::ConstantFP::get(context_, llvm::APFloat(value));
        }

        virtual void visitVariableRef(const VariableRef *expr) override {
            llvm::Value *allocaInst = lookupVariable(expr->name());
            valueStack_.push(irBuilder_.CreateLoad(allocaInst));
        }

        void visitedReturn(const Return *) override {
            DEBUG_ASSERT(valueStack_.size() > 0, "")
            llvm::Value* retValue = valueStack_.top();
            valueStack_.pop();
            irBuilder_.CreateRet(retValue);
        }

    }; // class CodeGenVisitor

    //TODO:  make this a PIMPL with a public interface
    class FunctionInvoker {
        llvm::Function *func_;
        llvm::ExecutionEngine *ee_;

        std::vector<llvm::GenericValue> args_;

    public:
        FunctionInvoker(llvm::Function *func, llvm::ExecutionEngine *ee) : func_(func), ee_(ee) {

        }

        FunctionInvoker &addArg(int value) {
            args_.emplace_back();
            args_[args_.size() - 1].IntVal = llvm::APInt(32, value);
            return *this;
        }

        float invokeFloat() {
            llvm::GenericValue gv = ee_->runFunction(func_, args_);
            return gv.FloatVal;
        }

        int invokeInt32() {
            llvm::GenericValue gv = ee_->runFunction(func_, args_);
            return (int)gv.IntVal.getSExtValue();
        }
    };

    //TODO:  make this a PIMPL with a public interface
    class ExecutionContext {
        //NOTE:  the order of definition here is significant!
        //context_ must be destroyed AFTER ee_!
        llvm::LLVMContext context_;
        unique_ptr<llvm::ExecutionEngine> ee_;

        void prettyPrint(const Module *module) {
            llast::PrettyPrinterVisitor visitor{std::cout};
            ExpressionTreeWalker walker{&visitor};
            walker.walkTree(module);
        }

    public:
        ExecutionContext() {
            //NOTE:  it is legal to call these multiple times.
            llvm::InitializeNativeTarget();
            llvm::InitializeNativeTargetAsmPrinter();
            llvm::InitializeNativeTargetAsmParser();
        }


        FunctionInvoker getFunctionInvoker(std::string name) {
            //TODO:  FindFunctionNamed() is slow...build std::unordered_map of functions
            llvm::Function *function = ee_->FindFunctionNamed(name.c_str());

            return FunctionInvoker{function, ee_.get()};
        }

        void addModule(const Module *module) {

            //prettyPrint(module);

            llast::CodeGenVisitor visitor{ context_};
            ExpressionTreeWalker walker{&visitor};
            walker.walkTree(module);
            //visitor.dumpIL();

            std::unique_ptr<llvm::Module> llvmModule = visitor.releaseLlvmModuleOwnership();
            if(!ee_) {
                llvm::EngineBuilder builder {move(llvmModule)};
                //builder.setUseOrcMCJITReplacement(true);
                ee_ = std::unique_ptr<llvm::ExecutionEngine>{builder.create()};
            } else {
                ee_->addModule(std::move(llvmModule));
            }
        }
    };

    namespace ExprRunner {
        namespace {
            const char *FUNC_NAME = "exprFunc";

            std::unique_ptr<ExecutionContext> makeExecutionContext(unique_ptr<const Expr> expr) {
                ModuleBuilder mb{"ExprModule"};

                FunctionBuilder fb{FUNC_NAME, expr->dataType()};
                BlockBuilder &bb = fb.blockBuilder();
                bb.addExpression(move(expr));

                mb.addFunction(fb.build());
                unique_ptr<const Module> module{mb.build()};

                auto ec = make_unique<ExecutionContext>();
                ec->addModule(module.get());
                return ec;
            }
        }

        void compile(unique_ptr<const Expr> expr) {
            FunctionBuilder fb{"someFunc", expr->dataType() };
            BlockBuilder &bb = fb.blockBuilder();
            bb.addExpression(move(expr));

            ModuleBuilder mb{"someModule"};
            mb.addFunction(fb.build());

            std::unique_ptr<const Module> m{mb.build()};
            llvm::LLVMContext ctx;
            llast::CodeGenVisitor visitor{ctx};
            ExpressionTreeWalker walker{&visitor};
            walker.walkTree(m.get());
        }

        float runFloatExpr(unique_ptr<const Expr> expr) {
            if(expr->dataType() != DataType::Float) {
                throw FatalException("expr->dataType() != DataType::Float");
            }
            unique_ptr<ExecutionContext> ec{makeExecutionContext(move(expr))};
            auto invoker = ec->getFunctionInvoker(FUNC_NAME);
            return invoker.invokeFloat();
        }


        int runInt32Expr(unique_ptr<const Expr> expr) {
            if(expr->dataType() != DataType::Int32) {
                throw FatalException("expr->dataType() != DataType::Int32");
            }
            unique_ptr<ExecutionContext> ec{makeExecutionContext(move(expr))};
            auto invoker = ec->getFunctionInvoker(FUNC_NAME);
            return invoker.invokeInt32();
        }
    } //namespace ExprRunner
} //namespace float
