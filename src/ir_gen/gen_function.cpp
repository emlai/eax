#include <llvm/IR/Verifier.h>
#include <llvm/IR/Function.h>

#include "ir_gen.h"
#include "ir_builder.h"
#include "../ast/function.h"
#include "../util/error.h"

using namespace eax;

void IrGen::createParamAllocas(Prototype const& proto, llvm::Function* fn) {
  llvm::Function::arg_iterator argIter = fn->arg_begin();
  
  for (auto& paramName : proto.getParamNames()) {
    llvm::AllocaInst* alloca = createEntryBlockAlloca(fn, paramName);
    builder.CreateStore(argIter, alloca);
    namedValues[paramName] = alloca;
    ++argIter;
  }
}

llvm::Function* IrGen::getFunction(llvm::StringRef name) {
  // First, see if the function has already been added to the current module.
  if (auto fn = module->getFunction(name)) return fn;
  
  // If not, check whether we can codegen the declaration from some existing
  // prototype.
  auto iterator = fnPrototypes.find(name);
  if (iterator != fnPrototypes.end()) {
    iterator->second->accept(*this);
    return dynamic_cast<llvm::Function*>(values.top());
  }
  
  // If no existing prototype exists, return null.
  return nullptr;
}

void IrGen::visit(Function& function) {
  auto& prototypeRef = *function.getPrototype();
  llvm::StringRef prototypeName = function.getPrototype()->getName();
  fnPrototypes[prototypeName] = std::move(function.getPrototype());
  auto fn = getFunction(prototypeName);
  assert(fn);
  
  auto basicBlock = llvm::BasicBlock::Create(llvm::getGlobalContext(),
                                             "entry",
                                             fn);
  builder.SetInsertPoint(basicBlock);
  
  namedValues.clear();
  createParamAllocas(prototypeRef, fn);
  
  function.getBody().accept(*this);
  if (auto value = values.top()) {
    values.pop();
    builder.CreateRet(value);
    llvm::verifyFunction(*fn);
    fnPassManager->run(*fn);
    values.push(fn);
    return;
  }
  
  // Error reading body, remove function.
  fn->eraseFromParent();
}
