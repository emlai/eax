cmake_minimum_required(VERSION 3.0)
project(eax VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 11)
set(LLVMLIBS core mcjit native)

if(NOT DEFINED LLVM_CONFIG)
  message(FATAL_ERROR "Set LLVM_CONFIG to the path to llvm-config")
endif()

execute_process(COMMAND ${LLVM_CONFIG} --cxxflags
                OUTPUT_VARIABLE LLVM_CXX_FLAGS
                OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${LLVM_CONFIG} --includedir
                OUTPUT_VARIABLE LLVM_INCLUDE_DIR
                OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${LLVM_CONFIG} --libdir
                OUTPUT_VARIABLE LLVM_LIBRARY_DIR
                OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${LLVM_CONFIG} --system-libs
                OUTPUT_VARIABLE LLVM_SYSTEM_LIBS
                OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${LLVM_CONFIG} --libs ${LLVMLIBS}
                OUTPUT_VARIABLE LLVM_LIBRARIES
                OUTPUT_STRIP_TRAILING_WHITESPACE)

string(REGEX REPLACE "-O[1-3]?" "-O0 -g" CMAKE_CXX_FLAGS "${LLVM_CXX_FLAGS} ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-multichar")

link_directories(${LLVM_LIBRARY_DIR})

file(GLOB COMPILER_SOURCES src/**/*.h src/**/*.cpp)
add_executable(eax ${COMPILER_SOURCES})

target_include_directories(eax SYSTEM PUBLIC ${LLVM_INCLUDE_DIR})
target_link_libraries(eax PUBLIC ${LLVM_SYSTEM_LIBS} ${LLVM_LIBRARIES})
