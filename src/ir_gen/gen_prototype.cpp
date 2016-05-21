#include <llvm/IR/LLVMContext.h>

#include "ir_gen.h"
#include "../ast/prototype.h"

using namespace eax;

void IrGen::visit(Prototype& proto) {
  std::vector<llvm::Type*> doubles(proto.getParamNames().size(),
    llvm::Type::getDoubleTy(context));
  
  auto fnType = llvm::FunctionType::get(
    llvm::Type::getDoubleTy(context), doubles, false);
  
  auto fn = llvm::Function::Create(fnType,
                                   llvm::Function::ExternalLinkage,
                                   proto.getName(),
                                   module);
  
  auto paramNameIter = proto.getParamNames().begin();
  for (auto& arg : fn->args()) {
    arg.setName(*paramNameIter++);
  }
  
  values.push(fn);
}
