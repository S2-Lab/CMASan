#ifndef LLVM_TRANSFORMS_INSTRUMENTATION_S2LAB_H
#define LLVM_TRANSFORMS_INSTRUMENTATION_S2LAB_H

#include "llvm/IR/PassManager.h"

namespace llvm {
class Module;

struct S2LabSanitizerPassOptions {
  bool CompileKernel;
  bool Recover;
  bool DisableOptimization;
};

class S2LabSanitizerPass : public PassInfoMixin<S2LabSanitizerPass> {
public:
  S2LabSanitizerPass(S2LabSanitizerPassOptions Options)
      : Options(Options){};
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
  static bool isRequired() { return true; }
private:
  S2LabSanitizerPassOptions Options;
  void logAllFunctionName (Module &M);
};

} // namespace llvm

#endif