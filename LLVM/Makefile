LLVM_DIR = ${CURDIR}/llvm
BUILD_DIR = ${CURDIR}/build

cmasan:
	mkdir -p ${BUILD_DIR}
	(cd ${BUILD_DIR} && \
	  CC=gcc CXX=g++ \
	  cmake -G "Ninja"\
	  -DCMAKE_BUILD_TYPE=Release \
	  -DCMAKE_C_COMPILER=gcc \
	  -DCMAKE_CXX_COMPILER=g++ \
	  -DLLVM_ENABLE_PROJECTS="llvm;clang;compiler-rt;lld" \
	  -DCLANG_ENABLE_OPAQUE_POINTERS=OFF \
	  -DLLVM_ENABLE_ASSERTIONS=OFF \
	  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
          -DLLVM_BINUTILS_INCDIR="${BUILD_DIR}/../binutils/include" \
	  -DLLVM_BUILD_TESTS=OFF \
	  -DLLVM_BUILD_EXAMPLES=OFF \
	  -DLLVM_INCLUDE_TESTS=OFF \
      -DLLVM_INCLUDE_BENCHMARKS=OFF \
	  -DLLVM_INCLUDE_EXAMPLES=OFF \
	  -DLLVM_TARGETS_TO_BUILD="X86" \
	  -DBUILD_SHARED_LIBS=ON \
	  -DCLANG_BUILD_EXAMPLES=ON \
	${LLVM_DIR})
	(ninja -C ${BUILD_DIR})

test:
	(cd ${BUILD_DIR} && ninja check-asan)

clean:
	rm -rf ${BUILD_DIR}./bu

.PHONY: cmasan test clean
