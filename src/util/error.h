#ifndef EAX_ERROR_H
#define EAX_ERROR_H

#include <iostream>
#include <cstdlib>
#include <llvm/ADT/StringRef.h>

namespace eax {

/// Prints the arguments to stderr and aborts the program.
template<typename T, typename... Ts>
[[noreturn]] void fatalError(T&& arg, Ts&&... args) {
  std::cerr << std::forward<T>(arg);
  using expander = int[];
  (void)expander{0, (void(std::cerr << std::forward<Ts>(args)), 0)...};
  std::cerr << std::endl;
  std::exit(1);
}

}

#endif
