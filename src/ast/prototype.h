#ifndef EAX_PROTOTYPE_H
#define EAX_PROTOTYPE_H

#include <vector>
#include <string>

#include "ast_node.h"

namespace eax {

/// Represents a function prototype.
class Prototype : public AstNode {
public:
  Prototype(std::string name, std::vector<std::string> paramNames)
    : name(std::move(name)), paramNames(std::move(paramNames)) {}
  void accept(AstVisitor& visitor) override { visitor.visit(*this); }
  llvm::StringRef getName() const { return name; }
  llvm::ArrayRef<std::string> getParamNames() const { return paramNames; }

private:
  std::string name;
  std::vector<std::string> paramNames;
};

}

#endif
