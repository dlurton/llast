// https://github.com/llvm-mirror/llvm/tree/master/examples


#include "EmbryonicCompiler.hpp"
#include "ExpressionTreeVisitor.hpp"

#include <stack>
#include <algorithm>
#include <memory>


#include <llvm/Support/DynamicLibrary.h>

#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/RTDyldMemoryManager.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/LambdaResolver.h"
#include "llvm/ExecutionEngine/Orc/ObjectLinkingLayer.h"
#include "llvm/IR/Mangler.h"

#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/Support/TargetSelect.h"
#include "ExpressionTreeWalker.hpp"


namespace llast {

    class CodeGenVisitor : public ExpressionTreeVisitor {
        const std::string moduleName_;
        llvm::LLVMContext context_;
        llvm::IRBuilder<> irBuilder_;
        std::unique_ptr<llvm::Module> module_;
        llvm::Function* function_;
        llvm::BasicBlock *block_;

        typedef std::unordered_map<std::string, llvm::AllocaInst*> AllocaScope;

        //this is a deque and not an actual std::stack because we need the ability to iterate over it's contents
        std::deque<AllocaScope> allocaScopeStack_;
        std::stack<llvm::Value*> valueStack_;
        std::stack<const Expr*> ancestryStack_;

    public:
        CodeGenVisitor(std::string moduleName) : moduleName_{moduleName}, irBuilder_(context_) { }

        virtual void initialize() {
            module_ = llvm::make_unique<llvm::Module>(moduleName_, context_);
            std::vector<llvm::Type*> argTypes;
            llvm::FunctionType *ft = llvm::FunctionType::get(llvm::Type::getInt32Ty(context_), argTypes, false);
            function_ = llvm::cast<llvm::Function>(module_->getOrInsertFunction("someFunc", ft));

            block_ = llvm::BasicBlock::Create(context_, "someEntry", function_);

            irBuilder_.SetInsertPoint(block_);
        }

        virtual void cleanUp() {
            //DEBUG_ASSERT(scopeStack_.size() == 0, "When compilation complete, no scopes should remain.");
            DEBUG_ASSERT(valueStack_.size() == 0, "When compilation complete, no values should remain.");
        }

        void dumpIL() {
            std::cout << "LLVM IL:\n";
            module_->print(llvm::outs(), nullptr);
        }

        int execute() {
            llvm::ExecutionEngine *EE =  llvm::EngineBuilder(std::move(module_)).create();

            std::vector<llvm::GenericValue> noargs;
            llvm::GenericValue gv = EE->runFunction(function_, noargs);

            // Import result of execution:
            //llvm::outs() << "Result: " << gv.IntVal << "\n";

            delete EE;

            uint64_t retval = *gv.IntVal.getRawData();

            return retval;
        }

        virtual void visitingNode(const Expr *expr) {
            ancestryStack_.push(expr);
        }

        virtual void visitedNode(const Expr *expr) {
            DEBUG_ASSERT(ancestryStack_.top() == expr, "Top node of ancestryStack_ should be the current node.");
            ancestryStack_.pop();

            //If the parent node of expr is a BlockExpr, the value left behind on valueStack_ is extraneous and
            //should be removed.  (This is a consequence of "everything is an expression.")
            if(ancestryStack_.size() >= 1
               && ancestryStack_.top()->expressionKind() == ExpressionKind::Block
               &&  valueStack_.size() > 0) {
                valueStack_.pop();
            }
        }

//        virtual void visitingConditional(const Conditional *expr) {}
//        virtual void visitedConditional(const Conditional *expr) {}
//        virtual void visitingBinary(const Binary *expr) {}

        llvm::Value *lookupVariable(const std::string &name) {
            for(auto scope = allocaScopeStack_.rbegin(); scope != allocaScopeStack_.rend(); ++scope) {
                auto foundValue = scope->find(name);
                if(foundValue != scope->end()) {
                    return foundValue->second;
                }
            }

            throw InvalidStateException(std::string("Variable '") + name + std::string("' was not defined."));
        }

        virtual void visitingBlock(const Block *expr) {
            allocaScopeStack_.emplace_back();
            AllocaScope &topScope = allocaScopeStack_.back();

            for(auto var : expr->variables()) {
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

        virtual void visitedBlock(const Block *expr) {
            allocaScopeStack_.pop_back();
        }

        virtual void visitedAssignVariable(const AssignVariable *expr) {
            llvm::Value *inst = lookupVariable(expr->name());

            llvm::Value *value = valueStack_.top();
            valueStack_.pop();

            value = irBuilder_.CreateStore(value, inst);
            valueStack_.push(value);
        }

        virtual void visitedBinary(const Binary *expr) {

            llvm::Value *lValue = valueStack_.top();
            valueStack_.pop();

            llvm::Value *rValue = valueStack_.top();
            valueStack_.pop();

            llvm::Value *result = createOperation(lValue, expr->operation(), rValue);
            valueStack_.push(result);
        }

        llvm::Value *createOperation(llvm::Value *lValue, OperationKind op, llvm::Value *rValue) {
            switch(op) {
                case OperationKind::Add: return irBuilder_.CreateAdd(lValue, rValue);
                case OperationKind::Sub: return irBuilder_.CreateSub(lValue, rValue);
                case OperationKind::Mul: return irBuilder_.CreateMul(lValue, rValue);
                case OperationKind::Div: return irBuilder_.CreateSDiv(lValue, rValue);
                default:
                    throw UnhandledSwitchCase();
            }
        }

        virtual void visitLiteralInt32(const LiteralInt32 *expr) {
            valueStack_.push(getConstantInt32(expr->value()));
        }

        llvm::ConstantInt *getConstantInt32(int value) {
            return llvm::ConstantInt::get(context_, llvm::APInt(32, value, true));
        }

        virtual void visitVariableRef(const VariableRef *expr) {
            llvm::Value *allocaInst = lookupVariable(expr->name());
            valueStack_.push(irBuilder_.CreateLoad(allocaInst));
        }

        void visitedReturn(const Return *expr) {
            DEBUG_ASSERT(valueStack_.size() > 0, "")
            llvm::Value* retValue = valueStack_.top();
            valueStack_.pop();
            irBuilder_.CreateRet(retValue);
        }

    }; // class CodeGenVisitor

    /** This method just demonstrates the process of compilation, for now */
    int EmbryonicCompiler::compileAndExecute(const Expr *expr) {

        //Generate LLVM IL
        llast::CodeGenVisitor visitor{"EmbryonicModule"};
        ExpressionTreeWalker walker{&visitor};
        walker.walkTree(expr);

        //Display the LLVM IL code to the console.
        //visitor.dumpIL();
        return visitor.execute();
    }

}