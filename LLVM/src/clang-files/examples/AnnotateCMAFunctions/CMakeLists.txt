add_llvm_library(AnnotateCMAFunctions MODULE AnnotateCMAFunctions.cpp PLUGIN_TOOL clang)

if(WIN32 OR CYGWIN)
  set(LLVM_LINK_COMPONENTS
    Support
  )
  clang_target_link_libraries(AnnotateCMAFunctions PRIVATE
    clangAST
    clangBasic
    clangFrontend
    clangLex
    )
endif()

target_include_directories(AnnotateCMAFunctions PRIVATE include)
