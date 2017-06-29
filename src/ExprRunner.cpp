#include "ExprRunner.hpp"
#include "ExpressionTreeVisitor.hpp"
#include "ExpressionTreeWalker.hpp"
#include "PrettyPrinter.hpp"

#include <map>
#include <stack>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Mangler.h"

#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/DynamicLibrary.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/RuntimeDyld.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"

#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/LambdaResolver.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"

#pragma GCC diagnostic pop

namespace llast {
    class CodeGenVisitor : public ExpressionTreeVisitor {
        llvm::LLVMContext &context_;
        llvm::TargetMachine &targetMachine_;
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
        CodeGenVisitor(llvm::LLVMContext &context, llvm::TargetMachine &targetMachine)
                : context_{context}, targetMachine_{targetMachine}, irBuilder_{context} { }

        virtual void visitingModule(const Module *module) override {
            module_ = llvm::make_unique<llvm::Module>(module->name(), context_);
            module_->setDataLayout(targetMachine_.createDataLayout());
        }

        virtual void visitedModule(const Module *) override {
            DEBUG_ASSERT(valueStack_.size() == 0, "When compilation complete, no values should remain.");
        }

        virtual void visitingFunction(const Function *func) override {
            std::vector<llvm::Type*> argTypes;

            function_ = llvm::cast<llvm::Function>(
                    module_->getOrInsertFunction(func->name(),
                                                 getType(func->returnType())));

            block_ = llvm::BasicBlock::Create(context_, "functionBody", function_);
            irBuilder_.SetInsertPoint(block_);
        }

        void dumpIR() {
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
            ancestryStack_.pop();

            //If the parent node of expr is a BlockExpr, the value left behind on valueStack_ is extraneous and
            //should be removed.  (This is a consequence of "everything is an expression.")
            if(ancestryStack_.size() >= 1
               && expr->nodeKind() == NodeKind::Block
               && valueStack_.size() > 0) {
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
                if(topScope.find(var->name()) != topScope.end()) {
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
                //throw std::runtime_error("Crapola");
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
                        default:
                            throw UnhandledSwitchCase();
                    }
                case DataType::Float:
                    switch(op) {
                        case OperationKind::Add: return irBuilder_.CreateFAdd(lValue, rValue);
                        case OperationKind::Sub: return irBuilder_.CreateFSub(lValue, rValue);
                        case OperationKind::Mul: return irBuilder_.CreateFMul(lValue, rValue);
                        case OperationKind::Div: return irBuilder_.CreateFDiv(lValue, rValue);
                        default:
                            throw UnhandledSwitchCase();
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
            llvm::ConstantInt *constantInt = llvm::ConstantInt::get(context_, llvm::APInt(32, value, true));
            return constantInt;
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


    //The llvm::orc::createResolver(...) version of this doesn't seem to work for some reason...
    template <typename DylibLookupFtorT, typename ExternalLookupFtorT>
    std::unique_ptr<llvm::orc::LambdaResolver<DylibLookupFtorT, ExternalLookupFtorT>>
    createLambdaResolver2(DylibLookupFtorT DylibLookupFtor, ExternalLookupFtorT ExternalLookupFtor) {
        typedef llvm::orc::LambdaResolver<DylibLookupFtorT, ExternalLookupFtorT> LR;
        return std::make_unique<LR>(DylibLookupFtor, ExternalLookupFtor);
    }

    /** This class originally taken from:
     * https://github.com/llvm-mirror/llvm/blob/master/examples/Kaleidoscope/include/KaleidoscopeJIT.h
     */
    class SimpleJIT {
    private:
        std::unique_ptr<llvm::TargetMachine> TM;
        const llvm::DataLayout DL;
        llvm::orc::RTDyldObjectLinkingLayer ObjectLayer;
        llvm::orc::IRCompileLayer<decltype(ObjectLayer), llvm::orc::SimpleCompiler> CompileLayer;

    public:
        using ModuleHandle = decltype(CompileLayer)::ModuleHandleT;

        SimpleJIT()
                : TM(llvm::EngineBuilder().selectTarget()), DL(TM->createDataLayout()),
                  CompileLayer(ObjectLayer, llvm::orc::SimpleCompiler(*TM)) {
            llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);
        }

        llvm::TargetMachine &getTargetMachine() { return *TM; }

        ModuleHandle addModule(std::shared_ptr<llvm::Module> M) {
            // Build our symbol resolver:
            // Lambda 1: Look back into the JIT itself to find symbols that are part of
            //           the same "logical dylib".
            // Lambda 2: Search for external symbols in the host process.
            auto Resolver = createLambdaResolver2(
                    [&](const std::string &Name) {
                        if (auto Sym = CompileLayer.findSymbol(Name, false))
                            return Sym;
                        return llvm::JITSymbol(nullptr);
                    },
                    [](const std::string &Name) {
                        if (auto SymAddr =
                                llvm::RTDyldMemoryManager::getSymbolAddressInProcess(Name))
                            return llvm::JITSymbol(SymAddr, llvm::JITSymbolFlags::Exported);
                        return llvm::JITSymbol(nullptr);
                    });

            // Add the set to the JIT with the resolver we created above and a newly
            // created SectionMemoryManager.
            return CompileLayer.addModule(M, std::make_unique<llvm::SectionMemoryManager>(), std::move(Resolver));
        }

        llvm::JITSymbol findSymbol(const std::string Name) {
            std::string MangledName;
            llvm::raw_string_ostream MangledNameStream(MangledName);
            llvm::Mangler::getNameWithPrefix(MangledNameStream, Name, DL);
            return CompileLayer.findSymbol(MangledNameStream.str(), true);
        }

        void removeModule(ModuleHandle H) {
            CompileLayer.removeModule(H);
        }
    }; //SimpleJIT

    //TODO:  make this a PIMPL with a public interface
    class ExecutionContext {
        //TODO:  determine if definition order (destruction order) is still significant, and if so
        //update this note to say so.

        llvm::LLVMContext context_;
        std::unique_ptr<SimpleJIT> jit_ = std::make_unique<SimpleJIT>();

        static void prettyPrint(const Module *module) {
            llast::PrettyPrinterVisitor visitor{std::cout};
            ExpressionTreeWalker walker{&visitor};
            walker.walkTree(module);
        }

    public:

        uint64_t getSymbolAddress(const std::string &name) {
            llvm::JITSymbol symbol = jit_->findSymbol(name);

            if(!symbol)
                return 0;

            uint64_t retval = symbol.getAddress();
            return retval;
        }

        void addModule(const Module *module) {
            //prettyPrint(module);
            llast::CodeGenVisitor visitor{ context_, jit_->getTargetMachine()};
            ExpressionTreeWalker walker{&visitor};
            walker.walkTree(module);

            //visitor.dumpIR();
            unique_ptr<llvm::Module> llvmModule = visitor.releaseLlvmModuleOwnership();
            jit_->addModule(move(llvmModule));
        }
    };

    namespace ExprRunner {

        void init() {
            llvm::InitializeNativeTarget();
            llvm::InitializeNativeTargetAsmPrinter();
            llvm::InitializeNativeTargetAsmParser();
        }

        namespace {
            const string FUNC_NAME = "exprFunc";

            std::unique_ptr<ExecutionContext> makeExecutionContext(unique_ptr<const Expr> expr) {
                FunctionBuilder fb{FUNC_NAME, expr->dataType()};
                BlockBuilder &bb = fb.blockBuilder();
                bb.addExpression(move(expr));

                ModuleBuilder mb{"ExprModule"};
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
            auto tm = unique_ptr<llvm::TargetMachine>(llvm::EngineBuilder().selectTarget());
            llast::CodeGenVisitor visitor{ctx, *tm.get()};
            ExpressionTreeWalker walker{&visitor};
            walker.walkTree(m.get());
        }

        float runFloatExpr(unique_ptr<const Expr> expr) {
            typedef float (*FloatFuncPtr)(void);

            if(expr->dataType() != DataType::Float) {
                throw FatalException("expr->dataType() != DataType::Float");
            }

            unique_ptr<ExecutionContext> ec{makeExecutionContext(move(expr))};
            auto funcPtr = reinterpret_cast<FloatFuncPtr>(ec->getSymbolAddress(FUNC_NAME));
            float retval = funcPtr();
            return retval;
        }

        int runInt32Expr(unique_ptr<const Expr> expr) {
            typedef int (*IntFuncPtr)(void);

            if(expr->dataType() != DataType::Int32) {
                throw FatalException("expr->dataType() != DataType::Int32");
            }

            unique_ptr<ExecutionContext> ec{makeExecutionContext(move(expr))};
            auto funcPtr = reinterpret_cast<IntFuncPtr>(ec->getSymbolAddress(FUNC_NAME));
            int retval = funcPtr();
            return retval;
        }
    } //namespace ExprRunner
} //namespace float

