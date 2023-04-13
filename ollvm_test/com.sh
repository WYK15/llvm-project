#!/bin/sh

./bin/clang \
-mllvm -enable-hidecallobf \
-mllvm -enable-hidebrobf \
-mllvm -enable-hideGVobf \
-mllvm -enable-subobf \
-mllvm -enable-cffobf \
-mllvm -enable-bcfobf \
./main.mm  \
-o ./main_macho_x86 \
-isysroot  `xcrun  --show-sdk-path` \
-framework Foundation