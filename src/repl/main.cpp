#include <iostream>
#include <iomanip>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Transforms/Scalar.h>
#include "llvm/Transforms/Scalar/GVN.h"

#include "jit.h"
#include "../ast/function.h"
#include "../ast/ast_printer.h"
#include "../parser/lexer.h"
#include "../ir_gen/ir_gen.h"
#include "../util/error.h"

using namespace eax;

static std::unique_ptr<JIT> jit;
static Lexer lexer;
static llvm::LLVMContext llvmContext;
static IrGen irgen(llvmContext);
static AstPrinter printer(std::cout);
static std::unique_ptr<llvm::Module> globalModule;
static std::unique_ptr<llvm::legacy::FunctionPassManager> fnPassManager;

static void initModuleAndFnPassManager() {
  globalModule = llvm::make_unique<llvm::Module>("eaxjit", llvmContext);
  globalModule->setDataLayout(jit->getTargetMachine().createDataLayout());
  
  fnPassManager = llvm::make_unique<llvm::legacy::FunctionPassManager>(globalModule.get());
  // Promote allocas to registers.
  fnPassManager->add(llvm::createPromoteMemoryToRegisterPass());
  // Do simple "peephole" and bit-twiddling  optimizations.
  fnPassManager->add(llvm::createInstructionCombiningPass());
  // Reassociate expressions.
  fnPassManager->add(llvm::createReassociatePass());
  // Eliminate common subexpressions.
  fnPassManager->add(llvm::createGVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc.).
  fnPassManager->add(llvm::createCFGSimplificationPass());
  fnPassManager->doInitialization();
  
  irgen.setModule(*globalModule);
  irgen.setFnPassManager(*fnPassManager);
}

static std::string evaluate(llvm::orc::TargetAddress addr, llvm::Type* type) {
  if (type == llvm::Type::getInt1Ty(llvmContext)) {
    return (reinterpret_cast<bool(*)()>(addr)() ? "true" : "false");
  }
  if (type == llvm::Type::getDoubleTy(llvmContext)) {
    return std::to_string(reinterpret_cast<double(*)()>(addr)());
  }
  std::string typeName;
  llvm::raw_string_ostream stream(typeName);
  type->print(stream);
  return "unknown type '" + stream.str() + "'";
}

static void handleFnDefinition() {
  if (auto fn = lexer.parseFnDefinition()) {
    fn->accept(irgen);
    if (auto ir = irgen.getResult()) {
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
    fn->accept(irgen);
    if (auto ir = irgen.getResult()) {
      // Get the type of the expression.
      llvm::Type* type = ir->getType()->getPointerElementType();
      type = llvm::cast<llvm::FunctionType>(type)->getReturnType();
      
      // JIT the module containing the anonymous expression,
      // keeping a handle so we can free it later.
      auto moduleHandle = jit->addModule(std::move(globalModule));
      initModuleAndFnPassManager();
      
      auto exprSym = jit->findSymbol("__anon_expr");
      assert(exprSym && "function not found");
      
      std::cout << evaluate(exprSym.getAddress(), type)
                << std::endl;
      
      // Delete the anonymous expression module from the JIT.
      jit->removeModule(moduleHandle);
    }
  } else {
    lexer.nextToken(); // Skip token for error recovery.
  }
}

static void mainInterpreterLoop() {
  std::cout << std::setfill('0');
  
  for (int count = 0;; ++count) {
    std::cout << "\e[2m" << std::setw(3) << count << ">\e[22m ";
    
    switch (lexer.nextToken()) {
    case TokenEof:
      return;
    case '\n':
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
