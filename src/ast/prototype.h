#ifndef EAX_PROTOTYPE_H
#define EAX_PROTOTYPE_H

#include <vector>
#include <string>
#include <llvm/IR/Function.h>

namespace eax {

/// Represents a function prototype.
class Prototype {
public:
  Prototype(std::string name, std::vector<std::string> paramNames)
    : name(std::move(name)), paramNames(std::move(paramNames)) {}
  llvm::Function* codegen() const;
  llvm::StringRef getName() const { return name; }
  
private:
  std::string name;
  std::vector<std::string> paramNames;
};

}

#endif
