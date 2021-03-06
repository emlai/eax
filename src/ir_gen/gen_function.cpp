#include <llvm/IR/Verifier.h>
#include <llvm/IR/Function.h>

#include "ir_gen.h"
#include "../ast/function.h"
#include "../util/error.h"

using namespace eax;

void IrGen::createParamAllocas(Prototype const& proto, llvm::Function* fn) {
  llvm::Function::arg_iterator argIter = fn->arg_begin();
  
  for (auto& paramName : proto.getParamNames()) {
    llvm::AllocaInst* alloca = createEntryBlockAlloca(fn, paramName);
    builder.CreateStore(&*argIter, alloca);
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

llvm::Function* IrGen::initFunction(Function& function, Prototype& proto) {
  auto fn = getFunction(proto.getName());
  assert(fn);
  
  auto basicBlock = llvm::BasicBlock::Create(context, "entry", fn);
  builder.SetInsertPoint(basicBlock);
  
  namedValues.clear();
  createParamAllocas(proto, fn);
  
  function.getBody().accept(*this);
  return fn;
}

void IrGen::visit(Function& function) {
  auto& proto = *function.getPrototype();
  fnPrototypes[proto.getName()] = std::move(function.getPrototype());
  llvm::Function* fn = initFunction(function, proto);
  
  if (auto value = values.top()) {
    values.pop();
    returnType = value->getType();
    
    // Recreate function with correct return type.
    fn->eraseFromParent();
    fn = initFunction(function, proto);
    value = values.top();
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

llvm::AllocaInst* IrGen::createEntryBlockAlloca(llvm::Function* fn,
                                                llvm::StringRef varName) {
  llvm::IRBuilder<> tmpBuilder(&fn->getEntryBlock(), fn->getEntryBlock().begin());
  return tmpBuilder.CreateAlloca(llvm::Type::getDoubleTy(context), 0, varName);
}
