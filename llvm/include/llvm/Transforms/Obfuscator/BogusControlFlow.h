#ifndef LLVM_TRANSFORMS_BCF_H
#define LLVM_TRANSFORMS_BCF_H

#include "llvm/IR/PassManager.h" 

namespace llvm { 
    class BogusControlFlowPass : public PassInfoMixin<BogusControlFlowPass> { 
        public: 
            PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM); 
            static bool isRequired() { return true; }

        private:
            bool doBcf(Function &F);
			void addBogusFlow(BasicBlock *basicBlock, Function &F);
			bool doF(Module &M);
			BasicBlock *createAlteredBasicBlock(BasicBlock *basicBlock,
                                              const Twine &Name,
                                              Function *F);
    };
} 

#endif  

