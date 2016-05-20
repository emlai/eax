#include <llvm/IR/LLVMContext.h>

#include "ir_builder.h"

using namespace eax;

llvm::AllocaInst* eax::createEntryBlockAlloca(llvm::Function* fn,
                                         llvm::StringRef varName) {
  llvm::IRBuilder<> tmpBuilder(&fn->getEntryBlock(), fn->getEntryBlock().begin());
  return tmpBuilder.CreateAlloca(llvm::Type::getDoubleTy(llvm::getGlobalContext()),
                                 0,
                                 varName);
}
