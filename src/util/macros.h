#ifndef EAX_MACROS_H
#define EAX_MACROS_H

/// eax_fallthrough - Marks a deliberate switch case fallthrough.
#if defined(__has_cpp_attribute) && __has_cpp_attribute(clang::fallthrough)
#define eax_fallthrough [[clang::fallthrough]]
#else
#define eax_fallthrough
#endif

#endif
