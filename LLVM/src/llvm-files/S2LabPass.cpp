//===-- S2LabPass.cpp -----------------------------------------------===//
//
//===----------------------------------------------------------------------===//

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Transforms/Instrumentation.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/MemoryBuiltins.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/InitializePasses.h"

#include "llvm/Pass.h"
#include "llvm/Support/FileSystem.h"

#include "llvm/Transforms/Instrumentation/s2lab.h"

#include <cxxabi.h>
#include <set>
#include <fstream>
#include <unordered_set>
#include <string>

#define MAXLEN 10000

using namespace llvm;
using namespace std;


char *
getDemangledFunctionName (StringRef mangledFunctionName)
{
  int unmangledStatus;
  char *functionName;

  if (mangledFunctionName.size ())
    {
      functionName = strdup (mangledFunctionName.str ().c_str ());
    }
  else
    {
      return strdup ("");
    }

  // size_t length;
  char *demangledFunctionName = abi::__cxa_demangle (
      functionName, nullptr, nullptr, &unmangledStatus);
  
  if (unmangledStatus == 0)
  {
    functionName = demangledFunctionName; // TODO: remove comment if unmangling needed!
  }

  return functionName;
}

void S2LabSanitizerPass::logAllFunctionName (Module &M)
{
  std::error_code EC;
  string defaultDir = "./";
  char *const envDir = getenv ("OUT"); 
  string outDir = "/function_list.log";
  if (envDir) outDir = string(envDir) + outDir;
  else outDir = defaultDir + outDir;

  raw_fd_ostream out (outDir.c_str(), EC,
                      llvm::sys::fs::OF_Append);
  if (EC.value()) {
    errs() << "S2Lab pass: " << EC.message() << "\n";
    return;
  }

  for (Module::iterator F = M.begin (), E = M.end (); F != E; ++F)
  {
    for (Function::iterator BB = F->begin (), E = F->end (); BB != E; ++BB)
      {
          
          StringRef functionName = F->getName ();
          if (functionName.empty()) continue;
          
          char *calledFunctionName = getDemangledFunctionName (functionName);
          char *errorFunctionName = "<error>";
          if (calledFunctionName == nullptr) calledFunctionName = errorFunctionName;
          string calleeName (calledFunctionName);
          if (calleeName.find("llvm.") != std::string::npos
   || calleeName.find("asan.") != std::string::npos) continue;
          out << "unmangled: " << calledFunctionName << "\n";
          out << "mangled: " << F->getName() << "\n";
          if (F->getFunctionType()) out << "# of params: " << F->getFunctionType()->getNumParams() << "\n";
          out << "\n";
                  
      }
  }

  out.close ();
}

// register pass
// char S2Lab::ID;

// INITIALIZE_PASS (S2Lab, "S2Lab", "S2LabPass: s2lab default pass", false, false)

PreservedAnalyses S2LabSanitizerPass::run(Module &M,
                                              ModuleAnalysisManager &MAM) {
  llvm::errs() << "Hi S2Lab pass\n";
  logAllFunctionName(M);
  return PreservedAnalyses::all();
}