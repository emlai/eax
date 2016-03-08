#include <llvm/IR/Verifier.h>

#include "function.h"
#include "globals.h"
#include "../util/error.h"

using namespace eax;

llvm::Function* Function::codegen() {
  auto& prototypeRef = *prototype;
  llvm::StringRef prototypeName = prototype->getName();
  fnPrototypes[prototypeName] = std::move(prototype);
  auto fn = getFunction(prototypeName);
  assert(fn);
  
  auto basicBlock = llvm::BasicBlock::Create(llvm::getGlobalContext(),
                                             "entry",
                                             fn);
  builder.SetInsertPoint(basicBlock);
  
  namedValues.clear();
  prototypeRef.createParamAllocas(fn);
  
  if (auto value = body->codegen()) {
    builder.CreateRet(value);
    llvm::verifyFunction(*fn);
    fnPassManager->run(*fn);
    return fn;
  }
  
  // Error reading body, remove function.
  fn->eraseFromParent();
  return nullptr;
}
