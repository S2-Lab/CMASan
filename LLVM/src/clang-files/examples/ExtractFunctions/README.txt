This is a simple example demonstrating how to use clang's facility for
providing AST consumers using a plugin.

Thanks to Goshawk and Clang. This module is modified from Goshawk's ExtractFunctionPrototypes

Build the plugin by running `make` in this directory.

Once the plugin is built, you can run it using:
--
$ clang++ -fsyntax-only -Xclang -load -Xclang  ~/D-CMA/LLVM/build/lib/ExtractFunctions.so -Xclang -plugin -Xclang extract-funcs -Xclang -plugin-arg-extract-funcs -Xclang /home/qbit/hi.log functional_test.cpp 
$ clang++  -Xclang -load -Xclang  ~/D-CMA/LLVM/build/lib/ExtractFunctions.so -Xclang -add-plugin -Xclang extract-funcs -Xclang -plugin-arg-extract-funcs -Xclang /home/qbit/hi.log functional_test.cpp -o functional_test
