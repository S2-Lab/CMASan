#ifndef DISABLE_CMASAN
#include "llvm/Support/YAMLParser.h"
#include "llvm/Support/YAMLTraits.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include <vector>
#include <string>
#include <unordered_map>

using namespace llvm;
using namespace llvm::yaml;

using llvm::yaml::Input;
using llvm::yaml::IO;
using llvm::yaml::MappingTraits;
using llvm::yaml::Output;
using llvm::yaml::ScalarEnumerationTraits;

enum CMA_TYPE
{
    NONE = 0,
    ALLOC = 1,
    FREE = 2,
    REALLOC = 3,
    CALLOC = 4,
    SIZE = 5,
    LOG_TARGET = 6,
    CLEAR = 7,
    GC = 8,
};

struct CMAInfo
{   
    CMA_TYPE type;
    int id;
    int idIdx;
    int sizeIdx;
    int ptrIdx;
    int numIdx;
    // int n_params;
    // StringRef functionName;
};

const char* kCMASanTargetAttribute = "CMA_TARGET";
static const char* kCMASanDelimiter = "|";

static std::unordered_map<const Function*, CMAInfo> _infoMap;


#define PARSE_INT_FROM_FIELD(i) stoi(fields[i])

void CMASanParseInfo(Module &M) {
    auto global_annos = M.getNamedGlobal("llvm.global.annotations");
    if (global_annos) {
        ConstantArray* A = cast<ConstantArray>(global_annos->getOperand(0));
        for (int i=0; i<A->getNumOperands(); i++) {
            ConstantStruct *CS = cast<ConstantStruct>(A->getOperand(i));
            if (Function *F = dyn_cast<Function>(CS->getOperand(0)->getOperand(0))) {
                StringRef anno = cast<ConstantDataArray>(cast<GlobalVariable>(CS->getOperand(1)->getOperand(0))->getOperand(0))->getAsCString();
                if (anno.startswith(kCMASanTargetAttribute)) {
                    F->addFnAttr(kCMASanTargetAttribute);

                    StringRef input = anno.split(kCMASanDelimiter).second; // skip kCMASanTargetAttribute
                    std::vector<std::string> fields;
                    while (!input.empty()) {
                        auto token = input.split(kCMASanDelimiter);
                        fields.push_back(token.first.str());
                        input = token.second;
                    }

                    struct CMAInfo info = {
                        (CMA_TYPE) PARSE_INT_FROM_FIELD(0), // TYPE
                        PARSE_INT_FROM_FIELD(1),
                        PARSE_INT_FROM_FIELD(2),
                        PARSE_INT_FROM_FIELD(3),
                        PARSE_INT_FROM_FIELD(4),
                        PARSE_INT_FROM_FIELD(5),
                        // PARSE_INT_FROM_FIELD(6),
                    };
                    _infoMap.insert({F,info});
                }
           }
        }
    }
}

CMAInfo getCMAInfo(const Function *F) {
    CMAInfo cmaInfo = _infoMap[F];
    return cmaInfo;
}


void WriteBackCMANames(std::string file_path, Module &M, std::vector<const Function*> cma_list) {
  std::error_code EC;
  
  if (file_path == "") {
    llvm::errs() << "Wrong file path from -asan-cma-writeback-path";
    return;
  }

  if (cma_list.size() == 0) return;

  llvm::errs() << "Write back to RT_CMA_LIST:" << file_path << "\n";
  raw_fd_ostream out (file_path, EC, llvm::sys::fs::OF_Append);
  out << "[module] ";
  out << M.getName();
  out << "\n";

  for (const Function *F : cma_list) {
    out << F->getName();
    out << " ";
    out << getCMAInfo(F).type;
    out << "\n";
  }

  out.close();
}
# endif