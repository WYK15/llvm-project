// For open-source license, please refer to [License](https://github.com/HikariObfuscator/Hikari/wiki/License).
//===----------------------------------------------------------------------===//
#include "llvm/Transforms/Obfuscator/CryptoUtils.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/Twine.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Format.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <chrono>

using namespace llvm;
namespace llvm {
ManagedStatic<CryptoUtils> cryptoutils;
}
CryptoUtils::CryptoUtils() {

}

uint32_t CryptoUtils::scramble32(uint32_t in,std::map<uint32_t/*IDX*/,uint32_t/*VAL*/>& VMap) {
  if(VMap.find(in)==VMap.end()){
    uint32_t V=get_uint32_t();
    VMap[in]=V;
    return V;
  }
  else{
    return VMap[in];
  }
}
CryptoUtils::~CryptoUtils() {
  if(eng!=nullptr){
    delete eng;
  }
}
void CryptoUtils::prng_seed(){
  using namespace std::chrono;
  std::uint_fast64_t ms = duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count();
  errs()<<format("std::mt19937_64 seeded with current timestamp: %" PRIu64"",ms)<<"\n";
  eng=new std::mt19937_64(ms);
}
void CryptoUtils::prng_seed(std::uint_fast64_t seed){
  errs()<<format("std::mt19937_64 seeded with: %" PRIu64"",seed)<<"\n";
  eng=new std::mt19937_64(seed);
}
std::uint_fast64_t CryptoUtils::get_raw(){
  if(eng==nullptr){
    prng_seed();
  }
  return (*eng)();
}
uint32_t CryptoUtils::get_range(uint32_t min,uint32_t max) {
  if(max==0){
    return 0;
  }
  std::uniform_int_distribution<uint32_t> dis(min, max-1);
  return dis(*eng);
}

void CryptoUtils::get_bytes(char *buffer, const int len) {

//  int sofar = 0, available = 0;
//
//  assert(buffer != NULL && "CryptoUtils::get_bytes buffer=NULL");
//  assert(len > 0 && "CryptoUtils::get_bytes len <= 0");
//
//  statsGetBytes++;
//
//  if (len > 0) {
//
//    // If the PRNG is not seeded, it the very last time to do it !
//    if (!seeded) {
//      prng_seed();
//      populate_pool();
//    }
//
//    do {
//      if (idx + (len - sofar) >= CryptoUtils_POOL_SIZE) {
//        // We don't have enough bytes ready in the pool,
//        // so let's use the available ones and repopulate !
//        available = CryptoUtils_POOL_SIZE - idx;
//        memcpy(buffer + sofar, pool + idx, available);
//        sofar += available;
//        populate_pool();
//      } else {
//        memcpy(buffer + sofar, pool + idx, len - sofar);
//        idx += len - sofar;
//        // This will trigger a loop exit
//        sofar = len;
//      }
//    } while (sofar < (len - 1));
//  }
}
