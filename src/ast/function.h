#ifndef EAX_FUNCTION_H
#define EAX_FUNCTION_H

#include <memory>

#include "ast_node.h"
#include "prototype.h"
#include "expr.h"

namespace eax {

/// Represents a function definition.
class Function : public AstNode {
public:
  Function(std::unique_ptr<Prototype> prototype, std::unique_ptr<Expr> body)
    : prototype(std::move(prototype)), body(std::move(body)) {}
  void accept(AstVisitor& visitor) override { visitor.visit(*this); }
  std::unique_ptr<Prototype>& getPrototype() { return prototype; }
  Expr& getBody() { return *body; }
  
private:
  std::unique_ptr<Prototype> prototype;
  std::unique_ptr<Expr> body;
};

}

#endif
