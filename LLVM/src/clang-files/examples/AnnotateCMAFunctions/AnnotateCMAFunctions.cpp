//===- AnnotateFunctions.cpp ----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Example clang plugin which adds an annotation to every function in
// translation units that start with #pragma enable_annotate.
//
//===----------------------------------------------------------------------===//

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Attr.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/LexDiagnostic.h"
#include "clang/Frontend/CompilerInstance.h"

#include "json.hpp"

#include <fstream>
#include <iterator>
#include <sstream>

using namespace clang;
using json = nlohmann::json;  // Alias for convenience

static const std::string cma_attribute_header = "CMA_TARGET";
static const std::string delimiter = "|";

#define GET_INT_FIELD_OR_INVALID(entry, field) (entry.find(field) != entry.end() ? entry[field].get<int>() : -1)
#define WITH_DELIMITER_BEGIN(str) cma_attribute_header + delimiter + str + delimiter +
#define PARSE_WITH_DELIMITER(entry, field) GET_FIELD_OR_INVALID(entry, field) + delimiter +
#define PARSE_WITH_DELIMITER_END(entry, field) GET_FIELD_OR_INVALID(entry, field),
#define INT2STR(i) std::to_string(i)

#define JOIN_STRINGS_DELIM(DELIMITER, ...) \
    ([&](){ \
        std::ostringstream oss; \
        std::string strs[] = {__VA_ARGS__}; \
        size_t numStrs = sizeof(strs) / sizeof(strs[0]); \
        for (size_t i = 0; i < numStrs; ++i) { \
            oss << strs[i]; \
            if (i < numStrs - 1) oss << DELIMITER; \
        } \
        return oss.str(); \
    })()


namespace {

static bool HandledDecl = false;

typedef struct CMAInfoClang {
  // For logging
  std::string funcname;

  // Attributes
  std::string type;
  int family;
  int id_i; 
  int size_i; 
  int ptr_i;
  int num_i;
  
  // For comparison
  std::string file;
  int line;
  bool is_member;
} CMAInfoClang;

class AnnotateFunctionASTVisitor : public RecursiveASTVisitor<AnnotateFunctionASTVisitor> {
    CompilerInstance &Instance;
    std::vector<CMAInfoClang> InfoVec;

    bool isCMAFunctionDecl(FunctionDecl *FD, CMAInfoClang info) {
      clang::SourceManager &SourceManager = Instance.getSourceManager();
      SourceLocation Loc = FD->getSourceRange().getBegin();
      SourceRange sr = FD->getSourceRange();
      PresumedLoc begin = SourceManager.getPresumedLoc(sr.getBegin());
      std::string file = begin.getFilename();
      int line = begin.getLine();
      // llvm::errs() << FD->getQualifiedNameAsString() << "\n";
      // llvm::errs() << file << ":" << line << "\n";
      // Retrieve absolute path
      llvm::SmallString<4096> absolutePath; 
      if (!llvm::sys::fs::real_path(file, absolutePath)) {
        file = absolutePath.c_str(); 
      }

      if (file.find(info.file) != std::string::npos && line == info.line) return true;
      return false;
    }

    void AnnotateCMADecl(FunctionDecl *FD, CMAInfoClang info) {
      /* Annotate the target function */
      llvm::errs() << "Annotate " << info.funcname << "\n";

      std::string attributes = JOIN_STRINGS_DELIM(delimiter, cma_attribute_header, info.type, INT2STR(info.family), INT2STR(info.id_i), INT2STR(info.size_i), INT2STR(info.ptr_i), INT2STR(info.num_i));
      FD->addAttr(AnnotateAttr::CreateImplicit(FD->getASTContext(), attributes, nullptr, 0));
      if (!FD->hasAttr<NoInlineAttr>()) {
        SourceLocation Loc = FD->getSourceRange().getBegin();
        FD->addAttr(NoInlineAttr::CreateImplicit(FD->getASTContext(), Loc));
      }
      FD->dropAttr<GNUInlineAttr>();
      FD->dropAttr<AlwaysInlineAttr>();
    }

    void AnnotateDestructorDecl(CXXMethodDecl *MD, CMAInfoClang info) {
      CXXDestructorDecl *DD = MD->getParent()->getDestructor();
      if (DD) {
        // Prevent duplicate annotation
        for (const auto *Attr : DD->attrs()) {
            if (const auto *Annotate = dyn_cast<AnnotateAttr>(Attr)) {
                if (Annotate->getAnnotation().str() == StringRef("cma.destructor")) {
                  return;
                }
            }
        }
        DD->addAttr(AnnotateAttr::CreateImplicit(DD->getASTContext(), "cma.destructor", nullptr, 0));
        
        llvm::outs() << "Automatically annotate the destructor: " 
          << DD->getNameAsString() << "\n";
        CMAInfoClang dinfo = { DD->getNameAsString(), "8", info.family, 0, -1, -1, -1, "", -1, true };
        AnnotateCMADecl(DD, dinfo);
      }
    }

public:
    AnnotateFunctionASTVisitor(CompilerInstance &Instance, std::vector<CMAInfoClang> &InfoVec)
    :Instance(Instance), InfoVec(InfoVec) {};

    // This method is called for every FunctionDecl in the AST.
    bool VisitFunctionDecl(FunctionDecl *FD) {
        if (FD) {
          for (CMAInfoClang info : InfoVec) {
            if (isCMAFunctionDecl(FD, info)) {
              if (!info.is_member) { // check if already annotated as member
                if (CXXMethodDecl *MD= dyn_cast<CXXMethodDecl>(FD)) {
                  // StringRef name = MD->getName();
                  // llvm::errs() << "it's MD: " << name << "\n";
                  /* Increase index if it has instance */
                  if (MD->isInstance()) {
                    info.id_i++; // if idIdx was -1, automatically class instance will be id
                    if (info.size_i != -1) info.size_i++;
                    if (info.ptr_i != -1) info.ptr_i++;
                    if (info.num_i != -1) info.num_i++;
                  }
                  info.is_member = true;
                  AnnotateDestructorDecl(MD, info);
                }
              }
              AnnotateCMADecl(FD, info);
              if (FD->getCanonicalDecl()) AnnotateCMADecl(FD->getCanonicalDecl(), info);
              break;
            }
          }
        }
        return true; // Returning true to continue visiting subsequent AST nodes.
    }
};

class AnnotateFunctionsConsumer : public ASTConsumer {
  CompilerInstance &Instance;
  std::vector<CMAInfoClang> InfoVec;
  AnnotateFunctionASTVisitor Visitor;
public:
  AnnotateFunctionsConsumer(CompilerInstance &Instance, std::vector<CMAInfoClang> &InfoVec)
    :Instance(Instance), InfoVec(InfoVec), Visitor(Instance, InfoVec) {};
  
 
  bool HandleTopLevelDecl(DeclGroupRef DG) override {
    HandledDecl = true;
    for (clang::DeclGroupRef::iterator I = DG.begin(), E = DG.end(); I != E; ++I) {
      Visitor.TraverseDecl(*I);
    }
    return true;
  }
};

class AnnotateFunctionsAction : public PluginASTAction {
  std::vector<CMAInfoClang> info_vector;

  int EncodeType(std::string type) {
      if (type == std::string("NONE"))
        return 0;
      if (type == std::string("ALLOC"))
        return 1;
      if (type == std::string("FREE"))
        return 2;
      if (type == std::string("REALLOC"))
        return 3;
      if (type == std::string("CALLOC"))
        return 4;
      if (type == std::string("SIZE"))
        return 5;
      if (type == std::string("LOG_TARGET"))
        return 6;
      if (type == std::string("CLEAR"))
        return 7;
      if (type == std::string("GC"))
        return 8;
      return -1;
  }
public:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 llvm::StringRef) override {
    return std::make_unique<AnnotateFunctionsConsumer>(CI, info_vector);
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) override {
                  
    if(args.size() <1)
    {
        llvm::errs() << "Lack CMA info path!\n";
        return false;
    }
    std::string info_path = args[0];
    // llvm::errs() << info_path << "\n";
    std::ifstream file(info_path);

    json json_array = json::parse(file);
    for (const auto& entry : json_array) {
      int type = EncodeType(entry["type"].get<std::string>()); // Type must be valid
      if (type == -1) continue;
      
      CMAInfoClang info = {
        entry["funcname"].get<std::string>(),
        std::to_string(type),
        GET_INT_FIELD_OR_INVALID(entry, "family"),
        GET_INT_FIELD_OR_INVALID(entry, "idIdx"),
        GET_INT_FIELD_OR_INVALID(entry, "sizeIdx"),
        GET_INT_FIELD_OR_INVALID(entry, "ptrIdx"),
        GET_INT_FIELD_OR_INVALID(entry, "numIdx"),
        entry["file"].get<std::string>(),
        entry["begin"].get<int>(),
        false
      };

      info_vector.push_back(info);
    }
    
    
    return true;
  }

  PluginASTAction::ActionType getActionType() override {
    return AddBeforeMainAction;
  }
};

}

static FrontendPluginRegistry::Add<AnnotateFunctionsAction>
X("annotate-cma-fns", "annotate CMA functions");