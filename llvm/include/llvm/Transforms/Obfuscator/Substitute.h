#ifndef LLVM_TRANSFORMS_Substitute_H

#define LLVM_TRANSFORMS_Substitute_H

#include "llvm/IR/PassManager.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"

#define NUMBER_ADD_SUBST 4
#define NUMBER_SUB_SUBST 3
#define NUMBER_AND_SUBST 2
#define NUMBER_OR_SUBST 2
#define NUMBER_XOR_SUBST 2

namespace llvm {

  class SubstitutePass : public PassInfoMixin<SubstitutePass> {

   public:
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
    static bool isRequired() { return true; }

    SubstitutePass();


    private:
      void (SubstitutePass::*funcAdd[NUMBER_ADD_SUBST])(BinaryOperator *bo);
      void (SubstitutePass::*funcSub[NUMBER_SUB_SUBST])(BinaryOperator *bo);
      void (SubstitutePass::*funcAnd[NUMBER_AND_SUBST])(BinaryOperator *bo);
      void (SubstitutePass::*funcOr[NUMBER_OR_SUBST])(BinaryOperator *bo);
      void (SubstitutePass::*funcXor[NUMBER_XOR_SUBST])(BinaryOperator *bo);

      bool doSubstitute(Function *f);

      void addNeg(BinaryOperator *bo);
      void addDoubleNeg(BinaryOperator *bo);
      void addRand(BinaryOperator *bo);
      void addRand2(BinaryOperator *bo);

      void subNeg(BinaryOperator *bo);
      void subRand(BinaryOperator *bo);
      void subRand2(BinaryOperator *bo);

      void andSubstitution(BinaryOperator *bo);
      void andSubstitutionRand(BinaryOperator *bo);

      void orSubstitution(BinaryOperator *bo);
      void orSubstitutionRand(BinaryOperator *bo);

      void xorSubstitution(BinaryOperator *bo);
      void xorSubstitutionRand(BinaryOperator *bo);
  };

}

#endif
