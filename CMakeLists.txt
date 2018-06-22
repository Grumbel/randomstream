cmake_minimum_required(VERSION 3.0)
project(randomstream VERSION 0.0.0)

find_package(Threads)

option(WARNINGS "Switch on extra warnings" OFF)
option(WERROR "Turn warnings into errors" OFF)

if(WARNINGS)
  set(WARNINGS_CXX_FLAGS
    -pedantic -Wall -Wextra -Wno-c++0x-compat -Wnon-virtual-dtor -Weffc++
    -Wconversion -Werror -Wshadow -Wcast-qual -Winit-self -Wno-unused-parameter)
  # -Winline -Wfloat-equal -Wunreachable-code
endif()

if(WERROR)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
endif()

add_executable(randomstream randomstream.cpp)
target_compile_options(randomstream PRIVATE -std=c++14)
target_compile_options(randomstream PRIVATE ${WARNINGS_CXX_FLAGS})
target_link_libraries (randomstream ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS randomstream DESTINATION bin)

# EOF #
