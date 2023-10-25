rm -rf ./ninja_build
mkdir ./ninja_build

cmake llvm -DLLVM_ENABLE_PROJECTS="clang" -B ninja_build -G Ninja -DCMAKE_BUILD_TYPE=Release -DLLVM_CREATE_XCODE_TOOLCHAIN=ON

# 若报错，不识别符号 _isPlatformVersionAtLeast , 即 项目中使用了 @available语法，是因为解析此语法的模块在compiler-rt中，因此需要在编译时，同时编译compiler-rt子模块
# cmake llvm -DLLVM_ENABLE_PROJECTS="clang;compiler-rt" -B ninja_build -G Ninja -DCMAKE_BUILD_TYPE=Release -DLLVM_CREATE_XCODE_TOOLCHAIN=ON

cd ninja_build

ninja

sudo ninja install-xcode-toolchain && sudo rm -rf  /Library/Developer/Toolchains/LLVM15.0.0git.xctoolchain && sudo mv /usr/local/Toolchains/LLVM15.0.0git.xctoolchain /Library/Developer/Toolchains


