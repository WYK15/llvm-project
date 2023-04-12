
#include "llvm/Transforms/Obfuscator/IndireactGlobalVariable.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Obfuscator/CryptoUtils.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/IR/InstIterator.h"

using namespace llvm;
using namespace std;

static cl::opt<bool> hideGVobf("enable-hideGVobf",
                               cl::desc("hideGVobf pass switch"),
                               cl::init(false));

PreservedAnalyses
IndireactGlobalVariablePass::run(Function &F, FunctionAnalysisManager &AM) {
  if (hideGVobf) {
    doIndireactGlobalVariable(F);
    return PreservedAnalyses::none();
  }
  return PreservedAnalyses::all();
}

void IndireactGlobalVariablePass::doIndireactGlobalVariable(Function &F) {
  GVNumbering.clear();
  GVOffsets.clear();
  GVs.clear();

  LLVMContext &Ctx = F.getContext();
  // 收集引用的GlobalVariable
  for (auto &BB : F) {
    for (auto &inst : BB) {
      for (User::op_iterator begin = inst.op_begin(), end = inst.op_end();
           begin != end; begin++) {
        Value *oprand = *begin;
        if (GlobalVariable *GV = dyn_cast<GlobalVariable>(oprand)) {
          if (!GV->isThreadLocal() && GVNumbering.count(GV) == 0) {
            GVNumbering[GV] = GVs.size();
            GVs.push_back(GV);
            GVOffsets[GV] =
                ConstantInt::get(Type::getInt32Ty(Ctx),
                                 llvm::cryptoutils->get_range(2, 32));
          }
        }
      }
    }
  }

  if (GVs.empty()) {
    return;
  }

  // 将引用的GV转化为指针存入新的GV数组中
  std::string GVName(F.getName().str() + "_IndirectGVars");
  GlobalVariable *GV = F.getParent()->getNamedGlobal(GVName);
  if (GV == nullptr) {
    std::vector<Constant *> Elements;
    for (auto gv : GVs) {
      Constant *CE =
          ConstantExpr::getBitCast(gv, Type::getInt8PtrTy(F.getContext()));
      CE = ConstantExpr::getGetElementPtr(
          Type::getInt8Ty(Ctx), CE, GVOffsets[gv]); // CE = (BYTE) CE + offset
      Elements.push_back(CE);
    }

    // save to  globalVariable, type : array
    ArrayType *ATy =
        ArrayType::get(Type::getInt8PtrTy(F.getContext()), Elements.size());
    Constant *CA = ConstantArray::get(ATy, ArrayRef<Constant *>(Elements));
    GV = new GlobalVariable(*F.getParent(), ATy, false,
                            GlobalValue::LinkageTypes::PrivateLinkage, CA,
                            GVName);
    appendToCompilerUsed(*F.getParent(), {GV});
  }

  ConstantInt *Zero = ConstantInt::get(Type::getInt32Ty(Ctx), 0);
  // 遍历所有的inst, 修改有GV引用的指令，将GV的引用修改为创建的array的引用
  for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; I++) {
    Instruction *inst = &*I;
    if (PHINode *PHI = dyn_cast<PHINode>(inst)) {
      // TODO: 修改PHINode类型的指令对GV的引用
      //                for (unsigned int i = 0; i <
      //                PHI->getNumIncomingValues(); ++i) {
      //                    Value *val = PHI->getIncomingValue(i);
      //                    if (GlobalVariable *gval =
      //                    dyn_cast<GlobalVariable>(val)) {
      //                        if (GVNumbering.count(gval) == 0) {
      //                            continue;
      //                        }
      //
      //                        // 在incoming块中，修改
      //                        Instruction *IP =
      //                        PHI->getIncomingBlock(i)->getTerminator();
      //                        IRBuilder<> IRB(IP);
      //                    }
      //                }
    } else {
      for (User::op_iterator opbegin = inst->op_begin(), opend = inst->op_end();
           opbegin != opend; opbegin++) {
        if (GlobalVariable *gval = dyn_cast<GlobalVariable>(*opbegin)) {
          if (GVNumbering.count(gval) == 0) {
            continue;
          }

          IRBuilder<> builder(inst);
          Value *idx =
              ConstantInt::get(Type::getInt32Ty(Ctx), GVNumbering[gval]);
          Value *gep = builder.CreateGEP( GV->getType()->getPointerElementType() ,GV, {Zero, idx});
          LoadInst *EncGVAddr = builder.CreateLoad(gep->getType()->getPointerElementType() ,gep, GV->getName());

          Constant *offset =
              ConstantExpr::getSub(Zero, GVOffsets[gval]); // -offset
          Value *originGvAddr = builder.CreateGEP( EncGVAddr->getType()->getPointerElementType(), EncGVAddr, offset); // destAddr = EncDestAddr - offset

          originGvAddr = builder.CreateBitCast(originGvAddr, gval->getType());
          originGvAddr->setName("IndGv");

          // errs() << "gval : " << *gval << " is replaced by : " <<
          // *originGvAddr << "\n";
          inst->replaceUsesOfWith(gval, originGvAddr);
        }
      }
    }
  }
}