#ifndef LLVM_TRANSFORMS_StringEncryptionPass_H 

#define LLVM_TRANSFORMS_StringEncryptionPass_H 

#include "llvm/IR/PassManager.h" 
#include "llvm/Pass.h"
#include "llvm/IR/Constants.h"
#include <map>
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>

namespace llvm { 

  class StringEncryptionPass : public PassInfoMixin<StringEncryptionPass> { 

    std::map<Function *, GlobalVariable *> encstatus;

   public: 

    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM); 

    static bool isRequired() { return true; } 

   private:
     

     void HandleFunction(Function *Func);

     GlobalVariable *ObjectivCString(GlobalVariable *GV, std::string name, GlobalVariable *oldrawString, GlobalVariable *newString, ConstantStruct *CS);

     void HandleDecryptionBlock(BasicBlock *B, BasicBlock *C,
                             std::map<GlobalVariable *, std::pair<Constant *, GlobalVariable *>> &GV2Keys);

  };

} 

#endif  

