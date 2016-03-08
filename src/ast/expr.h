#ifndef EAX_EXPR_H
#define EAX_EXPR_H

#include <memory>
#include <vector>
#include <string>
#include <llvm/IR/Value.h>

namespace eax {

/// Base class for all expression nodes.
class Expr {
public:
  virtual ~Expr() = default;
  virtual llvm::Value* codegen() const = 0;
};

/// Expression class for referencing variables.
class VariableExpr : public Expr {
public:
  VariableExpr(std::string name) : name(std::move(name)) {}
  llvm::Value* codegen() const override;
  std::string const& getName() const { return name; }
  
private:
  std::string name;
};

/// Expression class for unary operations.
class UnaryExpr : public Expr {
public:
  UnaryExpr(char op, std::unique_ptr<Expr> operand)
    : op(op), operand(std::move(operand)) {}
  llvm::Value* codegen() const override;
  
private:
  char op;
  std::unique_ptr<Expr> operand;
};

/// Expression class for binary operations.
class BinaryExpr : public Expr {
public:
  BinaryExpr(char op, std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs)
    : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
  llvm::Value* codegen() const override;
  
private:
  llvm::Value* codegenAssignment() const;
  
private:
  char op;
  std::unique_ptr<Expr> lhs;
  std::unique_ptr<Expr> rhs;
};

/// Expression class for function calls.
class CallExpr : public Expr {
public:
  CallExpr(std::string fnName, std::vector<std::unique_ptr<Expr>> args)
    : fnName(std::move(fnName)), args(std::move(args)) {}
  llvm::Value* codegen() const override;
  
private:
  std::string fnName;
  std::vector<std::unique_ptr<Expr>> args;
};

/// Expression class for numeric literals.
class NumberExpr : public Expr {
public:
  NumberExpr(double value) : value(value) {}
  llvm::Value* codegen() const override;
  
private:
  double value;
};

/// Expression class for if statements.
class IfExpr : public Expr {
public:
  IfExpr(std::unique_ptr<Expr> condition,
         std::unique_ptr<Expr> thenBranch,
         std::unique_ptr<Expr> elseBranch)
    : condition(std::move(condition)),
      thenBranch(std::move(thenBranch)),
      elseBranch(std::move(elseBranch)) {}
  llvm::Value* codegen() const override;
  
private:
  std::unique_ptr<Expr> condition;
  std::unique_ptr<Expr> thenBranch;
  std::unique_ptr<Expr> elseBranch;
};

}

#endif
