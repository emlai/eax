#include <cstdlib>
#include <unordered_map>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

#include "expr.h"
#include "globals.h"
#include "../util/error.h"
#include "../util/macros.h"

using namespace eax;

llvm::Value* VariableExpr::codegen() const {
  return namedValues.at(name);
}

llvm::Value* BinaryExpr::codegen() const {
  auto left = lhs->codegen();
  auto right = rhs->codegen();
  
  switch (op) {
  case '+': return builder.CreateFAdd(left, right, "addtmp");
  case '-': return builder.CreateFSub(left, right, "subtmp");
  case '*': return builder.CreateFMul(left, right, "multmp");
  case '/': return builder.CreateFDiv(left, right, "divtmp");
  case '>': std::swap(left, right); eax_fallthrough;
  case '<':
    left = builder.CreateFCmpULT(left, right, "cmptmp");
    // Convert boolean 0 or 1 to double 0.0 or 1.0.
    return builder.CreateUIToFP(left,
                                llvm::Type::getDoubleTy(llvm::getGlobalContext()),
                                "booltmp");
  default:
    fatalError("unsupported binary operator");
  }
}

llvm::Value* CallExpr::codegen() const {
  llvm::Function* fn = getFunction(fnName);
  if (!fn) {
    fatalError("unknown function name");
  }
  if (fn->arg_size() != args.size()) {
    fatalError("wrong number of arguments");
  }
  
  std::vector<llvm::Value*> argValues;
  argValues.reserve(args.size());
  
  for (auto const& arg : args) {
    argValues.push_back(arg->codegen());
    assert(argValues.back());
  }
  
  return builder.CreateCall(fn, argValues, "calltmp");
}

llvm::Value* NumberExpr::codegen() const {
  return llvm::ConstantFP::get(llvm::getGlobalContext(),
                               llvm::APFloat(value));
}
