#ifndef LLVM_TRANSFORMS_IPO_IPCSANLTOPASS_H
#define LLVM_TRANSFORMS_IPO_IPCSANLTOPASS_H

#include "llvm/IR/PassManager.h"

namespace llvm {

class Module;

/// Pass to perform interprocedural constant propagation.
class IPCSanLTOPass : public PassInfoMixin<IPCSanLTOPass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
};

} // end namespace llvm

#endif // LLVM_TRANSFORMS_IPO_IPCSANLTOPASS_H
