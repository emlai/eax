#include <llvm/IR/LLVMContext.h>

#include "globals.h"
#include "prototype.h"

namespace eax {

std::unique_ptr<llvm::Module> globalModule;
llvm::IRBuilder<> builder(llvm::getGlobalContext());
std::unique_ptr<llvm::legacy::FunctionPassManager> fnPassManager;
std::unordered_map<std::string, llvm::Value*> namedValues;
std::unordered_map<std::string, std::unique_ptr<Prototype>> fnPrototypes;

llvm::Function* getFunction(llvm::StringRef name) {
  // First, see if the function has already been added to the current module.
  if (auto fn = globalModule->getFunction(name)) return fn;
  
  // If not, check whether we can codegen the declaration from some existing
  // prototype.
  auto iterator = fnPrototypes.find(name);
  if (iterator != fnPrototypes.end()) {
    return iterator->second->codegen();
  }
  
  // If no existing prototype exists, return null.
  return nullptr;
}

}
