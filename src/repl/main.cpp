#include <iostream>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Transforms/Scalar.h>

#include "jit.h"
#include "../ast/function.h"
#include "../ast/globals.h"
#include "../parser/lexer.h"

using namespace eax;

static std::unique_ptr<JIT> jit;
static Lexer lexer;

static void initModuleAndFnPassManager() {
  globalModule = llvm::make_unique<llvm::Module>("eaxjit", llvm::getGlobalContext());
  globalModule->setDataLayout(jit->getTargetMachine().createDataLayout());
  
  fnPassManager = llvm::make_unique<llvm::legacy::FunctionPassManager>(globalModule.get());
  // Do simple "peephole" and bit-twiddling  optimizations.
  fnPassManager->add(llvm::createInstructionCombiningPass());
  // Reassociate expressions.
  fnPassManager->add(llvm::createReassociatePass());
  // Eliminate common subexpressions.
  fnPassManager->add(llvm::createGVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc.).
  fnPassManager->add(llvm::createCFGSimplificationPass());
  fnPassManager->doInitialization();
}

static void handleFnDefinition() {
  if (auto fn = lexer.parseFnDefinition()) {
    if (auto ir = fn->codegen()) {
      std::cout << "Parsed a function definition:" << std::endl;
      ir->dump();
      jit->addModule(std::move(globalModule));
      initModuleAndFnPassManager();
    }
  } else {
    lexer.nextToken(); // Skip token for error recovery.
  }
}

static void handleToplevelExpr() {
  // Evaluate a top-level expression into an anonymous function.
  if (auto fn = lexer.parseToplevelExpr()) {
    if (fn->codegen()) {
      std::cout << "Parsed a top-level expression." << std::endl;
      
      // JIT the module containing the anonymous expression,
      // keeping a handle so we can free it layer;
      auto moduleHandle = jit->addModule(std::move(globalModule));
      initModuleAndFnPassManager();
      
      auto exprSym = jit->findSymbol("__anon_expr");
      assert(exprSym && "function not found");
      
      // Get the symbol's address and cast it to the right type
      // so we can call it as a native function.
      auto fnPtr = reinterpret_cast<double(*)()>(exprSym.getAddress());
      std::cout << "Evaluated to " << fnPtr() << std::endl;
      
      // Delete the anonymous expression module from the JIT.
      jit->removeModule(moduleHandle);
    }
  } else {
    lexer.nextToken(); // Skip token for error recovery.
  }
}

static void mainInterpreterLoop() {
  while (true) {
    std::cout << " > ";
    
    switch (lexer.nextToken()) {
    case TokenEof:
      return;
    case ';':
      // Ignore top-level semicolons.
      break;
    case TokenDef:
      handleFnDefinition();
      break;
    default:
      handleToplevelExpr();
      break;
    }
  }
}

int main(int argc, char** argv) {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();
  
  jit = llvm::make_unique<JIT>();
  initModuleAndFnPassManager();
  
  mainInterpreterLoop();
}
