// https://github.com/llvm-mirror/llvm/tree/master/examples

#include "ExprRunner.hpp"
#include "TreeVisitor.hpp"
#include "TreeWalker.hpp"
#include "PrettyPrinter.hpp"

#include <map>

#include <stack>


#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/RuntimeDyld.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/LambdaResolver.h"
#include "llvm/ExecutionEngine/Orc/ObjectLinkingLayer.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Mangler.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"

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
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/LambdaResolver.h"
#include "llvm/ExecutionEngine/Orc/ObjectLinkingLayer.h"
#include "llvm/IR/Mangler.h"
#include "llvm/Support/DynamicLibrary.h"



namespace llast {
    bool gShouldThrow = false;

#define DIAG_THROW() if(gShouldThrow) { std::cout << "Throwing from " << __FILE__ << ":" << __LINE__ << "\n"; throw std::runtime_error("Eeek!"); }


    class CodeGenVisitor : public TreeVisitor {
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
            //DIAG_THROW();
        }

        virtual void visitedModule(const Module *) override {
            DEBUG_ASSERT(valueStack_.size() == 0, "When compilation complete, no values should remain.");
           // DIAG_THROW();
        }

        virtual void visitingFunction(const Function *func) override {
            std::vector<llvm::Type*> argTypes;

            function_ = llvm::cast<llvm::Function>(
                    module_->getOrInsertFunction(func->name(),
                    getType(func->returnType()),
                    nullptr));

            block_ = llvm::BasicBlock::Create(context_, "functionBody", function_);
            irBuilder_.SetInsertPoint(block_);
            //DIAG_THROW();
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
            PREVENT_UNUSED_WARNING(expr);
            ancestryStack_.pop();

            //If the parent node of expr is a BlockExpr, the value left behind on valueStack_ is extraneous and
            //should be removed.  (This is a consequence of "everything is an expression.")
            if(ancestryStack_.size() >= 1
               && ancestryStack_.top()->nodeKind() == NodeKind::Block
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
//            DIAG_THROW();
        }

        virtual void visitLiteralFloat(const LiteralFloat *expr) override {
            valueStack_.push(getConstantFloat(expr->value()));
        }

        llvm::Value *getConstantInt32(int value) {
            //DIAG_THROW();
            llvm::ConstantInt *constantInt = llvm::ConstantInt::get(context_, llvm::APInt(32, value, true));
//            DIAG_THROW();
            return constantInt;
        }

        llvm::Value *getConstantFloat(float value) {
            return llvm::ConstantFP::get(context_, llvm::APFloat(value));
        }

        virtual void visitVariableRef(const VariableRef *expr) override {
            llvm::Value *allocaInst = lookupVariable(expr->name());
            valueStack_.push(irBuilder_.CreateLoad(allocaInst));
            DIAG_THROW();
        }

        void visitedReturn(const Return *) override {
            DEBUG_ASSERT(valueStack_.size() > 0, "")
            llvm::Value* retValue = valueStack_.top();
            valueStack_.pop();
            irBuilder_.CreateRet(retValue);
//            DIAG_THROW();
        }

    }; // class CodeGenVisitor

    class KaleidoscopeJIT {
    private:
        std::unique_ptr<llvm::TargetMachine> TM;
        const llvm::DataLayout DL;
        llvm::orc::ObjectLinkingLayer<> ObjectLayer;
        llvm::orc::IRCompileLayer<decltype(ObjectLayer)> CompileLayer;

    public:
        typedef llvm::orc::IRCompileLayer<decltype(ObjectLayer)>::ModuleSetHandleT ModuleHandle;

        KaleidoscopeJIT(llvm::TargetMachine *tm)
                : TM(tm),
                  DL(TM->createDataLayout()),
                  CompileLayer(ObjectLayer, llvm::orc::SimpleCompiler(*TM)) {
            llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);
        }

        llvm::TargetMachine &getTargetMachine() { return *TM; }

        ModuleHandle addModule(unique_ptr<llvm::Module> M) {
            // Build our symbol resolver:
            // Lambda 1: Look back into the JIT itself to find symbols that are part of
            //           the same "logical dylib".
            // Lambda 2: Search for external symbols in the host process.
            auto Resolver = llvm::orc::createLambdaResolver(
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

            // Build a singleton module set to hold our module.
            std::vector<unique_ptr<llvm::Module>> Ms;
            Ms.push_back(move(M));

            // Add the set to the JIT with the resolver we created above and a newly
            // created SectionMemoryManager.
            return CompileLayer.addModuleSet(move(Ms),
                                             make_unique<llvm::SectionMemoryManager>(),
                                             move(Resolver));
        }

        llvm::JITSymbol findSymbol(const std::string Name) {
            std::string MangledName;
            llvm::raw_string_ostream MangledNameStream(MangledName);
            llvm::Mangler::getNameWithPrefix(MangledNameStream, Name, DL);
            return CompileLayer.findSymbol(MangledNameStream.str(), true);
        }

        void removeModule(ModuleHandle H) {
            CompileLayer.removeModuleSet(H);
        }
    };
//    template <typename DylibLookupFtorT, typename ExternalLookupFtorT>
//    std::unique_ptr<llvm::orc::LambdaResolver<DylibLookupFtorT, ExternalLookupFtorT>>
//    createLambdaResolver2(DylibLookupFtorT DylibLookupFtor, ExternalLookupFtorT ExternalLookupFtor) {
//        typedef llvm::orc::LambdaResolver<DylibLookupFtorT, ExternalLookupFtorT> LR;
//        return std::make_unique<LR>(DylibLookupFtor, ExternalLookupFtor);
//    }


    /** This class originally taken from:
     * https://github.com/llvm-mirror/llvm/blob/release_40/examples/Kaleidoscope/include/KaleidoscopeJIT.h
     * Examples of use are here:  https://github.com/llvm-mirror/llvm/blob/release_40/examples/Kaleidoscope/Chapter4/toy.cpp
     */
//    class KaleidoscopeJIT {
//    public:
//        typedef llvm::orc::ObjectLinkingLayer<> ObjLayerT;
//        typedef llvm::orc::IRCompileLayer<ObjLayerT> CompileLayerT;
//        typedef CompileLayerT::ModuleSetHandleT ModuleHandleT;
//
//    private:
//        std::unique_ptr<llvm::TargetMachine> TM;
//        const llvm::DataLayout DL;
//        ObjLayerT ObjectLayer;
//        CompileLayerT CompileLayer;
//        std::vector<ModuleHandleT> ModuleHandles;
//
//    public:
//        KaleidoscopeJIT()
//                : TM{llvm::EngineBuilder().selectTarget()},
//                  DL{TM->createDataLayout()},
//                  CompileLayer{ObjectLayer, llvm::orc::SimpleCompiler(*TM)} {
//
//            // this call smells.  is it needed?
//            llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);
//
//        }
//
//        llvm::TargetMachine &getTargetMachine() { return *TM; }
//
//        ModuleHandleT addModule(std::unique_ptr<llvm::Module> M) {
//            // We need a memory manager to allocate memory and resolve symbols for this
//            // new module. Create one that resolves symbols by looking back into the
//            // JIT.
//            typedef std::function<llvm::JITSymbol(const std::string &)> LambdaType;
//            auto Resolver = createLambdaResolver2<LambdaType, LambdaType>(
//                    [&](const std::string &Name) -> llvm::JITSymbol {
//                        if (auto Sym = findSymbol(Name)) {
//                            return Sym;
//                        }
//                        return llvm::JITSymbol(nullptr);
//                    },
//                    [](const std::string &) -> llvm::JITSymbol {
//                        return nullptr;
//                    });
//
//            std::vector<unique_ptr<llvm::Module>> modules;
//            modules.push_back(std::move(M));
//
//            auto H = CompileLayer.addModuleSet(std::move(modules),
//                                               std::make_unique<llvm::SectionMemoryManager>(),
//                                               std::move(Resolver));
//
//            ModuleHandles.push_back(H);
//            return H;
//        }
//
//        void removeModule(ModuleHandleT H) {
//            ModuleHandles.erase(llvm::find(ModuleHandles, H));
//            CompileLayer.removeModuleSet(H);
//        }
//
//        llvm::JITSymbol findSymbol(const std::string &Name) {
//            // Search modules in reverse order: from last added to first added.
//            // This is the opposite of the usual search order for dlsym, but makes more
//            // sense in a REPL where we want to bind to the newest available definition.
//            for (auto H : llvm::make_range(ModuleHandles.rbegin(), ModuleHandles.rend()))
//                if (auto Sym = CompileLayer.findSymbolIn(H, Name, true))
//                    return Sym;
//
//            // If we can't find the symbol in the JIT, try looking in the host process.
//            if (auto SymAddr = llvm::RTDyldMemoryManager::getSymbolAddressInProcess(Name))
//                return llvm::JITSymbol(SymAddr, llvm::JITSymbolFlags::Exported);
//
//            return nullptr;
//        }
//
//    private:
//        std::string mangle(const std::string &Name) {
//            std::string MangledName;
//            {
//                llvm::raw_string_ostream MangledNameStream(MangledName);
//                llvm::Mangler::getNameWithPrefix(MangledNameStream, Name, DL);
//            }
//            return MangledName;
//        }
//
//    }; //class KaleidoscopeJIT

    //TODO:  make this a PIMPL with a public interface
    class ExecutionContext {
        //TODO:  determine if definition order (destruction order) is still significant, and if so
        //update this note to say so.

        llvm::LLVMContext context_;
        KaleidoscopeJIT *jit_;

        static void prettyPrint(const Module *module) {
            llast::PrettyPrinterVisitor visitor{std::cout};
            ExpressionTreeWalker::walkTree(visitor, module);
        }

    public:
        ExecutionContext() {
            llvm::EngineBuilder builder;

            jit_ = new KaleidoscopeJIT(builder.selectTarget());
        }

        ~ExecutionContext() {
            std::cout << "~ExecutionContext()\n";
            delete jit_;
        }

        uint64_t getSymbolAddress(const std::string &name) {
//            DIAG_THROW();
            llvm::JITSymbol symbol = jit_->findSymbol(name);
//            DIAG_THROW();
            if(!symbol)
                return 0;
//            DIAG_THROW(); this throw will not crash
            uint64_t retval = symbol.getAddress();
            DIAG_THROW(); //this throw does crash!
            return retval;
        }

        void addModule(const Module *module) {
            prettyPrint(module);
            llast::CodeGenVisitor visitor{ context_, jit_->getTargetMachine()};
            //2 DIAG_THROW();

            ExpressionTreeWalker::walkTree(visitor, module);
            //DIAG_THROW();
            visitor.dumpIR();
            unique_ptr<llvm::Module> llvmModule = visitor.releaseLlvmModuleOwnership();
//            DIAG_THROW();
            jit_->addModule(move(llvmModule));
//            DIAG_THROW();
        }
    };

    namespace ExprRunner {
        void init() {
            llvm::InitializeNativeTarget();
            llvm::InitializeNativeTargetAsmPrinter();
            llvm::InitializeNativeTargetAsmParser();
//            llvm::InitializeAllTargets();
//            llvm::InitializeAllAsmParsers();
//            llvm::InitializeAllAsmPrinters();
//            llvm::InitializeAllTargetInfos();
//            llvm::InitializeAllTargetMCs();
//
//            auto TM = llvm::EngineBuilder().selectTarget();
//            auto DL = TM->createDataLayout();
//            auto ObjectLayer = llvm::orc::SimpleCompiler(*TM);
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

            ExpressionTreeWalker::walkTree(visitor, m.get());
        }

        float runFloatExpr(unique_ptr<const Expr> expr) {
            typedef float (*FloatFuncPtr)(void);

            //throw CompileException(CompileError::BinaryExprDataTypeMismatch, "TEST!");
            if(expr->dataType() != DataType::Float) {
                throw FatalException("expr->dataType() != DataType::Float");
            }

            unique_ptr<ExecutionContext> ec{makeExecutionContext(move(expr))};
            auto funcPtr = reinterpret_cast<FloatFuncPtr>(ec->getSymbolAddress(FUNC_NAME));
            float retval = funcPtr();
            return retval;
        }

//        int runInt32Expr(unique_ptr<const Expr> expr) {
//            gShouldThrow = true;
//
//            typedef int(*IntFuncPtr)(void);
//
//            if(expr->dataType() != DataType::Int32) {
//                throw FatalException("expr->dataType() != DataType::Int32");
//            }
////            DIAG_THROW();
//
//            unique_ptr<ExecutionContext> ec{makeExecutionContext(move(expr))};
//            //DIAG_THROW();
//
//            uint64_t address = ec->getSymbolAddress(FUNC_NAME);
//            DIAG_THROW();
//
//            //return 0;
//            //https://github.com/llvm-mirror/llvm/blob/master/examples/Kaleidoscope/BuildingAJIT/Chapter1/toy.cpp#L1153
//            DEBUG_ASSERT(address != 0, "Address returned from ec->getSymbolAddress() should not be null!")
//            auto funcPtr = reinterpret_cast<IntFuncPtr>(address);
//            DIAG_THROW();
//
//            int retval = funcPtr();
//            //int retval = 0;
//            //throw CompileException(CompileError::BinaryExprDataTypeMismatch, "TEST!");
//
//            DIAG_THROW();
//            return retval;
//        }

        int runInt32Expr(unique_ptr<const Expr> expr) {


//            FunctionBuilder fb{FUNC_NAME, expr->dataType()};
//            BlockBuilder &bb = fb.blockBuilder();
//            bb.addExpression(move(expr));
//
//            ModuleBuilder mb{"ExprModule"};
//            mb.addFunction(fb.build());
//            unique_ptr<const Module> module{mb.build()};
//
//            CodeGenVisitor visitor{context, *targetMachine};
//            ExpressionTreeWalker::walkTree(visitor, module.get());
//            jit.addModule(visitor.releaseLlvmModuleOwnership());
//            gShouldThrow = true;
  //          gShouldThrow = true;

            llvm::LLVMContext context;
            auto targetMachine = llvm::EngineBuilder().selectTarget();
            KaleidoscopeJIT jit{targetMachine};

            auto module = std::make_unique<llvm::Module>("help", context);

            llvm::IRBuilder<> irBuilder{context};

            std::vector<llvm::Type*> argTypes;

            auto function = llvm::cast<llvm::Function>(
                    module->getOrInsertFunction(FUNC_NAME,
                                                llvm::Type::getInt32Ty(context),
                                                nullptr));
//            DIAG_THROW();

            auto block = llvm::BasicBlock::Create(context, "functionBody", function);
            irBuilder.SetInsertPoint(block);

            irBuilder.CreateRet(llvm::ConstantInt::get(context, llvm::APInt(32, 1, true)));

            module->setDataLayout(targetMachine->createDataLayout());

            jit.addModule(std::move(module));

            llvm::JITSymbol someFuncSymbol = jit.findSymbol(FUNC_NAME);

//            DIAG_THROW();
            uint64_t ptr = someFuncSymbol.getAddress();
            DIAG_THROW();
            std::cout << "Ptr is " << ptr << "\n";

            int (*someFuncPtr)() = reinterpret_cast<int (*)()>(ptr);

            int returnValue = someFuncPtr();

            std::cout << "Return value is: " << returnValue << "\n";

            return returnValue;
        }
    } //namespace ExprRunner
} //namespace float
