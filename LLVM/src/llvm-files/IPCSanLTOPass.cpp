//===-- IPCSanLTO.cpp - Externalize this module's CFI checks ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This pass exports all llvm.bitset's found in the module in the form of a
// __cfi_check function, which can be used to verify cross-DSO call targets.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/Triple.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalObject.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Operator.h"
#include "llvm/Pass.h"
#include "llvm/InitializePasses.h"
#include "llvm/PassRegistry.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Transforms/IPO/IPCSanLTOPass.h"

using namespace llvm;

#define DEBUG_TYPE "ipcsan-lto"

namespace {

struct IPCSanLTO : public ModulePass {
  static char ID;
  IPCSanLTO() : ModulePass(ID) {
    initializeIPCSanLTOPass(*PassRegistry::getPassRegistry());
  }

  bool runOnModule(Module &M) {

    llvm::errs() << "Hi LTO pass !" << "\n";
    if (skipModule(M))
      return false;

    FILE *op;
    op = fopen("/tmp/test_log.txt","a");
    if (op) {
      fprintf(op," Module Name: %s \n", M.getName().str().c_str());
      fclose(op);
    }

    for (Function &F : M) {
      for (BasicBlock &BB : F)
        for (Instruction &I : BB)
          if (CallInst *call = dyn_cast<CallInst>(&I)) {
            int find = 0;
            Function *targetFun = call->getCalledFunction();
          }
    }

    return true;
  }
};

} // anonymous namespace

INITIALIZE_PASS_BEGIN(IPCSanLTO, "ipcsan-lto", "IPCSanLTO", false, false)
INITIALIZE_PASS_END(IPCSanLTO, "ipcsan-lto", "IPCSanLTO", false, false)
char IPCSanLTO::ID = 0;

ModulePass *llvm::createIPCSanLTOPass() { return new IPCSanLTO(); }
PreservedAnalyses IPCSanLTOPass::run(Module &M,
                                     ModuleAnalysisManager &AM) {
  return PreservedAnalyses::none();
}
