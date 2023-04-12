
#include "llvm/Transforms/Obfuscator/IndirectBr.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Transforms/Obfuscator/CryptoUtils.h"
#include "llvm/IR/AbstractCallSite.h"
#include "llvm/IR/Attributes.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm; 
using namespace std;

static cl::opt<bool> hidebrobf("enable-hidebrobf", cl::desc("hidebrobf pass switch"), cl::init(false));

PreservedAnalyses IndirectBrPass::run(Function &F, FunctionAnalysisManager &AM) {
    if (hidebrobf) {
      doIndirectBr(F);
      return PreservedAnalyses::none();
    }
    return PreservedAnalyses::all();
}

void IndirectBrPass::doIndirectBr(Function &F) {
    LLVMContext &Ctx = F.getContext();
    Constant *offsetEncKey = ConstantInt::get(Type::getInt32Ty(Ctx), 16);
    BBNumbering.clear();
    BBTargets.clear();
    BrIns.clear();

    // collect br successors
    for(auto &BB : F) {
        for (auto &inst : BB) {
            if (BranchInst *BI = dyn_cast<BranchInst>(&inst)) {
                BrIns.push_back(BI);
                if (BI->isConditional()) {
                    for(unsigned i = 0,nums = BI->getNumSuccessors(); i < nums; i++) {
                        BasicBlock *dest = BI->getSuccessor(i);
                        if (BBNumbering.count(dest) == 0) {
                            BBNumbering[dest] = BBTargets.size();
                            BBTargets.push_back(dest); // BBTargets[BBTargets.size()] = dest;
                        }
                    }
                }
                else if (BI->isUnconditional()) {
                    //TODO: 处理非条件跳转
                    BasicBlock *dest = BI->getSuccessor(0);
                    if (BBNumbering.count(dest) == 0) {
                        BBNumbering[dest] = BBTargets.size();
                        BBTargets.push_back(dest); // BBTargets[BBTargets.size()] = dest;
                    }
                }
            }
        }
    }
    
    if (BBNumbering.empty()) {
      return ;
    }

    // save basicblock to GV
    std::string GVName(F.getName().str() + "_iBrGv");
    GlobalVariable *GV = F.getParent()->getNamedGlobal(GVName);
    if (GV == nullptr) {
        std::vector<Constant *> gvOfBB;
        for(const auto BB : BBTargets) {
            Constant *ce = ConstantExpr::getBitCast(BlockAddress::get(BB), Type::getInt8PtrTy(Ctx));
            ce = ConstantExpr::getGetElementPtr(Type::getInt8Ty(Ctx), ce, offsetEncKey);
            gvOfBB.push_back(ce);
        }
        ArrayType *ATy = ArrayType::get(Type::getInt8PtrTy(F.getContext()), gvOfBB.size());
        Constant *CA = ConstantArray::get(ATy, ArrayRef<Constant *>(gvOfBB));
        GV = new GlobalVariable(*F.getParent(), ATy, false, GlobalValue::LinkageTypes::PrivateLinkage,
                                                    CA, GVName);
        appendToCompilerUsed(*F.getParent(), {GV});
    }
    
    
    ConstantInt *Zero = ConstantInt::get(Type::getInt32Ty(Ctx), 0);
    // br function -> br GV[functionIndex]
    for(auto inst : BrIns) {
        if (BranchInst *BI = dyn_cast<BranchInst>(inst)) {
            IRBuilder<> builder(BI);
            if (BI->isConditional()) {
                Value *condition = BI->getCondition();
                Value *idx;
                Value *TIdx, *FIdx;

                TIdx = ConstantInt::get(Type::getInt32Ty(Ctx), BBNumbering[BI->getSuccessor(0)]);
                FIdx = ConstantInt::get(Type::getInt32Ty(Ctx), BBNumbering[BI->getSuccessor(1)]);
                idx = builder.CreateSelect(condition, TIdx, FIdx);
                
                Value *GEP = builder.CreateGEP(GV->getType()->getPointerElementType() ,GV, {Zero, idx});
                LoadInst *EncDestAddr = builder.CreateLoad(GEP->getType()->getPointerElementType(),GEP, "EncDestAddr");

                Constant *offset = ConstantExpr::getSub(Zero, offsetEncKey); // -offset
                Value *destAddr = builder.CreateGEP(EncDestAddr->getType()->getPointerElementType() ,EncDestAddr, offset);      //destAddr = EncDestAddr - offset

                IndirectBrInst *newBr = IndirectBrInst::Create(destAddr, 2);
                newBr->addDestination(BI->getSuccessor(0));
                newBr->addDestination(BI->getSuccessor(1));

                ReplaceInstWithInst(BI, newBr);

                //errs() << "[Conditional Br] " << *BI << " is replaced by newBr :" << *newBr << " \n";
            }
            else if (BI->isUnconditional()) {
                Value *idx = ConstantInt::get(Type::getInt32Ty(Ctx), BBNumbering[BI->getSuccessor(0)]);
                Value *GEP = builder.CreateGEP(GV->getType()->getPointerElementType(),GV, {Zero, idx});
                
                LoadInst *EncDestAddr = builder.CreateLoad( GEP->getType()->getPointerElementType(),GEP, "EncDestAddr");
                Constant *offset = ConstantExpr::getSub(Zero, offsetEncKey); // -offset
                Value *destAddr = builder.CreateGEP(EncDestAddr->getType()->getPointerElementType(),EncDestAddr, offset);      //destAddr = EncDestAddr - offset
                
                IndirectBrInst *newBr = IndirectBrInst::Create(destAddr, 1);
                newBr->addDestination(BI->getSuccessor(0));
                
                ReplaceInstWithInst(BI, newBr);
                
                //errs() << "[Unconditional Br] " << *BI << " is replaced by newBr \n";
            }
        }
    }
}
