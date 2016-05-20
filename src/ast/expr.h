#ifndef EAX_EXPR_H
#define EAX_EXPR_H

#include <memory>
#include <vector>
#include <string>
#include <llvm/ADT/ArrayRef.h>

#include "ast_node.h"

namespace eax {

/// Base class for all expression nodes.
class Expr : public AstNode {
public:
  virtual ~Expr() = default;
};

/// Expression class for referencing variables.
class VariableExpr : public Expr {
public:
  VariableExpr(std::string name) : name(std::move(name)) {}
  void accept(AstVisitor& visitor) override { visitor.visit(*this); }
  std::string const& getName() const { return name; }
  
private:
  std::string name;
};

/// Expression class for unary operations.
class UnaryExpr : public Expr {
public:
  UnaryExpr(char op, std::unique_ptr<Expr> operand)
    : op(op), operand(std::move(operand)) {}
  void accept(AstVisitor& visitor) override { visitor.visit(*this); }
  char getOp() const { return op; }
  Expr& getOperand() const { return *operand; }
  
private:
  char op;
  std::unique_ptr<Expr> operand;
};

/// Expression class for binary operations.
class BinaryExpr : public Expr {
public:
  BinaryExpr(int op, std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs)
    : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
  void accept(AstVisitor& visitor) override { visitor.visit(*this); }
  int getOp() const { return op; }
  Expr& getLhs() const { return *lhs; }
  Expr& getRhs() const { return *rhs; }
  
private:
  int op;
  std::unique_ptr<Expr> lhs;
  std::unique_ptr<Expr> rhs;
};

/// Expression class for function calls.
class CallExpr : public Expr {
public:
  CallExpr(std::string fnName, std::vector<std::unique_ptr<Expr>> args)
    : fnName(std::move(fnName)), args(std::move(args)) {}
  void accept(AstVisitor& visitor) override { visitor.visit(*this); }
  std::string const& getName() const { return fnName; }
  llvm::ArrayRef<std::unique_ptr<Expr>> getArgs() const { return args; }
  
private:
  std::string fnName;
  std::vector<std::unique_ptr<Expr>> args;
};

/// Expression class for numeric literals.
class NumberExpr : public Expr {
public:
  NumberExpr(double value) : value(value) {}
  void accept(AstVisitor& visitor) override { visitor.visit(*this); }
  double getValue() const { return value; }
  
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
  void accept(AstVisitor& visitor) override { visitor.visit(*this); }
  Expr& getCondition() { return *condition; }
  Expr& getThen() { return *thenBranch; }
  Expr& getElse() { return *elseBranch; }
  
private:
  std::unique_ptr<Expr> condition;
  std::unique_ptr<Expr> thenBranch;
  std::unique_ptr<Expr> elseBranch;
};

}

#endif
