#ifndef EAX_GLOBALS_H
#define EAX_GLOBALS_H

#include <memory>
#include <unordered_map>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Value.h>

namespace eax {

class Prototype;

extern std::unique_ptr<llvm::Module> globalModule;
extern llvm::IRBuilder<> builder;
extern std::unique_ptr<llvm::legacy::FunctionPassManager> fnPassManager;

/// Holds the names of all named values currently in scope.
extern std::unordered_map<std::string, llvm::Value*> namedValues;

/// Holds the most recent prototype for each function.
extern std::unordered_map<std::string, std::unique_ptr<Prototype>> fnPrototypes;

/// Searches globalModule for an existing function declaration with the given name,
/// or, if it doesn't find one, generates a new one from FnPrototypes.
llvm::Function* getFunction(llvm::StringRef name);

}

#endif
