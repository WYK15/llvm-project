#include "llvm/Transforms/Obfuscator/Substitute.h"

#include "llvm/Transforms/Obfuscator/Utils.h"
#include "llvm/Transforms/Obfuscator/CryptoUtils.h"
#include "llvm/Transforms/Obfuscator/LegacyLowerSwitch.h"
#include "llvm/Transforms/Obfuscator/BogusControlFlow.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Transforms/Utils/Local.h" // For DemoteRegToStack and DemotePHIToStack
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/IR/NoFolder.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Transforms/Utils/ValueMapper.h"
#include "llvm/Transforms/Utils/Cloning.h"

using namespace llvm;
using namespace std;

#define DEBUG_TYPE "substitute"

static cl::opt<int>
    ObfTimes("sub_loop",
             cl::desc("Choose how many time the -sub pass loops on a function"),
             cl::value_desc("number of times"), cl::init(1), cl::Optional);
static cl::opt<unsigned int>
    ObfProbRate("sub_prob",
                cl::desc("Choose the probability [%] each basic blocks will be "
                         "obfuscated by the InstructioSubstitution pass"),
                cl::value_desc("probability rate"), cl::init(50), cl::Optional);

static cl::opt<bool> sub("enable-subobf", cl::desc("substitute pass switch"), cl::init(false));

// Stats
STATISTIC(Add, "Add substitued");
STATISTIC(Sub, "Sub substitued");
// STATISTIC(Mul,  "Mul substitued");
// STATISTIC(Div,  "Div substitued");
// STATISTIC(Rem,  "Rem substitued");
// STATISTIC(Shi,  "Shift substitued");
STATISTIC(And, "And substitued");
STATISTIC(Or, "Or substitued");
STATISTIC(Xor, "Xor substitued");

SubstitutePass::SubstitutePass() {
    funcAdd[0] = &SubstitutePass::addNeg;
    funcAdd[1] = &SubstitutePass::addDoubleNeg;
    funcAdd[2] = &SubstitutePass::addRand;
    funcAdd[3] = &SubstitutePass::addRand2;

    funcSub[0] = &SubstitutePass::subNeg;
    funcSub[1] = &SubstitutePass::subRand;
    funcSub[2] = &SubstitutePass::subRand2;

    funcAnd[0] = &SubstitutePass::andSubstitution;
    funcAnd[1] = &SubstitutePass::andSubstitutionRand;

    funcOr[0] = &SubstitutePass::orSubstitution;
    funcOr[1] = &SubstitutePass::orSubstitutionRand;

    funcXor[0] = &SubstitutePass::xorSubstitution;
    funcXor[1] = &SubstitutePass::xorSubstitutionRand;
}

// entry
PreservedAnalyses SubstitutePass::run(Function &F, FunctionAnalysisManager &AM) {
	if (sub)
	{
		//errs() << "run SubstitutePass on : " << F.getName() << "\n";
		 Function *tmp = &F;
		doSubstitute(tmp);
	}
	return PreservedAnalyses::all();
}


bool SubstitutePass::doSubstitute(Function *f) {
    if (ObfTimes <= 0) {
      errs() << "Substitution application number -sub_loop=x must be x > 0";
      return false;
    }
    if (ObfProbRate > 100) {
      errs() << "InstructionSubstitution application instruction percentage "
                "-sub_prob=x must be 0 < x <= 100";
      return false;
    }

    Function *tmp = f;

    // Loop for the number of time we run the pass on the function
    int times = ObfTimes;
    do {
      for (Function::iterator bb = tmp->begin(); bb != tmp->end(); ++bb) {
        for (BasicBlock::iterator inst = bb->begin(); inst != bb->end(); ++inst) {
          if (inst->isBinaryOp() && cryptoutils->get_range(100) <= ObfProbRate) {
            switch (inst->getOpcode()) {
            case BinaryOperator::Add:
              // case BinaryOperator::FAdd:
              // Substitute with random add operation
              (this->*funcAdd[llvm::cryptoutils->get_range(NUMBER_ADD_SUBST)])(
                  cast<BinaryOperator>(inst));
              ++Add;
              break;
            case BinaryOperator::Sub:
              // case BinaryOperator::FSub:
              // Substitute with random sub operation
              (this->*funcSub[llvm::cryptoutils->get_range(NUMBER_SUB_SUBST)])(
                  cast<BinaryOperator>(inst));
              ++Sub;
              break;
            case BinaryOperator::Mul:
            case BinaryOperator::FMul:
              //++Mul;
              break;
            case BinaryOperator::UDiv:
            case BinaryOperator::SDiv:
            case BinaryOperator::FDiv:
              //++Div;
              break;
            case BinaryOperator::URem:
            case BinaryOperator::SRem:
            case BinaryOperator::FRem:
              //++Rem;
              break;
            case Instruction::Shl:
              //++Shi;
              break;
            case Instruction::LShr:
              //++Shi;
              break;
            case Instruction::AShr:
              //++Shi;
              break;
            case Instruction::And:
              (this->*funcAnd[llvm::cryptoutils->get_range(2)])(
                  cast<BinaryOperator>(inst));
              ++And;
              break;
            case Instruction::Or:
              (this->*funcOr[llvm::cryptoutils->get_range(2)])(
                  cast<BinaryOperator>(inst));
              ++Or;
              break;
            case Instruction::Xor:
              (this->*funcXor[llvm::cryptoutils->get_range(2)])(
                  cast<BinaryOperator>(inst));
              ++Xor;
              break;
            default:
              break;
            }              // End switch
          }                // End isBinaryOp
        }                  // End for basickblock
      }                    // End for Function
    } while (--times > 0); // for times
    return false;
}


// Implementation of a = b - (-c)
void SubstitutePass::addNeg(BinaryOperator *bo) {
  BinaryOperator *op = NULL;

  // Create sub
  if (bo->getOpcode() == Instruction::Add) {
    op = BinaryOperator::CreateNeg(bo->getOperand(1), "", bo);
    op =
        BinaryOperator::Create(Instruction::Sub, bo->getOperand(0), op, "", bo);

    // Check signed wrap
    // op->setHasNoSignedWrap(bo->hasNoSignedWrap());
    // op->setHasNoUnsignedWrap(bo->hasNoUnsignedWrap());

    bo->replaceAllUsesWith(op);
  }
}

// Implementation of a = -(-b + (-c))
void SubstitutePass::addDoubleNeg(BinaryOperator *bo) {

  if (bo->getOpcode() == Instruction::Add) {
    BinaryOperator *op = BinaryOperator::CreateNeg(bo->getOperand(0), "", bo);
    BinaryOperator *op2 = BinaryOperator::CreateNeg(bo->getOperand(1), "", bo);
    op = BinaryOperator::Create(Instruction::Add, op, op2, "", bo);
    op = BinaryOperator::CreateNeg(op, "", bo);

    bo->replaceAllUsesWith(op);

    // Check signed wrap
    // op->setHasNoSignedWrap(bo->hasNoSignedWrap());
    // op->setHasNoUnsignedWrap(bo->hasNoUnsignedWrap());
  }
  else {
    UnaryOperator *op = UnaryOperator::CreateFNeg(bo->getOperand(0), "", bo); // -b
    UnaryOperator *op2 = UnaryOperator::CreateFNeg(bo->getOperand(1), "", bo); // -c
    BinaryOperator *addop = BinaryOperator::Create(Instruction::FAdd, op, op2, "", bo); // -b + (-c)
    UnaryOperator *finalOp = UnaryOperator::CreateFNeg(addop, "", bo); // - (-b + (-c))

    bo->replaceAllUsesWith(finalOp);
  }
    

}

// Implementation of  r = rand (); a = b + r; a = a + c; a = a - r
void SubstitutePass::addRand(BinaryOperator *bo) {
  BinaryOperator *op = NULL;

  if (bo->getOpcode() == Instruction::Add) {
    Type *ty = bo->getType();
    ConstantInt *co =
        (ConstantInt *)ConstantInt::get(ty, llvm::cryptoutils->get_uint64_t());
    op =
        BinaryOperator::Create(Instruction::Add, bo->getOperand(0), co, "", bo);
    op =
        BinaryOperator::Create(Instruction::Add, op, bo->getOperand(1), "", bo);
    op = BinaryOperator::Create(Instruction::Sub, op, co, "", bo);

    // Check signed wrap
    // op->setHasNoSignedWrap(bo->hasNoSignedWrap());
    // op->setHasNoUnsignedWrap(bo->hasNoUnsignedWrap());

    bo->replaceAllUsesWith(op);
  }
  /* else {
      Type *ty = bo->getType();
      ConstantFP *co =
  (ConstantFP*)ConstantFP::get(ty,(float)llvm::cryptoutils->get_uint64_t());
      op = BinaryOperator::Create(Instruction::FAdd,bo->getOperand(0),co,"",bo);
      op = BinaryOperator::Create(Instruction::FAdd,op,bo->getOperand(1),"",bo);
      op = BinaryOperator::Create(Instruction::FSub,op,co,"",bo);
  } */
}

// Implementation of r = rand (); a = b - r; a = a + b; a = a + r
void SubstitutePass::addRand2(BinaryOperator *bo) {
  BinaryOperator *op = NULL;

  if (bo->getOpcode() == Instruction::Add) {
    Type *ty = bo->getType();
    ConstantInt *co =
        (ConstantInt *)ConstantInt::get(ty, llvm::cryptoutils->get_uint64_t());
    op =
        BinaryOperator::Create(Instruction::Sub, bo->getOperand(0), co, "", bo);
    op =
        BinaryOperator::Create(Instruction::Add, op, bo->getOperand(1), "", bo);
    op = BinaryOperator::Create(Instruction::Add, op, co, "", bo);

    // Check signed wrap
    // op->setHasNoSignedWrap(bo->hasNoSignedWrap());
    // op->setHasNoUnsignedWrap(bo->hasNoUnsignedWrap());

    bo->replaceAllUsesWith(op);
  }
  /* else {
      Type *ty = bo->getType();
      ConstantFP *co =
  (ConstantFP*)ConstantFP::get(ty,(float)llvm::cryptoutils->get_uint64_t());
      op = BinaryOperator::Create(Instruction::FAdd,bo->getOperand(0),co,"",bo);
      op = BinaryOperator::Create(Instruction::FAdd,op,bo->getOperand(1),"",bo);
      op = BinaryOperator::Create(Instruction::FSub,op,co,"",bo);
  } */
}

// Implementation of a = b + (-c)
void SubstitutePass::subNeg(BinaryOperator *bo) {
  BinaryOperator *op = NULL;

  if (bo->getOpcode() == Instruction::Sub) {
    op = BinaryOperator::CreateNeg(bo->getOperand(1), "", bo);
    op =
        BinaryOperator::Create(Instruction::Add, bo->getOperand(0), op, "", bo);

    // Check signed wrap
    // op->setHasNoSignedWrap(bo->hasNoSignedWrap());
    // op->setHasNoUnsignedWrap(bo->hasNoUnsignedWrap());
  }
  else {
    UnaryOperator *op1 = UnaryOperator::CreateFNeg(bo->getOperand(1), "", bo);
    op = BinaryOperator::Create(Instruction::FAdd, bo->getOperand(0), op1, "",
                                bo);
  }
  bo->replaceAllUsesWith(op);
}

// Implementation of  r = rand (); a = b + r; a = a - c; a = a - r
void SubstitutePass::subRand(BinaryOperator *bo) {
  BinaryOperator *op = NULL;

  if (bo->getOpcode() == Instruction::Sub) {
    Type *ty = bo->getType();
    ConstantInt *co =
        (ConstantInt *)ConstantInt::get(ty, llvm::cryptoutils->get_uint64_t());
    op =
        BinaryOperator::Create(Instruction::Add, bo->getOperand(0), co, "", bo);
    op =
        BinaryOperator::Create(Instruction::Sub, op, bo->getOperand(1), "", bo);
    op = BinaryOperator::Create(Instruction::Sub, op, co, "", bo);

    // Check signed wrap
    // op->setHasNoSignedWrap(bo->hasNoSignedWrap());
    // op->setHasNoUnsignedWrap(bo->hasNoUnsignedWrap());

    bo->replaceAllUsesWith(op);
  }
  /* else {
      Type *ty = bo->getType();
      ConstantFP *co =
  (ConstantFP*)ConstantFP::get(ty,(float)llvm::cryptoutils->get_uint64_t());
      op = BinaryOperator::Create(Instruction::FAdd,bo->getOperand(0),co,"",bo);
      op = BinaryOperator::Create(Instruction::FSub,op,bo->getOperand(1),"",bo);
      op = BinaryOperator::Create(Instruction::FSub,op,co,"",bo);
  } */
}

// Implementation of  r = rand (); a = b - r; a = a - c; a = a + r
void SubstitutePass::subRand2(BinaryOperator *bo) {
  BinaryOperator *op = NULL;

  if (bo->getOpcode() == Instruction::Sub) {
    Type *ty = bo->getType();
    ConstantInt *co =
        (ConstantInt *)ConstantInt::get(ty, llvm::cryptoutils->get_uint64_t());
    op =
        BinaryOperator::Create(Instruction::Sub, bo->getOperand(0), co, "", bo);
    op =
        BinaryOperator::Create(Instruction::Sub, op, bo->getOperand(1), "", bo);
    op = BinaryOperator::Create(Instruction::Add, op, co, "", bo);

    // Check signed wrap
    // op->setHasNoSignedWrap(bo->hasNoSignedWrap());
    // op->setHasNoUnsignedWrap(bo->hasNoUnsignedWrap());

    bo->replaceAllUsesWith(op);
  }
  /* else {
      Type *ty = bo->getType();
      ConstantFP *co =
  (ConstantFP*)ConstantFP::get(ty,(float)llvm::cryptoutils->get_uint64_t());
      op = BinaryOperator::Create(Instruction::FSub,bo->getOperand(0),co,"",bo);
      op = BinaryOperator::Create(Instruction::FSub,op,bo->getOperand(1),"",bo);
      op = BinaryOperator::Create(Instruction::FAdd,op,co,"",bo);
  } */
}

// Implementation of a = b & c => a = (b^~c)& b
void SubstitutePass::andSubstitution(BinaryOperator *bo) {
  BinaryOperator *op = NULL;

  // Create NOT on second operand => ~c
  op = BinaryOperator::CreateNot(bo->getOperand(1), "", bo);

  // Create XOR => (b^~c)
  BinaryOperator *op1 =
      BinaryOperator::Create(Instruction::Xor, bo->getOperand(0), op, "", bo);

  // Create AND => (b^~c) & b
  op = BinaryOperator::Create(Instruction::And, op1, bo->getOperand(0), "", bo);
  bo->replaceAllUsesWith(op);
}

// Implementation of a = a && b <=> !(!a | !b) && (r | !r)
void SubstitutePass::andSubstitutionRand(BinaryOperator *bo) {
  // Copy of the BinaryOperator type to create the random number with the
  // same type of the operands
  Type *ty = bo->getType();

  // r (Random number)
  ConstantInt *co =
      (ConstantInt *)ConstantInt::get(ty, llvm::cryptoutils->get_uint64_t());

  // !a
  BinaryOperator *op = BinaryOperator::CreateNot(bo->getOperand(0), "", bo);

  // !b
  BinaryOperator *op1 = BinaryOperator::CreateNot(bo->getOperand(1), "", bo);

  // !r
  BinaryOperator *opr = BinaryOperator::CreateNot(co, "", bo);

  // (!a | !b)
  BinaryOperator *opa =
      BinaryOperator::Create(Instruction::Or, op, op1, "", bo);

  // (r | !r)
  opr = BinaryOperator::Create(Instruction::Or, co, opr, "", bo);

  // !(!a | !b)
  op = BinaryOperator::CreateNot(opa, "", bo);

  // !(!a | !b) && (r | !r)
  op = BinaryOperator::Create(Instruction::And, op, opr, "", bo);

  // We replace all the old AND operators with the new one transformed
  bo->replaceAllUsesWith(op);
}

// Implementation of a = b | c => a = (b & c) | (b ^ c)
void SubstitutePass::orSubstitutionRand(BinaryOperator *bo) {

  Type *ty = bo->getType();
  ConstantInt *co =
      (ConstantInt *)ConstantInt::get(ty, llvm::cryptoutils->get_uint64_t());

  // !a
  BinaryOperator *op = BinaryOperator::CreateNot(bo->getOperand(0), "", bo);

  // !b
  BinaryOperator *op1 = BinaryOperator::CreateNot(bo->getOperand(1), "", bo);

  // !r
  BinaryOperator *op2 = BinaryOperator::CreateNot(co, "", bo);

  // !a && r
  BinaryOperator *op3 =
      BinaryOperator::Create(Instruction::And, op, co, "", bo);

  // a && !r
  BinaryOperator *op4 =
      BinaryOperator::Create(Instruction::And, bo->getOperand(0), op2, "", bo);

  // !b && r
  BinaryOperator *op5 =
      BinaryOperator::Create(Instruction::And, op1, co, "", bo);

  // b && !r
  BinaryOperator *op6 =
      BinaryOperator::Create(Instruction::And, bo->getOperand(1), op2, "", bo);

  // (!a && r) || (a && !r)
  op3 = BinaryOperator::Create(Instruction::Or, op3, op4, "", bo);

  // (!b && r) ||(b && !r)
  op4 = BinaryOperator::Create(Instruction::Or, op5, op6, "", bo);

  // (!a && r) || (a && !r) ^ (!b && r) ||(b && !r)
  op5 = BinaryOperator::Create(Instruction::Xor, op3, op4, "", bo);

  // !a || !b
  op3 = BinaryOperator::Create(Instruction::Or, op, op1, "", bo);

  // !(!a || !b)
  op3 = BinaryOperator::CreateNot(op3, "", bo);

  // r || !r
  op4 = BinaryOperator::Create(Instruction::Or, co, op2, "", bo);

  // !(!a || !b) && (r || !r)
  op4 = BinaryOperator::Create(Instruction::And, op3, op4, "", bo);

  // [(!a && r) || (a && !r) ^ (!b && r) ||(b && !r) ] || [!(!a || !b) && (r ||
  // !r)]
  op = BinaryOperator::Create(Instruction::Or, op5, op4, "", bo);
  bo->replaceAllUsesWith(op);
}

void SubstitutePass::orSubstitution(BinaryOperator *bo) {
  BinaryOperator *op = NULL;

  // Creating first operand (b & c)
  op = BinaryOperator::Create(Instruction::And, bo->getOperand(0),
                              bo->getOperand(1), "", bo);

  // Creating second operand (b ^ c)
  BinaryOperator *op1 = BinaryOperator::Create(
      Instruction::Xor, bo->getOperand(0), bo->getOperand(1), "", bo);

  // final op
  op = BinaryOperator::Create(Instruction::Or, op, op1, "", bo);
  bo->replaceAllUsesWith(op);
}

// Implementation of a = a ~ b => a = (!a && b) || (a && !b)
void SubstitutePass::xorSubstitution(BinaryOperator *bo) {
  BinaryOperator *op = NULL;

  // Create NOT on first operand
  op = BinaryOperator::CreateNot(bo->getOperand(0), "", bo); // !a

  // Create AND
  op = BinaryOperator::Create(Instruction::And, bo->getOperand(1), op, "",
                              bo); // !a && b

  // Create NOT on second operand
  BinaryOperator *op1 =
      BinaryOperator::CreateNot(bo->getOperand(1), "", bo); // !b

  // Create AND
  op1 = BinaryOperator::Create(Instruction::And, bo->getOperand(0), op1, "",
                               bo); // a && !b

  // Create OR
  op = BinaryOperator::Create(Instruction::Or, op, op1, "",
                              bo); // (!a && b) || (a && !b)
  bo->replaceAllUsesWith(op);
}

// implementation of a = a ^ b <=> (a ^ r) ^ (b ^ r) <=> (!a && r || a && !r) ^
// (!b && r || b && !r)
// note : r is a random number
void SubstitutePass::xorSubstitutionRand(BinaryOperator *bo) {
  BinaryOperator *op = NULL;

  Type *ty = bo->getType();
  ConstantInt *co =
      (ConstantInt *)ConstantInt::get(ty, llvm::cryptoutils->get_uint64_t());

  // !a
  op = BinaryOperator::CreateNot(bo->getOperand(0), "", bo);

  // !a && r
  op = BinaryOperator::Create(Instruction::And, co, op, "", bo);

  // !r
  BinaryOperator *opr = BinaryOperator::CreateNot(co, "", bo);

  // a && !r
  BinaryOperator *op1 =
      BinaryOperator::Create(Instruction::And, bo->getOperand(0), opr, "", bo);

  // !b
  BinaryOperator *op2 = BinaryOperator::CreateNot(bo->getOperand(1), "", bo);

  // !b && r
  op2 = BinaryOperator::Create(Instruction::And, op2, co, "", bo);

  // b && !r
  BinaryOperator *op3 =
      BinaryOperator::Create(Instruction::And, bo->getOperand(1), opr, "", bo);

  // (!a && r) || (a && !r)
  op = BinaryOperator::Create(Instruction::Or, op, op1, "", bo);

  // (!b && r) || (b && !r)
  op1 = BinaryOperator::Create(Instruction::Or, op2, op3, "", bo);

  // (!a && r) || (a && !r) ^ (!b && r) || (b && !r)
  op = BinaryOperator::Create(Instruction::Xor, op, op1, "", bo);
  bo->replaceAllUsesWith(op);
}