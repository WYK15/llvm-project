#!/bin/bash

if [ $# -eq 1 ]; then
  echo "passname is: $1"
else
  echo "input a passname!, exit"
  exit 0
fi

passname=$1

llvm_project_path='/Users/wangyankun/Documents/onebyone/2023-3/llvm-project'
pass_register_file_path=$llvm_project_path/llvm/lib/Passes/PassRegistry.def
passbuilder_file_path=$llvm_project_path/llvm/lib/Passes/PassBuilder.cpp
obfuscator_dic_name='Obfuscator'

obfuscator_lib_path=$llvm_project_path/llvm/lib/Transforms/$obfuscator_dic_name
header_dic=$llvm_project_path/llvm/include/llvm/Transforms/$obfuscator_dic_name

CMakeLists_file=$obfuscator_lib_path/CMakeLists.txt

header_file_name=${passname}.h
cpp_file_name=${passname}.cpp

header_file_path=$header_dic/$header_file_name
cpp_file_path=$obfuscator_lib_path/$cpp_file_name

transform_cmake_list=$llvm_project_path/llvm/lib/Transforms/CMakeLists.txt

# create project dictionary 
if ! [ -d $obfuscator_lib_path ]; then
  mkdir -p $obfuscator_lib_path
fi

if ! [ -d $header_dic ]; then
  mkdir -p $header_dic
fi

if ! [ -e $CMakeLists_file ]; then
  touch $CMakeLists_file
fi

# add obfuscator directory to cmakelist
echo -e "add_subdirectory(${obfuscator_dic_name})\n" $transform_cmake_list

# code cmakelist.txt
echo -e 'add_llvm_component_library(LLVMObfuscator\n' >> $CMakeLists_file
echo -e $passname >> $CMakeLists_file
echo -e '\n' >> $CMakeLists_file

echo -e 'ADDITIONAL_HEADER_DIRS\n' >> $CMakeLists_file
echo -e '${LLVM_MAIN_INCLUDE_DIR}/llvm/Transforms\n' >> $CMakeLists_file
echo -e '${LLVM_MAIN_INCLUDE_DIR}/llvm/Transforms/Utils\n' >> $CMakeLists_file
echo -e '\n' >> $CMakeLists_file

echo -e 'DEPENDS\n' >> $CMakeLists_file
echo -e 'intrinsics_gen\n' >> $CMakeLists_file

echo -e 'LINK_COMPONENTS\n' >> $CMakeLists_file
echo -e 'Analysis\n' >> $CMakeLists_file
echo -e 'Core\n' >> $CMakeLists_file
echo -e 'Support\n' >> $CMakeLists_file

echo -e ')\n' >> $CMakeLists_file


# code .h
rm -rf $header_file_path
touch $header_file_path

echo -e "#ifndef LLVM_TRANSFORMS_${passname}_H \n" >> $header_file_path
echo -e "#define LLVM_TRANSFORMS_${passname}_H \n" >> $header_file_path

echo -e '#include "llvm/IR/PassManager.h" \n' >> $header_file_path

echo -e 'namespace llvm { \n' >> $header_file_path
echo -e "  class ${passname}Pass : public PassInfoMixin<${passname}Pass> { \n" >> $header_file_path

echo -e '   public: \n' >> $header_file_path
echo -e '   PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM); \n' >> $header_file_path

echo -e '  };\n' >> $header_file_path
echo -e '} \n' >> $header_file_path

echo -e '#endif  \n' >> $header_file_path


# code .cpp
rm -rf $cpp_file_path
touch $cpp_file_path

echo -e '#include \"$header_file_path\"' >> $cpp_file_path
echo -e 'using namespace llvm; \n' >> $cpp_file_path
echo -e "PreservedAnalyses ${passname}Pass::run(Function &F, FunctionAnalysisManager &AM) { \n"  >> $cpp_file_path
echo -e 'errs() << F.getName(); \n' >> $cpp_file_path
echo -e '	return PreservedAnalyses::all(); \n' >> $cpp_file_path
echo -e '} \n' >> $cpp_file_path

# https://releases.llvm.org/15.0.0/docs/WritingAnLLVMNewPMPass.html
# That’s it for the pass itself. Now in order to “register” the pass, 
# we need to add it to a couple places. 
# Add the following to llvm/lib/Passes/PassRegistry.def in the FUNCTION_PASS section
sed -i '' "272a\
FUNCTION_PASS(\"${passname}\", ${passname}Pass())
" $pass_register_file_path


# we need to also add the proper #include in llvm/lib/Passes/PassBuilder.cpp:
sed -i '' "20a\
#include ${header_file_path}
" $pass_register_file_path


# 在llvm-project/clang/lib/CodeGen/BackendUtil.cpp的addSanitizers()中，调用PM.registerxxxx方法注册
# TODO: 添加进入cpp的addSanitizers方法中

# PB.registerScalarOptimizerLateEPCallback([&](FunctionPassManager &FPM, OptimizationLevel level){
#   FPM.addPass(FlattenPass());
# });


echo "create succeed!"
