rm -rf ./ninja_build
mkdir ./ninja_build

cmake llvm -DLLVM_ENABLE_PROJECTS="clang" -B ninja_build -G Ninja -DCMAKE_BUILD_TYPE=Release -DLLVM_CREATE_XCODE_TOOLCHAIN=ON

cd ninja_build

ninja

sudo ninja install-xcode-toolchain && sudo rm -rf  /Library/Developer/Toolchains/LLVM15.0.0git.xctoolchain && sudo mv /usr/local/Toolchains/LLVM15.0.0git.xctoolchain /Library/Developer/Toolchains


