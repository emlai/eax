#ifndef EAX_IR_BUILDER_H
#define EAX_IR_BUILDER_H

#include <llvm/IR/IRBuilder.h>

namespace eax {

/// Creates an 'alloca' instruction in the entry block of the given
/// function. This is used for mutable variables etc.
llvm::AllocaInst* createEntryBlockAlloca(llvm::Function* fn,
                                         llvm::StringRef varName);

}

#endif
