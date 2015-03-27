cmake_minimum_required(VERSION 3.0)
project(randomstream VERSION 0.0.0)

set(WARNINGS 0 CACHE BOOL "Switch on extra warnings")

if(WARNINGS)
  set(WARNINGS_CXX_FLAGS
    -pedantic -Wall -Wextra -Wno-c++0x-compat -Wnon-virtual-dtor -Weffc++
    -Wconversion -Werror -Wshadow -Wcast-qual -Winit-self -Wno-unused-parameter)
  # -Winline -Wfloat-equal -Wunreachable-code
endif()

add_executable(randomstream randomstream.cpp)
target_compile_options(randomstream PRIVATE -std=c++11)
target_compile_options(randomstream PRIVATE ${WARNINGS_CXX_FLAGS})

# EOF #
