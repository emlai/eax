Requirements
------------
- C++11 compiler
- [CMake][1]
- [LLVM][2]

Building
--------
1. Find the `llvm-config` executable on your system.
2. Run `cmake -D LLVM_CONFIG=/path/to/llvm-config .` in the project
   root directory to generate the build system.
3. Use the generated build system to build the project, e.g. `make`.

[1]: https://cmake.org
[2]: http://llvm.org
