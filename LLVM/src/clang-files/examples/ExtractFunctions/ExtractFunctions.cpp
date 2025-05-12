//===- ExtractFunctions.cpp ----------------------------------------------===//
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

#include <fstream>
#include <iterator>
#include <sstream>

#include "llvm/Support/FileSystem.h"

using namespace clang;

namespace {

// static bool EnableAnnotate = false;
static bool HandledDecl = false;

class ExtractFunctionASTVisitor : public RecursiveASTVisitor<ExtractFunctionASTVisitor> {
    CompilerInstance &Instance;
    std::ofstream log_fstream;

    void logFunction(FunctionDecl *FD) {
      if (!FD) return;
      clang::SourceManager &SourceManager = Instance.getSourceManager();
      SourceRange sr = FD->getSourceRange();
      PresumedLoc begin = SourceManager.getPresumedLoc(sr.getBegin());
      int line = begin.getLine();
      std::string filePath = begin.getFilename();

      // Retrieve absolute path
      llvm::SmallString<4096> absolutePath; 
      if (llvm::sys::fs::real_path(filePath, absolutePath)) {
        log_fstream << FD->getNameAsString() << ", " << filePath << ", " << line <<  ", " << "0" << "\n"; 
      } else {
        log_fstream << FD->getNameAsString() << ", " << absolutePath.c_str() << ", " << line <<  ", " << "1" << "\n"; 
      }
      log_fstream.flush();
    }


public:
    ExtractFunctionASTVisitor(CompilerInstance &Instance, std::string log_path)
    :Instance(Instance) {
        log_fstream.open(log_path, std::ios::app);
        if (!log_fstream.is_open()) {
            llvm::errs() << "Failed to open file: " << log_path << "\n";
        }
    };

    // This method is called for every FunctionDecl in the AST.
    bool VisitFunctionDecl(FunctionDecl *FD) {
        logFunction(FD);
        return true; // Returning true to continue visiting subsequent AST nodes.
    }
};

class ExtractFunctionsConsumer : public ASTConsumer {
  CompilerInstance &Instance;
  ExtractFunctionASTVisitor Visitor;
public:
  ExtractFunctionsConsumer(CompilerInstance &Instance, std::string log_path)
    :Instance(Instance), Visitor(Instance, log_path) {};
  
 
  bool HandleTopLevelDecl(DeclGroupRef DG) override {
    HandledDecl = true;
    for (clang::DeclGroupRef::iterator I = DG.begin(), E = DG.end(); I != E; ++I) {
      Visitor.TraverseDecl(*I);
    }
    return true;
  }
};

class ExtractFunctionsAction : public PluginASTAction {
    std::string log_path;
public:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 llvm::StringRef) override {
    return std::make_unique<ExtractFunctionsConsumer>(CI, log_path);
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) override {

    // Parse from -Xclang -plugin-arg-extract-funcs -Xclang <path to log file>
    if(args.size() <1)
    {
        llvm::errs() << "Lack function log path!\n";
        return false;
    }

    log_path = args[0];

    return true;
  }

  PluginASTAction::ActionType getActionType() override {
    return AddBeforeMainAction;
  }
};

}

static FrontendPluginRegistry::Add<ExtractFunctionsAction>
X("extract-funcs", "Extract all C/C++ functions");

// static PragmaHandlerRegistry::Add<PragmaAnnotateHandler>
// Y("enable_annotate","enable annotation");
