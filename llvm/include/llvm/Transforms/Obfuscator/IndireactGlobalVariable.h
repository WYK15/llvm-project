
#ifndef LLVM_TRANSFORMS_INDIREACTGLOBALVARIABLE_H
#define LLVM_TRANSFORMS_INDIREACTGLOBALVARIABLE_H

#include "llvm/IR/PassManager.h" 
#include "llvm/Pass.h"
#include "llvm/IR/Constants.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Transforms/Utils/Local.h" // For DemoteRegToStack and DemotePHIToStack
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Support/CommandLine.h"

#include <map>
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>

namespace llvm {
    class IndireactGlobalVariablePass : public PassInfoMixin<IndireactGlobalVariablePass> {
        public: 
            PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM); 

            static bool isRequired() { return true; } 

        private:
            std::map<GlobalVariable *, unsigned> GVNumbering;
            std::map<GlobalVariable *, Constant *> GVOffsets;
            std::vector<GlobalVariable *> GVs;

            void doIndireactGlobalVariable(Function &F);

    };
}

#endif
