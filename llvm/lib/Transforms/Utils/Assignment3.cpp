
#include "llvm/Transforms/Utils/Assignment3.h"

#include "../../../include/llvm/ADT/DepthFirstIterator.h"

#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/Analysis/PostDominators.h"

using namespace llvm;

void Assignment3Pass::markLive(Instruction *I) {
  if (liveInstrictions.find(I) == liveInstrictions.end()) {
    liveInstrictions.insert(I);
    worklist.emplace(I);
  }
}

bool Assignment3Pass::isTriviallyDead(Instruction *I) {
  return I->users().empty() && !I->isTerminator();
}

bool Assignment3Pass::isTriviallyLive(Instruction *I) {
  return I->isTerminator() || I->mayHaveSideEffects() || I->mayWriteToMemory();
}

PreservedAnalyses Assignment3Pass::run(Function &F, FunctionAnalysisManager &AM) {

    // Initialize structures
    func = &F;
    liveInstrictions.clear();
    reachableBlocks.clear();
    assert(worklist.empty());

    // Mark trivially live and trivially dead instructions

    // TODO
    BasicBlock *BBstart = &F.getEntryBlock();
    for (BasicBlock* BB : depth_first(BBstart)) {
      reachableBlocks.insert(BB);
      for (auto I = BB->begin(); I != BB->end(); ++I) {
        Instruction *inst = &*I;
        if (isTriviallyLive(inst)) {
          markLive(inst);
        } else if (isTriviallyDead(inst)) {
          auto next = ++I;
          inst->dropAllReferences();
          inst->removeFromParent();
          I = --next;
        }
      }
    }


    // Process worklist to find new live instructions

    // TODO
    while (!worklist.empty()) {
      Instruction *I = worklist.front();
      worklist.pop();
      BasicBlock *BB = I->getParent();

      if (reachableBlocks.find(BB) != reachableBlocks.end()) {
        for (auto & op : I->operands()) {
          if (auto *inst = dyn_cast<Instruction>(op)) {
            markLive(inst);
          }
        }
      }
    }


    // Delete all instructions that are not live
    bool changed = false;

    // TODO
    for (BasicBlock* BB : reachableBlocks) {
      for (auto I = BB->begin(); I != BB->end(); ++I) {
        Instruction *inst = &*I;
        if (liveInstrictions.find(inst) == liveInstrictions.end()) {
          changed = true;
          auto next = ++I;
          inst->dropAllReferences();
          inst->removeFromParent();
          I = --next;
        }
      }
    }


    // Return
    if(changed) {
        return PreservedAnalyses::none();
    } else {
        return PreservedAnalyses::all();
    }

}

