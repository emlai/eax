#ifndef EAX_ERROR_H
#define EAX_ERROR_H

#include <iostream>
#include <cstdlib>

namespace eax {

/// Prints the arguments to stderr and returns nullptr.
template<typename... Ts>
std::nullptr_t error(Ts&&... args) {
  using expander = int[];
  (void)expander{0, (void(std::cerr << std::forward<Ts>(args)), 0)...};
  std::cerr << std::endl;
  return nullptr;
}

/// Prints the arguments to stderr and aborts the program.
template<typename... Ts>
[[noreturn]] void fatalError(Ts&&... args) {
  error(std::forward<Ts>(args)...);
  std::exit(1);
}

}

#endif
