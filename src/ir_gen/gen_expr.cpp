#include <cstdlib>
#include <unordered_map>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

#include "ir_gen.h"
#include "../ast/expr.h"
#include "../util/error.h"
#include "../util/macros.h"

using namespace eax;

llvm::Value* IrGen::boolToDouble(llvm::Value* boolean) {
  return builder.CreateUIToFP(
    boolean, llvm::Type::getDoubleTy(context), "booltmp");
}

llvm::Value* IrGen::createLogicalNegation(llvm::Value* operand) {
  if (operand->getType() != llvm::Type::getInt1Ty(context))
    return error("logical negation requires a Bool operand");
  
  return builder.CreateICmpEQ(
    operand, llvm::ConstantInt::getFalse(context), "negtmp");
}

llvm::Value* IrGen::createEqualityComparison(llvm::Value* lhs, llvm::Value* rhs) {
  if (lhs->getType() == llvm::Type::getInt1Ty(context))
    return builder.CreateICmpEQ(lhs, rhs, "eqltmp");
  else if (lhs->getType() == llvm::Type::getDoubleTy(context))
    return builder.CreateFCmpOEQ(lhs, rhs, "eqltmp");
  else
    fatalError("unknown type");
}

llvm::Value* IrGen::createInequalityComparison(llvm::Value* lhs, llvm::Value* rhs) {
  if (lhs->getType() == llvm::Type::getInt1Ty(context))
    return builder.CreateICmpNE(lhs, rhs, "neqtmp");
  else if (lhs->getType() == llvm::Type::getDoubleTy(context))
    return builder.CreateFCmpONE(lhs, rhs, "neqtmp");
  else
    fatalError("unknown type");
}

void IrGen::visit(VariableExpr& expr) {
  auto iterator = namedValues.find(expr.getName());
  if (iterator == namedValues.end())
    return values.push(error("unknown variable '", expr.getName(), "'"));
  values.push(builder.CreateLoad(iterator->second, expr.getName()));
}

void IrGen::visit(UnaryExpr& expr) {
  expr.getOperand().accept(*this);
  llvm::Value* operandValue = values.top();
  if (!operandValue) return;
  values.pop();
  
  llvm::Value* v;
  
  switch (expr.getOp()) {
  case '!':
    v = createLogicalNegation(operandValue); break;
  case '+':
    v = operandValue; break;
  case '-':
    v = builder.CreateFSub(
      llvm::ConstantFP::get(context, llvm::APFloat(0.0)), operandValue, "subtmp");
    break;
  default:
    return values.push(error("unsupported unary operator"));
  }
  
  values.push(v);
}

void IrGen::codegenAssignment(BinaryExpr& expr) {
  VariableExpr* lhsVar = dynamic_cast<VariableExpr*>(&expr.getLhs());
  if (!lhsVar)
    return values.push(error("left operand of '=' must be a variable"));
  
  expr.getRhs().accept(*this);
  llvm::Value* rhsValue = values.top();
  if (!rhsValue) return;
  values.pop();
  
  auto variableIter = namedValues.find(lhsVar->getName());
  if (variableIter == namedValues.end())
    return values.push(error("unknown variable name"));
  
  builder.CreateStore(rhsValue, variableIter->second);
  values.push(rhsValue);
}

void IrGen::visit(BinaryExpr& expr) {
  // Special case for '=' because we don't want to emit lhs as an expression.
  if (expr.getOp() == '=') return codegenAssignment(expr);
  
  expr.getLhs().accept(*this);
  llvm::Value* left = values.top();
  values.pop();
  expr.getRhs().accept(*this);
  llvm::Value* right = values.top();
  values.pop();
  
  llvm::Value* v;
  
  switch (expr.getOp()) {
  case '+': v = builder.CreateFAdd(left, right, "addtmp"); break;
  case '-': v = builder.CreateFSub(left, right, "subtmp"); break;
  case '*': v = builder.CreateFMul(left, right, "multmp"); break;
  case '/': v = builder.CreateFDiv(left, right, "divtmp"); break;
  case '==': v = createEqualityComparison(left, right); break;
  case '!=': v = createInequalityComparison(left, right); break;
  case '>': std::swap(left, right); eax_fallthrough;
  case '<': v = builder.CreateFCmpULT(left, right, "cmptmp"); break;
  case '>=': std::swap(left, right); eax_fallthrough;
  case '<=': v = builder.CreateFCmpULE(left, right, "cmptmp"); break;
  default: return values.push(error("unsupported binary operator"));
  }
  
  values.push(v);
}

void IrGen::visit(CallExpr& expr) {
  llvm::Function* fn = getFunction(expr.getName());
  if (!fn) return values.push(error("unknown function name"));
  llvm::ArrayRef<std::unique_ptr<Expr>> const args = expr.getArgs();
  
  if (fn->arg_size() != args.size())
    return values.push(error("wrong number of arguments, expected ",
                             fn->arg_size()));
  
  std::vector<llvm::Value*> argValues;
  argValues.reserve(args.size());
  
  for (auto const& arg : args) {
    arg->accept(*this);
    argValues.push_back(values.top());
    values.pop();
    assert(argValues.back());
  }
  
  values.push(builder.CreateCall(fn, argValues, "calltmp"));
}

void IrGen::visit(NumberExpr& expr) {
  values.push(llvm::ConstantFP::get(context, llvm::APFloat(expr.getValue())));
}

void IrGen::visit(BoolExpr& expr) {
  values.push(llvm::ConstantInt::get(context, llvm::APInt(1, expr.getValue())));
}

void IrGen::visit(IfExpr& expr) {
  // condition
  
  expr.getCondition().accept(*this);
  llvm::Value* conditionValue = values.top();
  if (!conditionValue) return;
  values.pop();
  
  // Convert condition to a bool by comparing to 0.
  conditionValue = builder.CreateFCmpONE(
    conditionValue, llvm::ConstantFP::get(context, llvm::APFloat(0.0)), "ifcond");
  
  llvm::Function* fn = builder.GetInsertBlock()->getParent();
  
  llvm::BasicBlock* thenBlock = llvm::BasicBlock::Create(context, "then", fn);
  llvm::BasicBlock* elseBlock = llvm::BasicBlock::Create(context, "else");
  llvm::BasicBlock* mergeBlock = llvm::BasicBlock::Create(context, "ifcont");
  
  builder.CreateCondBr(conditionValue, thenBlock, elseBlock);
  
  // then
  
  builder.SetInsertPoint(thenBlock);
  
  expr.getThen().accept(*this);
  llvm::Value* thenValue = values.top();
  if (!thenValue) return;
  values.pop();
  
  builder.CreateBr(mergeBlock);
  thenBlock = builder.GetInsertBlock();
  
  // else
  
  fn->getBasicBlockList().push_back(elseBlock);
  builder.SetInsertPoint(elseBlock);
  
  expr.getElse().accept(*this);
  llvm::Value* elseValue = values.top();
  if (!elseValue) return;
  values.pop();
  
  builder.CreateBr(mergeBlock);
  elseBlock = builder.GetInsertBlock();
  
  // merge
  
  fn->getBasicBlockList().push_back(mergeBlock);
  builder.SetInsertPoint(mergeBlock);
  
  llvm::PHINode* phi = builder.CreatePHI(
    llvm::Type::getDoubleTy(context), 2, "iftmp");
  phi->addIncoming(thenValue, thenBlock);
  phi->addIncoming(elseValue, elseBlock);
  values.push(phi);
}
