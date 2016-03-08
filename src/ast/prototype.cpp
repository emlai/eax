#include <llvm/IR/LLVMContext.h>

#include "prototype.h"
#include "globals.h"

using namespace eax;

llvm::Function* Prototype::codegen() const {
  std::vector<llvm::Type*> doubles(paramNames.size(),
    llvm::Type::getDoubleTy(llvm::getGlobalContext()));
  
  auto fnType = llvm::FunctionType::get(
    llvm::Type::getDoubleTy(llvm::getGlobalContext()), doubles, false);
  
  auto fn = llvm::Function::Create(fnType,
                                   llvm::Function::ExternalLinkage,
                                   name,
                                   globalModule.get());
  
  auto paramNameIter = paramNames.begin();
  for (auto& arg : fn->args()) {
    arg.setName(*paramNameIter++);
  }
  
  return fn;
}

void Prototype::createParamAllocas(llvm::Function* fn) const {
  llvm::Function::arg_iterator argIter = fn->arg_begin();
  
  for (auto& paramName : paramNames) {
    llvm::AllocaInst* alloca = createEntryBlockAlloca(fn, paramName);
    builder.CreateStore(argIter, alloca);
    namedValues[paramName] = alloca;
    ++argIter;
  }
}
