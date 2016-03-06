#ifndef EAX_ERROR_H
#define EAX_ERROR_H

#include <iostream>
#include <cstdlib>
#include <llvm/ADT/StringRef.h>

namespace eax {

[[noreturn]] inline void fatalError(llvm::StringRef message) {
  std::cerr << message.str() << std::endl;
  std::exit(1);
}

}

#endif
