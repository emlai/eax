#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/RTDyldMemoryManager.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/LambdaResolver.h>
#include <llvm/IR/Mangler.h>
#include <llvm/Support/DynamicLibrary.h>

#include "jit.h"

using namespace eax;

JIT::JIT()
  : targetMachine(llvm::EngineBuilder().selectTarget()),
    dataLayout((assert(targetMachine), targetMachine->createDataLayout())),
    compileLayer(objectLayer, llvm::orc::SimpleCompiler(*targetMachine)) {
  llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);
}

JIT::ModuleHandleT JIT::addModule(std::unique_ptr<llvm::Module> module) {
  // We need a memory manager to allocate memory and resolve symbols for this
  // new module. Create one that resolves symbols by looking back into the JIT.
  auto resolver = llvm::orc::createLambdaResolver(
    [&](std::string const& name) {
      if (auto sym = findMangledSymbol(name)) {
        return llvm::RuntimeDyld::SymbolInfo(sym.getAddress(), sym.getFlags());
      }
      return llvm::RuntimeDyld::SymbolInfo(nullptr);
    },
    [](std::string const&) { return nullptr; });
  
  std::vector<std::unique_ptr<llvm::Module>> moduleSet;
  moduleSet.push_back(std::move(module));
  
  auto moduleHandle = compileLayer.addModuleSet(
    std::move(moduleSet),
    llvm::make_unique<llvm::SectionMemoryManager>(),
    std::move(resolver));
  
  moduleHandles.push_back(moduleHandle);
  return moduleHandle;
}

void JIT::removeModule(ModuleHandleT moduleHandle) {
  moduleHandles.erase(
    std::find(moduleHandles.begin(), moduleHandles.end(), moduleHandle));
  compileLayer.removeModuleSet(moduleHandle);
}

llvm::orc::JITSymbol JIT::findSymbol(std::string const& name) {
  return findMangledSymbol(mangle(name));
}

std::string JIT::mangle(std::string const& name) {
  std::string mangledName;
  {
    llvm::raw_string_ostream mangledNameStream(mangledName);
    llvm::Mangler::getNameWithPrefix(mangledNameStream, name, dataLayout);
  }
  return mangledName;
}

llvm::orc::JITSymbol JIT::findMangledSymbol(std::string const& name) {
  // Search modules in reverse order: from last added to first added.
  // This is the opposite of the usual search order for dlsym, but makes more
  // sense in a REPL where we want to bind to the newest available definition.
  for (auto handle : llvm::make_range(moduleHandles.rbegin(),
                                      moduleHandles.rend())) {
    if (auto sym = compileLayer.findSymbolIn(handle, name, true)) {
      return sym;
    }
  }
  
  // If we can't find the symbol in the JIT, try looking in the host process.
  if (auto symAddr = llvm::RTDyldMemoryManager::getSymbolAddressInProcess(name)) {
    return {symAddr, llvm::JITSymbolFlags::Exported};
  }
  
  return nullptr;
}
