add_llvm_component_library(LLVMObfuscator
Flatten.cpp
BogusControlFlow.cpp
CryptoUtils.cpp
Utils.cpp
LegacyLowerSwitch.cpp

ADDITIONAL_HEADER_DIRS
${LLVM_MAIN_INCLUDE_DIR}/llvm/Transforms
${LLVM_MAIN_INCLUDE_DIR}/llvm/Transforms/Utils


DEPENDS
intrinsics_gen

LINK_COMPONENTS
Analysis
Core
Support
)