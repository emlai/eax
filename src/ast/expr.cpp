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

static llvm::Value* boolToDouble(llvm::Value* boolean) {
  return builder.CreateUIToFP(boolean,
                              llvm::Type::getDoubleTy(llvm::getGlobalContext()),
                              "booltmp");
}

llvm::Value* VariableExpr::codegen() const {
  return builder.CreateLoad(namedValues.at(name), name);
}

llvm::Value* UnaryExpr::codegen() const {
  llvm::Value* operandValue = operand->codegen();
  if (!operandValue) return nullptr;
  
  switch (op) {
  case '!':
    return boolToDouble(builder.CreateFCmpOEQ(
      operandValue,
      llvm::ConstantFP::get(llvm::getGlobalContext(), llvm::APFloat(0.0)),
      "negtmp"));
  default:
    fatalError("unsupported unary operator");
  }
}

llvm::Value* BinaryExpr::codegen() const {
  // Special case for '=' because we don't
  // want to emit lhs as an expression.
  if (op == '=')
    return codegenAssignment();
  
  auto left = lhs->codegen();
  auto right = rhs->codegen();
  
  switch (op) {
  case '+': return builder.CreateFAdd(left, right, "addtmp");
  case '-': return builder.CreateFSub(left, right, "subtmp");
  case '*': return builder.CreateFMul(left, right, "multmp");
  case '/': return builder.CreateFDiv(left, right, "divtmp");
  case '==':
    return boolToDouble(builder.CreateFCmpOEQ(left, right, "eqltmp"));
  case '>': std::swap(left, right); eax_fallthrough;
  case '<':
    return boolToDouble(builder.CreateFCmpULT(left, right, "cmptmp"));
  default:
    fatalError("unsupported binary operator");
  }
}

llvm::Value* BinaryExpr::codegenAssignment() const {
  VariableExpr* lhsVar = dynamic_cast<VariableExpr*>(lhs.get());
  if (!lhsVar) fatalError("left operand of '=' must be a variable");
  
  llvm::Value* rhsValue = rhs->codegen();
  if (!rhsValue) return nullptr;
  
  auto variableIter = namedValues.find(lhsVar->getName());
  if (variableIter == namedValues.end())
    fatalError("unknown variable name");
  
  builder.CreateStore(rhsValue, variableIter->second);
  return rhsValue;
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

llvm::Value* IfExpr::codegen() const {
  // condition
  
  llvm::Value* conditionValue = condition->codegen();
  if (!conditionValue) return nullptr;
  
  // Convert condition to a bool by comparing to 0.
  conditionValue = builder.CreateFCmpONE(
    conditionValue,
    llvm::ConstantFP::get(llvm::getGlobalContext(), llvm::APFloat(0.0)),
    "ifcond");
  
  llvm::Function* fn = builder.GetInsertBlock()->getParent();
  
  llvm::BasicBlock* thenBlock = llvm::BasicBlock::Create(
    llvm::getGlobalContext(), "then", fn);
  llvm::BasicBlock* elseBlock = llvm::BasicBlock::Create(
    llvm::getGlobalContext(), "else");
  llvm::BasicBlock* mergeBlock = llvm::BasicBlock::Create(
    llvm::getGlobalContext(), "ifcont");
  
  builder.CreateCondBr(conditionValue, thenBlock, elseBlock);
  
  // then
  
  builder.SetInsertPoint(thenBlock);
  
  llvm::Value* thenValue = thenBranch->codegen();
  if (!thenValue) return nullptr;
  
  builder.CreateBr(mergeBlock);
  thenBlock = builder.GetInsertBlock();
  
  // else
  
  fn->getBasicBlockList().push_back(elseBlock);
  builder.SetInsertPoint(elseBlock);
  
  llvm::Value* elseValue = elseBranch->codegen();
  if (!elseValue) return nullptr;
  
  builder.CreateBr(mergeBlock);
  elseBlock = builder.GetInsertBlock();
  
  // merge
  
  fn->getBasicBlockList().push_back(mergeBlock);
  builder.SetInsertPoint(mergeBlock);
  
  llvm::PHINode* phi = builder.CreatePHI(
    llvm::Type::getDoubleTy(llvm::getGlobalContext()), 2, "iftmp");
  phi->addIncoming(thenValue, thenBlock);
  phi->addIncoming(elseValue, elseBlock);
  return phi;
}
