
#include "compiler.hpp"
#include "visitor.hpp"

#include <stack>

#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"

//#include "llvm/IR/DerivedTypes.h"
//#include "llvm/IR/LLVMContext.h"
//#include "llvm/IR/Type.h"
//#include "llvm/ADT/APFloat.h"
//#include "llvm/ADT/STLExtras.h"
//#include "llvm/IR/BasicBlock.h"
//#include "llvm/IR/Constants.h"
//#include "llvm/IR/DerivedTypes.h"
//#include "llvm/IR/Function.h"
//#include "llvm/IR/IRBuilder.h"
//#include "llvm/IR/LLVMContext.h"
//#include "llvm/IR/Module.h"
//#include "llvm/IR/Type.h"
//#include "llvm/IR/Verifier.h"

#include "llvm/Support/TargetSelect.h"


namespace llast {

    class CodeGenVisitor : public ExpressionTreeVisitor {
        llvm::LLVMContext context_;
        llvm::IRBuilder<> irBuilder_;
        std::unique_ptr<llvm::Module> module_;
        llvm::Function* function_;
        llvm::BasicBlock *block_;

        std::unordered_map<std::string, llvm::AllocaInst*> namedValues_;

        std::deque<const Scope*> scopeStack_;
        std::stack<llvm::Value*> valueStack_;
        std::stack<const Expr*> ancestryStack_;

    public:
        CodeGenVisitor() : irBuilder_(context_) { }

        virtual void initialize() {
//
//            llvm::InitializeNativeTarget();
//            llvm::InitializeNativeTargetAsmPrinter();
//            llvm::InitializeNativeTargetAsmParser();


            module_ = llvm::make_unique<llvm::Module>("llast jit", context_);
            //function_ = module_->getFunction("someFunc");
            std::vector<llvm::Type*> argTypes;
            llvm::FunctionType *ft = llvm::FunctionType::get(llvm::Type::getVoidTy(context_), argTypes, false);
            function_ = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "someFunc",
                                                          module_.get());

            block_ = llvm::BasicBlock::Create(context_, "someEntry", function_);

            irBuilder_.SetInsertPoint(block_);
        }

        virtual void cleanUp() {
            DEBUG_ASSERT(scopeStack_.size() == 0, "When compilation complete, no scopes should remain.");
            DEBUG_ASSERT(valueStack_.size() == 0, "When compilation complete, no values should remain.");
        }

        void dumpIL() {
            std::cout << "LLVM IL:\n";
            module_->print(llvm::outs(), nullptr);
        }

        virtual void visitingNode(const Expr *expr) {
            ancestryStack_.push(expr);
        }

        virtual void visitedNode(const Expr *expr) {
            DEBUG_ASSERT(ancestryStack_.top() == expr, "Top node of ancestryStack_ should be the current node.");
            ancestryStack_.pop();

            //If the parent node of expr is a BlockExpr, the value left behind on valueStack_ is extraneous and
            //should be removed.  (This is a consequence of "everything is an expression.")
            if(ancestryStack_.size() >= 1 && ancestryStack_.top()->expressionKind() == ExpressionKind::Block)
                valueStack_.pop();
        }

//        virtual void visitingConditional(const Conditional *expr) {}
//        virtual void visitedConditional(const Conditional *expr) {}
//        virtual void visitingBinary(const Binary *expr) {}

        virtual void visitingBlock(const Block *expr) {
            scopeStack_.push_back(expr);

            //TODO:  these should be scoped as well..
            for(auto var : expr->variables()) {
                if(namedValues_.find(var->name()) != namedValues_.end()) {
                    throw InvalidStateException("More than one variable named '" + var->name() +
                                                        "' was defined in the current scope.");
                }

                //TODO:  determine the actual type to use here.
                auto type = llvm::Type::getInt32Ty(context_);
                //llvm::AllocaInst *allocaInst = irBuilder_.CreateAlloca(type, getConstantInt32(0), var->name());
                llvm::AllocaInst *allocaInst = irBuilder_.CreateAlloca(type, nullptr, var->name());
                namedValues_[var->name()] = allocaInst;
            }
        }

        virtual void visitedBlock(const Block *expr) {
            scopeStack_.pop_back();
        }

//        const Variable *lookupVariable(std::string &name) {
//            for(auto itr = scopeStack_.rbegin(); itr != scopeStack_.rend(); ++itr) {
//                const Variable *var = (*itr)->findVariable(name);
//                if(var)
//                    return var;
//            }
//
//            return nullptr;
//        }

        virtual void visitedAssignVariable(const AssignVariable *expr) {
            llvm::AllocaInst *inst = findAllocaInst(expr->name());

            llvm::Value *value = valueStack_.top();
            valueStack_.pop();

            value = irBuilder_.CreateStore(value, inst);
            valueStack_.push(value);
        }

        virtual void visitedBinary(const Binary *expr) {

            llvm::Value *rValue = valueStack_.top();
            valueStack_.pop();

            llvm::Value *lValue = valueStack_.top();
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
            llvm::AllocaInst *allocaInst = findAllocaInst(expr->name());
            valueStack_.push(irBuilder_.CreateLoad(allocaInst));
        }

        llvm::AllocaInst *findAllocaInst(std::string name) const {
            //TODO:  traverse scope
            auto foundAllocaInst = namedValues_.find(name);
            if(foundAllocaInst == namedValues_.end())
                throw InvalidStateException(std::string("Ooops, variable named '") + name +
                                            std::string("' was not added to the current scope"));

            return foundAllocaInst->second;
        }
    }; // class CodeGenVisitor

    /** This method just demonstrates the process of compilation, for now */
    void EmbryonicCompiler::compileEmbryonically(const Expr *expr) {

        //Generate LLVM IL
        CodeGenVisitor visitor;
        ExpressionTreeWalker walker(&visitor);
        walker.walkTree(expr);

        //Display the LLVM IL code to the console.
        visitor.dumpIL();
    }

}