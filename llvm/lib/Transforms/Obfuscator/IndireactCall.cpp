
#include "llvm/Transforms/Obfuscator/IndireactCall.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Transforms/Obfuscator/CryptoUtils.h"
#include "llvm/IR/AbstractCallSite.h"
#include "llvm/IR/Attributes.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm; 
using namespace std;

static cl::opt<bool> hidecall("enable-hidecallobf", cl::desc("hidecallobf pass switch"), cl::init(false));

PreservedAnalyses IndireactCallPass::run(Function &F, FunctionAnalysisManager &AM) {
    if (hidecall) {
      doIndireactCall(F);
      return PreservedAnalyses::none();
    }
    return PreservedAnalyses::all();
}


void IndireactCallPass::doIndireactCall(Function &Func) {
    CalleeNumbering.clear();
    FunctionOffsets.clear();
    CallSites.clear();
    Functions.clear();
    
    LLVMContext &Ctx = Func.getContext();
    
    // 计算所有的CallInst
    for(auto &block : Func) {
        for (auto &inst : block) {
            if (CallInst *ci = dyn_cast<CallInst>(&inst)) {
                Function *callee = ci->getCalledFunction();
                if (callee == nullptr || callee->isIntrinsic()) {
                    continue;
                }
                CallSites.push_back((CallInst*) &inst);
                if (CalleeNumbering.count(callee) == 0) {
                    CalleeNumbering[callee] = Functions.size();
                    FunctionOffsets[callee] = ConstantInt::get(Type::getInt32Ty(Ctx), llvm::cryptoutils->get_range(1, 10));
                    Functions.push_back(callee);
                }
            }
        }
    }
    
    // 创建gv
    std::string GVName(Func.getName().str() + "_IndiCall");
    GlobalVariable *GV = Func.getParent()->getNamedGlobal(GVName);
    if (GV == nullptr) {
        // callee's address
        std::vector<Constant *> functionSet;
        for (int i = 0; i < (int)Functions.size(); i++) {
            Constant *ce = ConstantExpr::getBitCast(Functions[i], Type::getInt8PtrTy(Func.getContext()));
            Constant *offset = FunctionOffsets[Functions[i]];
            //ce = (BYTE)ce + 5
            ce = ConstantExpr::getGetElementPtr(Type::getInt8Ty(Ctx), ce, offset);
            functionSet.push_back(ce);
        }
        ArrayType *ATy = ArrayType::get(Type::getInt8PtrTy(Ctx), functionSet.size());
        Constant *CA = ConstantArray::get(ATy, ArrayRef<Constant *>(functionSet));
        GV = new GlobalVariable(*Func.getParent(), ATy, false, GlobalValue::LinkageTypes::PrivateLinkage,
                                                        CA, GVName);
        appendToCompilerUsed(*Func.getParent(), {GV});
    }
    
    // 修改CallInst为引用到全局变量
    for (auto callinst : CallSites) {
        SmallVector<Value *, 8> Args;
        SmallVector<AttributeSet, 8> ArgAttrVec;
        CallBase *CB = dyn_cast<CallBase>(callinst);
        

        Args.clear();
        ArgAttrVec.clear();
        
        Function *destFunc = CB->getCalledFunction();
        FunctionType *fty = CB->getFunctionType();
        
        IRBuilder<> builder(callinst);
        
        // 计算函数地址
        // function = GV[destFunctionIndex]
        // functionPtr = loadToStack(function);
        Value *index = ConstantInt::get(Type::getInt32Ty(Ctx), CalleeNumbering[destFunc]);
        ConstantInt *Zero = ConstantInt::get(Type::getInt32Ty(Ctx), 0);
        Value *destFunction = builder.CreateGEP(GV->getType()->getPointerElementType(),GV, {Zero, index});
        LoadInst *destFunctionLoad = builder.CreateLoad(destFunction->getType()->getPointerElementType(), destFunction, callinst->getName());
        
        // destAddr = destFunctionLoad - offset
        Constant *offset = FunctionOffsets[destFunc];
        offset = ConstantExpr::getSub(Zero, offset);
        Value *destAddr = builder.CreateGEP(destFunctionLoad->getType()->getPointerElementType(),destFunctionLoad, offset);
        
        // 复制参数 和 参数属性
        const AttributeList &CallPAL = CB->getAttributes();
        User::op_iterator I = CB->arg_begin();
        unsigned i = 0;

        for (unsigned e = fty->getNumParams(); i != e; ++I, ++i) {
            Args.push_back(*I);
            AttributeSet Attrs = CallPAL.getAttributes(i);
            ArgAttrVec.push_back(Attrs);
        }

        for (User::op_iterator E = CB->arg_end(); I != E; ++I, ++i) {
            Args.push_back(*I);
            ArgAttrVec.push_back(CallPAL.getAttributes(i));
        }
        
        AttributeList newCallParamsAttribtes = AttributeList::get(
        callinst->getContext(), CallPAL.getFnAttrs(), CallPAL.getRetAttrs(), ArgAttrVec);
        
        // 变换load为函数指针
        Value *FnPtr = builder.CreateBitCast(destAddr, fty->getPointerTo());
        FnPtr->setName("Call_" + destFunc->getName());
        CallInst *nc = CallInst::Create(fty, FnPtr, Args, callinst->getName());
        nc->setAttributes(newCallParamsAttribtes);
        nc->setCallingConv(CB->getCallingConv());
        
        // 替换callinst为nc
        ReplaceInstWithInst(callinst, nc);
    }
}
