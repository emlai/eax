#ifndef EAX_JIT_H
#define EAX_JIT_H

#include <memory>
#include <vector>
#include <string>
#include <llvm/ExecutionEngine/Orc/ObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>

namespace eax {

/// A simple JIT compiler, based on Kaleidoscope JIT
/// (https://llvm.org/svn/llvm-project/llvm/trunk/examples/Kaleidoscope/include/KaleidoscopeJIT.h)
class JIT {
public:
  using ObjLayerT = llvm::orc::ObjectLinkingLayer<>;
  using CompileLayerT = llvm::orc::IRCompileLayer<ObjLayerT>;
  using ModuleHandleT = CompileLayerT::ModuleSetHandleT;
  
public:
  JIT();
  llvm::TargetMachine& getTargetMachine() { return *targetMachine; }
  ModuleHandleT addModule(std::unique_ptr<llvm::Module>);
  void removeModule(ModuleHandleT);
  llvm::orc::JITSymbol findSymbol(std::string const& name);
  
private:
  std::string mangle(std::string const& name);
  llvm::orc::JITSymbol findMangledSymbol(std::string const& name);
  
private:
  std::unique_ptr<llvm::TargetMachine> targetMachine;
  llvm::DataLayout const dataLayout;
  ObjLayerT objectLayer;
  CompileLayerT compileLayer;
  std::vector<ModuleHandleT> moduleHandles;
};

}

#endif
