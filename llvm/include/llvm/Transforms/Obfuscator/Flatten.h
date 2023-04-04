#ifndef LLVM_TRANSFORMS_FLATTEN_H
#define LLVM_TRANSFORMS_FLATTEN_H

#include "llvm/IR/PassManager.h" 

namespace llvm { 
    class FlattenPass : public PassInfoMixin<FlattenPass> { 
        public: 
            PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM); 
            static bool isRequired() { return true; }

        private:
            bool doFlatten(Function *f);
    };
} 

#endif  

