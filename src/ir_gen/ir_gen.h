#ifndef EAX_IR_GEN_H
#define EAX_IR_GEN_H

#include <stack>
#include <memory>
#include <unordered_map>
#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Instructions.h>

#include "../ast/ast_visitor.h"
#include "../ast/prototype.h"

namespace eax {

class IrGen : public AstVisitor {
public:
  IrGen() : context(llvm::getGlobalContext()), builder(context) {}
  llvm::Value* getResult() const { return values.top(); }
  void setModule(llvm::Module& module) { this->module = &module; }
  void setFnPassManager(llvm::legacy::FunctionPassManager& fpm) {
    fnPassManager = &fpm;
  }
  
private:
  void visit(VariableExpr&) override;
  void visit(UnaryExpr&) override;
  void visit(BinaryExpr&) override;
  void visit(CallExpr&) override;
  void visit(NumberExpr&) override;
  void visit(BoolExpr&) override;
  void visit(IfExpr&) override;
  void visit(Function&) override;
  void visit(Prototype&) override;
  
  // Codegen helpers
  llvm::Value* boolToDouble(llvm::Value* boolean);
  llvm::Value* createLogicalNegation(llvm::Value* operand);
  llvm::Value* createEqualityComparison(llvm::Value* lhs, llvm::Value* rhs);
  llvm::Value* createInequalityComparison(llvm::Value* lhs, llvm::Value* rhs);
  void codegenAssignment(BinaryExpr&);
  void createParamAllocas(Prototype const&, llvm::Function*);
  
  /// Searches "module" for an existing function declaration with the given
  /// name, or, if it doesn't find one, generates a new one from "fnPrototypes".
  llvm::Function* getFunction(llvm::StringRef name);
  
private:
  llvm::LLVMContext& context;
  llvm::IRBuilder<> builder;
  llvm::Module* module;
  llvm::legacy::FunctionPassManager* fnPassManager;
  std::unordered_map<std::string, llvm::AllocaInst*> namedValues;
  std::unordered_map<std::string, std::unique_ptr<Prototype>> fnPrototypes;
  std::stack<llvm::Value*> values;
};

}

#endif
