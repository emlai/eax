#ifndef EAX_FUNCTION_H
#define EAX_FUNCTION_H

#include <memory>
#include <llvm/IR/Function.h>

#include "prototype.h"
#include "expr.h"

namespace eax {

/// Represents a function definition.
class Function {
public:
  Function(std::unique_ptr<Prototype> prototype, std::unique_ptr<Expr> body)
    : prototype(std::move(prototype)), body(std::move(body)) {}
  llvm::Function* codegen();
  
private:
  std::unique_ptr<Prototype> prototype;
  std::unique_ptr<Expr> body;
};

}

#endif
